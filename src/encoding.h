#ifndef TIPPSE_ENCODING_H
#define TIPPSE_ENCODING_H

#include <stdlib.h>
#include "types.h"

struct range_tree_node;
struct encoding;
struct encoding_stream;
struct encoding_cache;
struct encoding_cache_codepoint;

#define ENCODING_STREAM_PLAIN 0
#define ENCODING_STREAM_PAGED 1

#define ENCODING_CACHE_SIZE 1024

struct encoding_stream {
  int type;                             // type of stream (plain or paged)

  const uint8_t* plain;                 // address of buffer
  struct range_tree_node* buffer;       // page in tree, if paged stream

  size_t displacement;                  // offset in buffer
  size_t cache_length;                  // length of buffer
};

struct encoding_cache {
  size_t start;                         // current position in cache
  size_t end;                           // last position in cache
  struct encoding* encoding;            // encoding used for stream
  struct encoding_stream* stream;       // input stream
  codepoint_t codepoints[ENCODING_CACHE_SIZE];  // cache with codepoints
  size_t lengths[ENCODING_CACHE_SIZE];  // cache with lengths of codepoints
};

struct encoding {
  struct encoding* (*create)(void);
  void (*destroy)(struct encoding*);

  const char* (*name)(void);
  size_t (*character_length)(struct encoding*);
  codepoint_t (*visual)(struct encoding*, codepoint_t);
  codepoint_t (*decode)(struct encoding*, struct encoding_stream*, size_t*);
  size_t (*next)(struct encoding*, struct encoding_stream*);
  size_t (*strnlen)(struct encoding*, struct encoding_stream*, size_t);
  size_t (*strlen)(struct encoding*, struct encoding_stream*);
  size_t (*seek)(struct encoding*, struct encoding_stream*, size_t);
  size_t (*encode)(struct encoding*, codepoint_t, uint8_t*, size_t);
};

#include "rangetree.h"

void encoding_stream_from_plain(struct encoding_stream* stream, const uint8_t* plain, size_t size);
void encoding_stream_from_page(struct encoding_stream* stream, struct range_tree_node* buffer, file_offset_t displacement);

uint8_t encoding_stream_read_forward_oob(struct encoding_stream* stream);
inline uint8_t encoding_stream_read_forward(struct encoding_stream* stream) {
  if (stream->displacement<stream->cache_length) {
    return *(stream->plain+stream->displacement++);
  } else {
    return encoding_stream_read_forward_oob(stream);
  }
}

uint8_t encoding_stream_read_reverse_oob(struct encoding_stream* stream);
inline uint8_t encoding_stream_read_reverse(struct encoding_stream* stream) {
  if (stream->displacement>0) {
    return *(stream->plain+(--stream->displacement));
  } else {
    return encoding_stream_read_reverse_oob(stream);
  }
}

void encoding_stream_forward_oob(struct encoding_stream* stream, size_t length);
inline void encoding_stream_forward(struct encoding_stream* stream, size_t length) {
  if (stream->displacement+length<=stream->cache_length) {
    stream->displacement += length;
  } else {
    encoding_stream_forward_oob(stream, length);
  }
}

void encoding_stream_reverse_oob(struct encoding_stream* stream, size_t length);
inline void encoding_stream_reverse(struct encoding_stream* stream, size_t length) {
  if (stream->displacement>=length) {
    stream->displacement -= length;
  } else {
    encoding_stream_reverse_oob(stream, length);
  }
}

struct range_tree_node* encoding_stream_end_oob(struct encoding_stream* stream);
struct range_tree_node* encoding_stream_start_oob(struct encoding_stream* stream);

inline int encoding_stream_end(struct encoding_stream* stream) {
  return (stream->displacement>=stream->cache_length && (stream->type==ENCODING_STREAM_PLAIN || !stream->buffer || !encoding_stream_end_oob(stream)))?1:0;
}

inline int encoding_stream_start(struct encoding_stream* stream) {
  return (stream->displacement>=stream->cache_length && (stream->type==ENCODING_STREAM_PLAIN || !stream->buffer || !encoding_stream_start_oob(stream)))?1:0;
}

void encoding_cache_clear(struct encoding_cache* cache, struct encoding* encoding, struct encoding_stream* stream);
// Skip code points and rebase absolute offset
inline void encoding_cache_advance(struct encoding_cache* cache, size_t advance) {
  cache->start += advance;
  if (cache->end<=cache->start) {
    cache->end = 0;
    cache->start = 0;
  }
}

void encoding_cache_buffer(struct encoding_cache* cache, size_t offset);
// Returned code point from relative offset
inline codepoint_t encoding_cache_find_codepoint(struct encoding_cache* cache, size_t offset) {
  if (offset+cache->start>=cache->end) {
    encoding_cache_buffer(cache, offset);
  }

  return cache->codepoints[(cache->start+offset)%ENCODING_CACHE_SIZE];
}

// Returned code point byte length from relative offset
inline size_t encoding_cache_find_length(struct encoding_cache* cache, size_t offset) {
  if (offset+cache->start>=cache->end) {
    encoding_cache_buffer(cache, offset);
  }

  return cache->lengths[(cache->start+offset)%ENCODING_CACHE_SIZE];
}

uint16_t* encoding_reverse_table(uint16_t* table, size_t length, size_t max);

#endif  /* #ifndef TIPPSE_ENCODING_H */
