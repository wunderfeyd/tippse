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

void stream_from_plain(struct stream* base, const uint8_t* plain, size_t size);
void stream_from_page(struct stream* base, struct range_tree_node* buffer, file_offset_t displacement);
void stream_from_file(struct stream* base, struct file_cache* cache, file_offset_t offset);

size_t stream_offset_plain(struct stream* base);
file_offset_t stream_offset_page(struct stream* base);
file_offset_t stream_offset_file(struct stream* base);
file_offset_t stream_offset(struct stream* base);

uint8_t stream_read_forward_oob(struct stream* base);
inline uint8_t stream_read_forward(struct stream* base) {
  if (base->displacement<base->cache_length) {
    return *(base->plain+base->displacement++);
  } else {
    return stream_read_forward_oob(base);
  }
}

uint8_t stream_read_reverse_oob(struct stream* base);
inline uint8_t stream_read_reverse(struct stream* base) {
  base->displacement--;
  if (base->displacement<base->cache_length) {
    return *(base->plain+base->displacement);
  } else {
    return stream_read_reverse_oob(base);
  }
}

void stream_forward_oob(struct stream* base, size_t length);
inline void stream_forward(struct stream* base, size_t length) {
  if (base->displacement+length<=base->cache_length) {
    base->displacement += length;
  } else {
    stream_forward_oob(base, length);
  }
}

void stream_reverse_oob(struct stream* base, size_t length);
inline void stream_reverse(struct stream* base, size_t length) {
  if (base->displacement>=length) {
    base->displacement -= length;
  } else {
    stream_reverse_oob(base, length);
  }
}

int stream_end_oob(struct stream* base);
int stream_start_oob(struct stream* base);

inline int stream_end(struct stream* base) {
  return (base->displacement>=base->cache_length && (base->type==STREAM_TYPE_PLAIN || stream_end_oob(base)))?1:0;
}

inline int stream_start(struct stream* base) {
  return (base->displacement>base->cache_length && (base->type==STREAM_TYPE_PLAIN || stream_start_oob(base)))?1:0;
}

#endif  /* #ifndef TIPPSE_STREAM_H */
