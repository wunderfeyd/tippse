// Tippse - Stream - A random access view onto different data buffer structures

#include "stream.h"

#include "filecache.h"
#include "fragment.h"

// Build streaming view into plain memory
void stream_from_plain(struct stream* base, const uint8_t* plain, size_t size) {
  base->type = STREAM_TYPE_PLAIN;
  base->plain = plain;
  base->displacement = 0;
  base->page_offset = 0;
  base->cache_length = size;
}

// Build streaming view onto a range tree
void stream_from_page(struct stream* base, struct range_tree_node* buffer, file_offset_t displacement) {
  base->type = STREAM_TYPE_PAGED;
  base->buffer = buffer;
  base->displacement = displacement%FILE_CACHE_PAGE_SIZE;
  base->page_offset = displacement-base->displacement;
  stream_reference_page(base);
}

// Build stream from file
void stream_from_file(struct stream* base, struct file_cache* cache, file_offset_t offset) {
  base->type = STREAM_TYPE_FILE;
  base->file.cache = cache;
  base->displacement = offset%FILE_CACHE_PAGE_SIZE;
  base->page_offset = 0;
  base->file.offset = offset-base->displacement;
  base->cache_node = file_cache_invoke(base->file.cache, base->file.offset, FILE_CACHE_PAGE_SIZE, &base->plain, &base->cache_length);
}

// Clear data structures
void stream_destroy(struct stream* base) {
  if (base->type==STREAM_TYPE_PAGED) {
    stream_unreference_page(base);
  } else if (base->type==STREAM_TYPE_FILE) {
    file_cache_revoke(base->file.cache, base->cache_node);
  }
}

// Copy one stream to another
void stream_clone(struct stream* dst, struct stream* src) {
  memcpy(dst, src, sizeof(struct stream));
  if (dst->type==STREAM_TYPE_PAGED) {
    if (dst->buffer && dst->buffer->buffer && dst->buffer->buffer->type==FRAGMENT_FILE) {
      file_cache_clone(dst->buffer->buffer->cache, dst->cache_node);
    }
  } else if (dst->type==STREAM_TYPE_FILE) {
    file_cache_clone(dst->file.cache, dst->cache_node);
  }
}

// Check for page end since the page might be fragmented in cache
int stream_rereference_page(struct stream* base) {
  if (base->buffer && stream_combined_offset(base->page_offset, base->displacement)<range_tree_length(base->buffer)) {
    stream_unreference_page(base);
    stream_reference_page(base);
    return 1;
  }

  return 0;
}

// Load page from fragment or file cache
void stream_reference_page(struct stream* base) {
  if (base->buffer && base->buffer->buffer && base->buffer->buffer->type==FRAGMENT_FILE) {
    file_offset_t ondisk = base->buffer->buffer->offset+base->buffer->offset;
    file_offset_t final = ondisk+stream_combined_offset(base->page_offset, base->displacement);

    file_offset_t page = final-(final%FILE_CACHE_PAGE_SIZE);
    file_offset_t max = page>ondisk?page:ondisk;
    size_t displacement = (final-max)%FILE_CACHE_PAGE_SIZE;

    base->page_offset = max-ondisk;
    size_t cache_length;
    base->cache_node = file_cache_invoke(base->buffer->buffer->cache, max, FILE_CACHE_PAGE_SIZE, &base->plain, &cache_length);

    file_offset_t page_size = range_tree_length(base->buffer);
    base->displacement = displacement;
    base->cache_length = page_size>cache_length?cache_length:page_size;
  } else {
    base->plain = base->buffer?(base->buffer->buffer->buffer+base->buffer->offset):NULL;
    base->cache_length = range_tree_length(base->buffer);
  }
}

// Release page
void stream_unreference_page(struct stream* base) {
  if (base->buffer && base->buffer->buffer && base->buffer->buffer->type==FRAGMENT_FILE) {
    file_cache_revoke(base->buffer->buffer->cache, base->cache_node);
  }
}

// Return stream offset (plain stream)
size_t stream_offset_plain(struct stream* base) {
  return base->displacement;
}

// Return stream offset (page stream)
file_offset_t stream_offset_page(struct stream* base) {
  return range_tree_offset(base->buffer)+base->page_offset+base->displacement;
}

// Return stream offset (file stream)
file_offset_t stream_offset_file(struct stream* base) {
  return base->file.offset+base->displacement;
}

