/* Tippse - Encoding - Base for different encoding schemes (characters <-> bytes) */

#include "encoding.h"

void encoding_stream_from_plain(struct encoding_stream* stream, const char* plain) {
  stream->type = ENCODING_STREAM_PLAIN;
  stream->plain = plain;
}

void encoding_stream_from_page(struct encoding_stream* stream, struct range_tree_node* buffer, file_offset_t buffer_pos) {
  stream->type = ENCODING_STREAM_PAGED;
  stream->buffer = buffer;
  stream->buffer_pos = buffer_pos;
}

unsigned char encoding_stream_peek(struct encoding_stream* stream, size_t offset) {
  if (stream->type==ENCODING_STREAM_PLAIN) {
    return *(stream->plain+offset);
  } else {
    file_offset_t buffer_pos = stream->buffer_pos+offset;
    struct range_tree_node* buffer = stream->buffer;
    while (buffer && buffer_pos>=buffer->length && buffer->buffer) {
      buffer_pos -= buffer->length;
      buffer = range_tree_next(buffer);
      continue;
    }

    if (buffer) {
      return *(buffer->buffer->buffer+buffer->offset+buffer_pos);
    } else {
      return 0;
    }
  }
}

void encoding_stream_forward(struct encoding_stream* stream, size_t length) {
  if (stream->type==ENCODING_STREAM_PLAIN) {
    stream->plain += length;
  } else {
    stream->buffer_pos += length;
    while (stream->buffer && stream->buffer_pos>=stream->buffer->length && stream->buffer->buffer) {
      stream->buffer_pos -= stream->buffer->length;
      stream->buffer = range_tree_next(stream->buffer);
      continue;
    }
  }
}
