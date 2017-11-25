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
  stream->cache_length = buffer?(buffer->length-displacement):0;
  if (buffer) {
    fragment_cache(buffer->buffer);
  }

  stream->plain = buffer?(buffer->buffer->buffer+buffer->offset+displacement):NULL;
}

// Return streaming data that cannot peeked directly
uint8_t encoding_stream_peek_oob(struct encoding_stream* stream, size_t offset) {
  if (stream->type==ENCODING_STREAM_PLAIN) {
    return 0;
  }

  file_offset_t displacement = offset-stream->cache_length;
  struct range_tree_node* buffer = stream->buffer;
  while (buffer) {
    buffer = range_tree_next(buffer);
    if (!buffer) {
      return 0;
    }

    if (displacement>=buffer->length) {
      displacement -= buffer->length;
      continue;
    }

    fragment_cache(buffer->buffer);
    return *(buffer->buffer->buffer+buffer->offset+displacement);
  }

  return 0;
}

// Forward to next leaf in tree if direct forward failed
void encoding_stream_forward_oob(struct encoding_stream* stream, size_t length) {
  if (stream->type==ENCODING_STREAM_PLAIN) {
    stream->cache_length = 0;
  } else {
    stream->displacement = length-stream->cache_length;
    stream->cache_length = 0;
    while (stream->buffer) {
      stream->buffer = range_tree_next(stream->buffer);
      if (!stream->buffer) {
        break;
      }

      if (stream->displacement>=stream->buffer->length) {
        stream->displacement -= stream->buffer->length;
        continue;
      }

      stream->cache_length = stream->buffer->length-stream->displacement;
      fragment_cache(stream->buffer->buffer);
      stream->plain = stream->buffer->buffer->buffer+stream->buffer->offset+stream->displacement;
      break;
    }
  }
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

    encoding_stream_forward(cache->stream, cache->lengths[pos]);

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
