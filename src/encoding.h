#ifndef TIPPSE_ENCODING_H
#define TIPPSE_ENCODING_H

#include <stdlib.h>
#include "types.h"

struct encoding;
struct encoding_cache;
struct encoding_cache_codepoint;
struct stream;

#define ENCODING_CACHE_SIZE 1024

struct encoding_cache {
  size_t start;                         // current position in cache
  size_t end;                           // last position in cache
  struct encoding* encoding;            // encoding used for stream
  struct stream* stream;                // input stream
  codepoint_t codepoints[ENCODING_CACHE_SIZE];  // cache with codepoints
  size_t lengths[ENCODING_CACHE_SIZE];  // cache with lengths of codepoints
};

struct encoding {
  struct encoding* (*create)(void);
  void (*destroy)(struct encoding*);

  const char* (*name)(void);
  size_t (*character_length)(struct encoding*);
  codepoint_t (*visual)(struct encoding*, codepoint_t);
  codepoint_t (*decode)(struct encoding*, struct stream*, size_t*);
  size_t (*next)(struct encoding*, struct stream*);
  size_t (*strnlen)(struct encoding*, struct stream*, size_t);
  size_t (*strlen)(struct encoding*, struct stream*);
  size_t (*seek)(struct encoding*, struct stream*, size_t);
  size_t (*encode)(struct encoding*, codepoint_t, uint8_t*, size_t);
};

void encoding_cache_clear(struct encoding_cache* base, struct encoding* encoding, struct stream* stream);
// Skip code points and rebase absolute offset
inline void encoding_cache_advance(struct encoding_cache* base, size_t advance) {
  base->start += advance;
  if (base->end<=base->start) {
    base->end = 0;
    base->start = 0;
  }
}

void encoding_cache_buffer(struct encoding_cache* base, size_t offset);
// Returned code point from relative offset
inline codepoint_t encoding_cache_find_codepoint(struct encoding_cache* base, size_t offset) {
  if (offset+base->start>=base->end) {
    encoding_cache_buffer(base, offset);
  }

  return base->codepoints[(base->start+offset)%ENCODING_CACHE_SIZE];
}

// Returned code point byte length from relative offset
inline size_t encoding_cache_find_length(struct encoding_cache* base, size_t offset) {
  if (offset+base->start>=base->end) {
    encoding_cache_buffer(base, offset);
  }

  return base->lengths[(base->start+offset)%ENCODING_CACHE_SIZE];
}

uint16_t* encoding_reverse_table(uint16_t* table, size_t length, size_t max);

struct range_tree_node* encoding_transform_stream(struct stream* stream, struct encoding* from, struct encoding* to);
uint8_t* encoding_transform_plain(const uint8_t* buffer, size_t length, struct encoding* from, struct encoding* to);

#include "stream.h"

#endif  /* #ifndef TIPPSE_ENCODING_H */
