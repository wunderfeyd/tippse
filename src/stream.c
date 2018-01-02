// Tippse - Stream - A random access view onto different data buffer structures

#include "stream.h"

// Build streaming view into plain memory
void stream_from_plain(struct stream* base, const uint8_t* plain, size_t size) {
  base->type = STREAM_TYPE_PLAIN;
  base->plain = plain;
  base->displacement = 0;
  base->cache_length = size;
}

// Build streaming view onto a range tree
void stream_from_page(struct stream* base, struct range_tree_node* buffer, file_offset_t displacement) {
  base->type = STREAM_TYPE_PAGED;
  base->buffer = buffer;
  base->displacement = displacement;
  base->cache_length = buffer?buffer->length:0;
  if (buffer) {
    fragment_cache(buffer->buffer);
  }

  base->plain = buffer?(buffer->buffer->buffer+buffer->offset):NULL;
}

// Build stream from file
void stream_from_file(struct stream* base, struct file_cache* cache, file_offset_t offset) {
  base->type = STREAM_TYPE_FILE;
  base->file.cache = cache;
  base->file.node = NULL;
  base->displacement = offset%TREE_BLOCK_LENGTH_MAX;
  base->file.offset = offset-base->displacement;
  base->plain = file_cache_use_node_ranged(base->file.cache, &base->file.node, base->file.offset, TREE_BLOCK_LENGTH_MAX);
  base->cache_length = base->file.node->length;
}

// Return stream offset (plain stream)
size_t stream_offset_plain(struct stream* base) {
  return base->displacement;
}

// Return stream offset (page stream)
file_offset_t stream_offset_page(struct stream* base) {
  return range_tree_offset(base->buffer)+base->displacement;
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
    while (base->buffer) {
      struct range_tree_node* buffer = range_tree_next(base->buffer);
      if (!buffer) {
        return;
      }

      base->displacement -= base->buffer->length;
      base->buffer = buffer;
      base->cache_length = base->buffer->length;
      if (base->displacement<base->buffer->length) {
        fragment_cache(base->buffer->buffer);
        base->plain = base->buffer->buffer->buffer+base->buffer->offset;
        break;
      }
    }
  } else if (base->type==STREAM_TYPE_FILE) {
    while (base->cache_length>=TREE_BLOCK_LENGTH_MAX && base->displacement>=TREE_BLOCK_LENGTH_MAX) {
      base->displacement -= TREE_BLOCK_LENGTH_MAX;
      base->file.offset += TREE_BLOCK_LENGTH_MAX;
      base->plain = file_cache_use_node_ranged(base->file.cache, &base->file.node, base->file.offset, TREE_BLOCK_LENGTH_MAX);
      base->cache_length = base->file.node->length;
    }
  }
}

// Back to previous leaf in tree if direct reverse failed
void stream_reverse_oob(struct stream* base, size_t length) {
  base->displacement -= length;

  if (base->type==STREAM_TYPE_PLAIN) {
  } else if (base->type==STREAM_TYPE_PAGED) {
    while (base->buffer) {
      struct range_tree_node* buffer = range_tree_prev(base->buffer);
      if (!buffer) {
        return;
      }

      base->buffer = buffer;
      base->displacement += base->buffer->length;
      base->cache_length = base->buffer->length;
      if (base->displacement<base->buffer->length) {
        fragment_cache(base->buffer->buffer);
        base->plain = base->buffer->buffer->buffer+base->buffer->offset;
        break;
      }
    }
  } else if (base->type==STREAM_TYPE_FILE) {
    while (base->file.offset>0 && base->displacement>=TREE_BLOCK_LENGTH_MAX) {
      base->displacement += TREE_BLOCK_LENGTH_MAX;
      base->file.offset -= TREE_BLOCK_LENGTH_MAX;
      base->plain = file_cache_use_node_ranged(base->file.cache, &base->file.node, base->file.offset, TREE_BLOCK_LENGTH_MAX);
      base->cache_length = base->file.node->length;
    }
  }
}

// Helper for range tree end check (TODO: clean include files to allow inline usage of range_tree_next)
int stream_end_oob(struct stream* base) {
  return ((base->type==STREAM_TYPE_PAGED && (!base->buffer || !range_tree_next(base->buffer))) || (base->type==STREAM_TYPE_FILE && base->cache_length<TREE_BLOCK_LENGTH_MAX))?1:0;
}

// Helper for range tree start check (TODO: clean include files to allow inline usage of range_tree_prev)
int stream_start_oob(struct stream* base) {
  return ((base->type==STREAM_TYPE_PAGED && (!base->buffer || !range_tree_prev(base->buffer))) || (base->type==STREAM_TYPE_FILE && base->file.offset==0))?1:0;
}
