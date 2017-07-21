/* Tippse - Encoding - Base for different encoding schemes (characters <-> bytes) */

#include "encoding.h"

void encoding_stream_from_plain(struct encoding_stream* stream, const char* plain, size_t size) {
  stream->type = ENCODING_STREAM_PLAIN;
  stream->plain = plain;
  stream->buffer_pos = 0;
  stream->cache_length = size;
}

void encoding_stream_from_page(struct encoding_stream* stream, struct range_tree_node* buffer, file_offset_t buffer_pos) {
  stream->type = ENCODING_STREAM_PAGED;
  stream->buffer = buffer;
  stream->buffer_pos = buffer_pos;
  stream->cache_length = buffer?(buffer->length-buffer_pos):0;
  stream->plain = buffer?(buffer->buffer->buffer+buffer->offset+buffer_pos):NULL;
}

uint8_t encoding_stream_peek_oob(struct encoding_stream* stream, size_t offset) {
  if (stream->type==ENCODING_STREAM_PLAIN) {
    return 0;
  }

  file_offset_t buffer_pos = stream->buffer_pos+offset;
  struct range_tree_node* buffer = stream->buffer;
  while (buffer && buffer_pos>=buffer->length) {
    buffer_pos -= buffer->length;
    buffer = range_tree_next(buffer);
  }

  if (buffer) {
    return *(buffer->buffer->buffer+buffer->offset+buffer_pos);
  } else {
    return 0;
  }
}

void encoding_stream_forward_oob(struct encoding_stream* stream, size_t length) {
  if (stream->type==ENCODING_STREAM_PLAIN) {
    stream->cache_length = 0;
  } else {
    stream->buffer_pos = length-stream->cache_length;
    stream->cache_length = 0;
    while (stream->buffer) {
      stream->buffer = range_tree_next(stream->buffer);
      if (!stream->buffer) {
        break;
      }

      if (stream->buffer_pos>=stream->buffer->length) {
        stream->buffer_pos -= stream->buffer->length;
        continue;
      }

      stream->cache_length = stream->buffer->length-stream->buffer_pos;
      stream->plain = stream->buffer->buffer->buffer+stream->buffer->offset+stream->buffer_pos;
      break;
    }
  }
}

void encoding_cache_clear(struct encoding_cache* cache) {
  cache->start = 0;
  cache->end = 0;
}

void encoding_cache_fill(struct encoding_cache* cache, struct encoding* encoding, struct encoding_stream* stream, size_t advance) {
  cache->start += advance;

  while (cache->end-cache->start<ENCODING_CACHE_SIZE) {
    size_t pos = cache->end%ENCODING_CACHE_SIZE;
    cache->cache[pos].cp = (*encoding->decode)(encoding, stream, &cache->cache[pos].length);

    encoding_stream_forward(stream, cache->cache[pos].length);

    cache->end++;
  }
}

int encoding_cache_find_codepoint(struct encoding_cache* cache, size_t offset) {
  return cache->cache[(cache->start+offset)%ENCODING_CACHE_SIZE].cp;
}

size_t encoding_cache_find_length(struct encoding_cache* cache, size_t offset) {
  return cache->cache[(cache->start+offset)%ENCODING_CACHE_SIZE].length;
}
