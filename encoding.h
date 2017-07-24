#ifndef __TIPPSE_ENCODING__
#define __TIPPSE_ENCODING__

#include <stdlib.h>

struct encoding;
struct encoding_stream;
struct encoding_cache;
struct encoding_cache_codepoint;

#include "rangetree.h"

#define ENCODING_STREAM_PLAIN 0
#define ENCODING_STREAM_PAGED 1

#define ENCODING_CACHE_SIZE 1024

struct encoding_cache_codepoint {
  int cp;
  size_t length;
};

struct encoding_stream {
  int type;

  const char* plain;
  struct range_tree_node* buffer;

  size_t buffer_pos;
  size_t cache_length;
};

struct encoding_cache {
  size_t start;
  size_t end;
  int empty;
  struct encoding* encoding;
  struct encoding_stream* stream;
  struct encoding_cache_codepoint cache[ENCODING_CACHE_SIZE];
};

struct encoding {
  struct encoding* (*create)();
  void (*destroy)(struct encoding*);

  const char* (*name)();
  size_t (*character_length)(struct encoding*);
  int (*decode)(struct encoding*, struct encoding_stream*, size_t*);
  size_t (*next)(struct encoding*, struct encoding_stream*);
  size_t (*strnlen)(struct encoding*, struct encoding_stream*, size_t);
  size_t (*strlen)(struct encoding*, struct encoding_stream*);
  size_t (*seek)(struct encoding*, struct encoding_stream*, size_t);
  size_t (*encode)(struct encoding*, int, struct encoding_stream*, size_t);
};

void encoding_stream_from_plain(struct encoding_stream* stream, const char* plain, size_t size);
void encoding_stream_from_page(struct encoding_stream* stream, struct range_tree_node* buffer, file_offset_t buffer_pos);
uint8_t encoding_stream_peek_oob(struct encoding_stream* stream, size_t offset);
inline uint8_t encoding_stream_peek(struct encoding_stream* stream, size_t offset) {
  if (offset<stream->cache_length) {
    return *(stream->plain+offset);
  } else {
    return encoding_stream_peek_oob(stream, offset);
  }
}

void encoding_stream_forward_oob(struct encoding_stream* stream, size_t length);
inline void encoding_stream_forward(struct encoding_stream* stream, size_t length) {
  stream->plain += length;
  if (length<stream->cache_length) {
    stream->cache_length -= length;
  } else {
    encoding_stream_forward_oob(stream, length);
  }
}

void encoding_cache_clear(struct encoding_cache* cache);
void encoding_cache_fill(struct encoding_cache* cache, struct encoding* encoding, struct encoding_stream* stream, size_t advance);
int encoding_cache_find_codepoint(struct encoding_cache* cache, size_t offset);
size_t encoding_cache_find_length(struct encoding_cache* cache, size_t offset);

#endif  /* #ifndef __TIPPSE_ENCODING__ */
