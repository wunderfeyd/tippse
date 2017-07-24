/* Tippse - Encoding - Base for different encoding schemes (characters <-> bytes) */

#include "encoding.h"

// Build streaming view into plain memory
void encoding_stream_from_plain(struct encoding_stream* stream, const char* plain, size_t size) {
  stream->type = ENCODING_STREAM_PLAIN;
  stream->plain = plain;
  stream->buffer_pos = 0;
  stream->cache_length = size;
}

// Build streaming view onto a range tree
void encoding_stream_from_page(struct encoding_stream* stream, struct range_tree_node* buffer, file_offset_t buffer_pos) {
  stream->type = ENCODING_STREAM_PAGED;
  stream->buffer = buffer;
  stream->buffer_pos = buffer_pos;
  stream->cache_length = buffer?(buffer->length-buffer_pos):0;
  stream->plain = buffer?(buffer->buffer->buffer+buffer->offset+buffer_pos):NULL;
}

// Return streaming data that cannot peeked directly
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

// Forward to next leaf in tree if direct forward failed
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
