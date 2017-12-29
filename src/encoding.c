// Tippse - Encoding - Base for different encoding schemes (characters <-> bytes)

#include "encoding.h"

// Build streaming view into plain memory
void encoding_stream_from_plain(struct encoding_stream* stream, const uint8_t* plain, size_t size) {
  stream->type = ENCODING_STREAM_PLAIN;
  stream->plain = plain;
  stream->displacement = 0;
  stream->cache_length = size;
}

// Build streaming view onto a range tree
void encoding_stream_from_page(struct encoding_stream* stream, struct range_tree_node* buffer, file_offset_t displacement) {
  stream->type = ENCODING_STREAM_PAGED;
  stream->buffer = buffer;
  stream->displacement = displacement;
  stream->cache_length = buffer?buffer->length:0;
  if (buffer) {
    fragment_cache(buffer->buffer);
  }

  stream->plain = buffer?(buffer->buffer->buffer+buffer->offset):NULL;
}

// Build stream from file
void encoding_stream_from_file(struct encoding_stream* stream, struct file_cache* cache, file_offset_t offset) {
  stream->type = ENCODING_STREAM_FILE;
  stream->file.cache = cache;
  stream->file.node = NULL;
  stream->displacement = offset%TREE_BLOCK_LENGTH_MAX;
  stream->file.offset = offset-stream->displacement;
  stream->plain = file_cache_use_node_ranged(stream->file.cache, &stream->file.node, stream->file.offset, TREE_BLOCK_LENGTH_MAX);
  stream->cache_length = stream->file.node->length;
}

// Return stream offset
size_t encoding_stream_offset_plain(struct encoding_stream* stream) {
  return stream->displacement;
}

// Return stream offset
file_offset_t encoding_stream_offset_page(struct encoding_stream* stream) {
  return range_tree_offset(stream->buffer)+stream->displacement;
}

// Return stream offset
file_offset_t encoding_stream_offset_file(struct encoding_stream* stream) {
  return stream->file.offset+stream->displacement;
}

// Return streaming data that cannot read directly (forward)
uint8_t encoding_stream_read_forward_oob(struct encoding_stream* stream) {
  if (stream->type==ENCODING_STREAM_PLAIN) {
    stream->displacement++;
    return 0;
  }

  encoding_stream_forward_oob(stream, 0);

  if (stream->displacement<stream->cache_length) {
    return *(stream->plain+stream->displacement++);
  } else {
    stream->displacement++;
    return 0;
  }
}

// Return streaming data that cannot read directly (reverse)
uint8_t encoding_stream_read_reverse_oob(struct encoding_stream* stream) {
  if (stream->type==ENCODING_STREAM_PLAIN) {
    stream->displacement--;
    return 0;
  }

  encoding_stream_reverse_oob(stream, 1);

  if (stream->displacement<stream->cache_length) {
    return *(stream->plain+stream->displacement);
  } else {
    return 0;
  }
}

// Forward to next leaf in tree if direct forward failed
void encoding_stream_forward_oob(struct encoding_stream* stream, size_t length) {
  stream->displacement += length;

  if (stream->type==ENCODING_STREAM_PLAIN) {
  } else if (stream->type==ENCODING_STREAM_PAGED) {
    while (stream->buffer) {
      struct range_tree_node* buffer = range_tree_next(stream->buffer);
      if (!buffer) {
        return;
      }

      stream->displacement -= stream->buffer->length;
      stream->buffer = buffer;
      stream->cache_length = stream->buffer->length;
      if (stream->displacement<stream->buffer->length) {
        fragment_cache(stream->buffer->buffer);
        stream->plain = stream->buffer->buffer->buffer+stream->buffer->offset;
        break;
      }
    }
  } else if (stream->type==ENCODING_STREAM_FILE) {
    while (stream->cache_length>=TREE_BLOCK_LENGTH_MAX && stream->displacement>=TREE_BLOCK_LENGTH_MAX) {
      stream->displacement -= TREE_BLOCK_LENGTH_MAX;
      stream->file.offset += TREE_BLOCK_LENGTH_MAX;
      stream->plain = file_cache_use_node_ranged(stream->file.cache, &stream->file.node, stream->file.offset, TREE_BLOCK_LENGTH_MAX);
      stream->cache_length = stream->file.node->length;
    }
  }
}

// Back to previous leaf in tree if direct reverse failed
void encoding_stream_reverse_oob(struct encoding_stream* stream, size_t length) {
  stream->displacement -= length;

  if (stream->type==ENCODING_STREAM_PLAIN) {
  } else if (stream->type==ENCODING_STREAM_PAGED) {
    while (stream->buffer) {
      struct range_tree_node* buffer = range_tree_prev(stream->buffer);
      if (!buffer) {
        return;
      }

      stream->buffer = buffer;
      stream->displacement += stream->buffer->length;
      stream->cache_length = stream->buffer->length;
      if (stream->displacement<stream->buffer->length) {
        fragment_cache(stream->buffer->buffer);
        stream->plain = stream->buffer->buffer->buffer+stream->buffer->offset;
        break;
      }
    }
  } else if (stream->type==ENCODING_STREAM_FILE) {
    while (stream->file.offset>0 && stream->displacement>=TREE_BLOCK_LENGTH_MAX) {
      stream->displacement += TREE_BLOCK_LENGTH_MAX;
      stream->file.offset -= TREE_BLOCK_LENGTH_MAX;
      stream->plain = file_cache_use_node_ranged(stream->file.cache, &stream->file.node, stream->file.offset, TREE_BLOCK_LENGTH_MAX);
      stream->cache_length = stream->file.node->length;
    }
  }
}

// Helper for range tree end check (TODO: clean include files to allow inline usage of range_tree_next)
int encoding_stream_end_oob(struct encoding_stream* stream) {
  return ((stream->type==ENCODING_STREAM_PAGED && (!stream->buffer || !range_tree_next(stream->buffer))) || (stream->type==ENCODING_STREAM_FILE && stream->cache_length<TREE_BLOCK_LENGTH_MAX))?1:0;
}

// Helper for range tree start check (TODO: clean include files to allow inline usage of range_tree_prev)
int encoding_stream_start_oob(struct encoding_stream* stream) {
  return ((stream->type==ENCODING_STREAM_PAGED && (!stream->buffer || !range_tree_prev(stream->buffer))) || (stream->type==ENCODING_STREAM_FILE && stream->file.offset==0))?1:0;
}

// Reset code point cache
void encoding_cache_clear(struct encoding_cache* cache, struct encoding* encoding, struct encoding_stream* stream) {
  cache->start = 0;
  cache->end = 0;
  cache->encoding = encoding;
  cache->stream = stream;
}

// Fill code point cache
void encoding_cache_buffer(struct encoding_cache* cache, size_t offset) {
  while (cache->end-cache->start<=offset) {
    size_t pos = cache->end%ENCODING_CACHE_SIZE;
    cache->codepoints[pos] = (*cache->encoding->decode)(cache->encoding, cache->stream, &cache->lengths[pos]);
    cache->end++;
  }
}

// Invert lookup table for unicode and codepage translation
uint16_t* encoding_reverse_table(uint16_t* table, size_t length, size_t max) {
  uint16_t* output = (uint16_t*)malloc(sizeof(uint16_t)*max);
  for (size_t n = 0; n<max; n++) {
    output[n] = (uint16_t)~0u;
  }

  for (size_t n = 0; n<length; n++) {
    size_t set = (size_t)table[n];
    if (set<max) {
      output[set] = (uint16_t)n;
    }
  }

  return output;
}