// Return stream offset (generalization)
file_offset_t stream_offset(struct stream* base) {
  if (base->type==STREAM_TYPE_PLAIN) {
    return (file_offset_t)stream_offset_plain(base);
  } else if (base->type==STREAM_TYPE_PAGED) {
    return (file_offset_t)stream_offset_page(base);
  } else if (base->type==STREAM_TYPE_FILE) {
    return (file_offset_t)stream_offset_file(base);
  }
  return 0;
}

// Return streaming data that cannot read directly (forward)
uint8_t stream_read_forward_oob(struct stream* base) {
  if (base->type==STREAM_TYPE_PLAIN) {
    base->displacement++;
    return 0;
  }

  stream_forward_oob(base, 0);

  if (base->displacement<base->cache_length) {
    return *(base->plain+base->displacement++);
  } else {
    base->displacement++;
    return 0;
  }
}

// Return streaming data that cannot read directly (reverse)
uint8_t stream_read_reverse_oob(struct stream* base) {
  if (base->type==STREAM_TYPE_PLAIN) {
    return 0;
  }

  stream_reverse_oob(base, 0);

  if (base->displacement<base->cache_length) {
    return *(base->plain+base->displacement);
  } else {
    return 0;
  }
}

// Forward to next leaf in tree if direct forward failed
void stream_forward_oob(struct stream* base, size_t length) {
  base->displacement += length;

  if (base->type==STREAM_TYPE_PLAIN) {
  } else if (base->type==STREAM_TYPE_PAGED) {
    if (stream_rereference_page(base)) {
      return;
    }

    stream_unreference_page(base);
    file_offset_t displacement = stream_combined_offset(base->page_offset, base->displacement);
    base->page_offset = 0;
    while (base->buffer) {
      struct range_tree_node* buffer = range_tree_next(base->buffer);
      if (!buffer) {
        base->displacement = displacement%FILE_CACHE_PAGE_SIZE;
        base->page_offset = displacement-base->displacement;
        stream_reference_page(base);
        return;
      }

      displacement -= base->buffer->length;
      base->buffer = buffer;
      if (displacement<base->buffer->length) {
        base->displacement = displacement%FILE_CACHE_PAGE_SIZE;
        base->page_offset = displacement-base->displacement;
        stream_reference_page(base);
        break;
      }
    }
  } else if (base->type==STREAM_TYPE_FILE) {
    while (base->cache_length>=FILE_CACHE_PAGE_SIZE && base->displacement>=FILE_CACHE_PAGE_SIZE) {
      file_cache_revoke(base->file.cache, base->cache_node);
      base->displacement -= FILE_CACHE_PAGE_SIZE;
      base->file.offset += FILE_CACHE_PAGE_SIZE;
      base->cache_node = file_cache_invoke(base->file.cache, base->file.offset, FILE_CACHE_PAGE_SIZE, &base->plain, &base->cache_length);
    }
  }
}

// Back to previous leaf in tree if direct reverse failed
void stream_reverse_oob(struct stream* base, size_t length) {
  base->displacement -= length;

  if (base->type==STREAM_TYPE_PLAIN) {
  } else if (base->type==STREAM_TYPE_PAGED) {
    if (stream_rereference_page(base)) {
      return;
    }

    stream_unreference_page(base);
    file_offset_t displacement = stream_combined_offset(base->page_offset, base->displacement);
    base->page_offset = 0;
    while (base->buffer) {
      struct range_tree_node* buffer = range_tree_prev(base->buffer);
      if (!buffer) {
        base->displacement = displacement%FILE_CACHE_PAGE_SIZE;
        base->page_offset = displacement-base->displacement;
        stream_reference_page(base);
        return;
      }

      base->buffer = buffer;
      displacement += base->buffer->length;
      if (displacement<base->buffer->length) {
        base->displacement = displacement%FILE_CACHE_PAGE_SIZE;
        base->page_offset = displacement-base->displacement;
        stream_reference_page(base);
        break;
      }
    }
  } else if (base->type==STREAM_TYPE_FILE) {
    while (base->file.offset>0 && (ssize_t)base->displacement<0) {
      file_cache_revoke(base->file.cache, base->cache_node);
      base->displacement += FILE_CACHE_PAGE_SIZE;
      base->file.offset -= FILE_CACHE_PAGE_SIZE;
      base->cache_node = file_cache_invoke(base->file.cache, base->file.offset, FILE_CACHE_PAGE_SIZE, &base->plain, &base->cache_length);
    }
  }
}
