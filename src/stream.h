#ifndef TIPPSE_STREAM_H
#define TIPPSE_STREAM_H

#include <stdlib.h>
#include "types.h"

struct range_tree_node;
struct stream;
struct file_cache;
struct file_cache_node;

#define STREAM_TYPE_PLAIN 0
#define STREAM_TYPE_PAGED 1
#define STREAM_TYPE_FILE 2

struct stream {
  int type;                             // type of stream

  const uint8_t* plain;                 // address of buffer
  size_t displacement;                  // offset in buffer
  size_t cache_length;                  // length of buffer

  union {
    struct range_tree_node* buffer;     // page in tree, if paged stream
    struct {
      struct file_cache_node* node;   // cached node
      struct file_cache* cache;         // file cache itself
      file_offset_t offset;             // current file position
    } file;
  };
};

#include "rangetree.h"

void stream_from_plain(struct stream* stream, const uint8_t* plain, size_t size);
void stream_from_page(struct stream* stream, struct range_tree_node* buffer, file_offset_t displacement);
void stream_from_file(struct stream* stream, struct file_cache* cache, file_offset_t offset);

size_t stream_offset_plain(struct stream* stream);
file_offset_t stream_offset_page(struct stream* stream);
file_offset_t stream_offset_file(struct stream* stream);

uint8_t stream_read_forward_oob(struct stream* stream);
inline uint8_t stream_read_forward(struct stream* stream) {
  if (stream->displacement<stream->cache_length) {
    return *(stream->plain+stream->displacement++);
  } else {
    return stream_read_forward_oob(stream);
  }
}

uint8_t stream_read_reverse_oob(struct stream* stream);
inline uint8_t stream_read_reverse(struct stream* stream) {
  if (stream->displacement>0) {
    return *(stream->plain+(--stream->displacement));
  } else {
    return stream_read_reverse_oob(stream);
  }
}

void stream_forward_oob(struct stream* stream, size_t length);
inline void stream_forward(struct stream* stream, size_t length) {
  if (stream->displacement+length<=stream->cache_length) {
    stream->displacement += length;
  } else {
    stream_forward_oob(stream, length);
  }
}

void stream_reverse_oob(struct stream* stream, size_t length);
inline void stream_reverse(struct stream* stream, size_t length) {
  if (stream->displacement>=length) {
    stream->displacement -= length;
  } else {
    stream_reverse_oob(stream, length);
  }
}

int stream_end_oob(struct stream* stream);
int stream_start_oob(struct stream* stream);

inline int stream_end(struct stream* stream) {
  return (stream->displacement>=stream->cache_length && (stream->type==STREAM_TYPE_PLAIN || stream_end_oob(stream)))?1:0;
}

inline int stream_start(struct stream* stream) {
  return (stream->displacement>stream->cache_length && (stream->type==STREAM_TYPE_PLAIN || stream_start_oob(stream)))?1:0;
}

#endif  /* #ifndef TIPPSE_STREAM_H */