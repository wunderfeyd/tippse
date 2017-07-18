#ifndef __TIPPSE_ENCODING__
#define __TIPPSE_ENCODING__

#include <stdlib.h>

struct encoding;
struct encoding_stream;

#include "rangetree.h"

#define ENCODING_STREAM_PLAIN 0
#define ENCODING_STREAM_PAGED 1

struct encoding_stream {
  int type;

  const char* plain;
  struct range_tree_node* buffer;
  file_offset_t buffer_pos;
};

struct encoding {
  struct encoding* (*create)();
  void (*destroy)(struct encoding*);

  const char* (*name)();
  size_t (*character_length)(struct encoding*);
  int (*decode)(struct encoding*, struct encoding_stream*, size_t, size_t*);
  size_t (*next)(struct encoding*, struct encoding_stream*, size_t);
  size_t (*strnlen)(struct encoding*, struct encoding_stream*, size_t);
  size_t (*strlen)(struct encoding*, struct encoding_stream*);
  size_t (*seek)(struct encoding*, struct encoding_stream*, size_t);
  size_t (*encode)(struct encoding*, int, struct encoding_stream*, size_t);
};

void encoding_stream_from_plain(struct encoding_stream* stream, const char* plain);
void encoding_stream_from_page(struct encoding_stream* stream, struct range_tree_node* buffer, file_offset_t buffer_pos);
unsigned char encoding_stream_peek(struct encoding_stream* stream, size_t offset);
void encoding_stream_forward(struct encoding_stream* stream, size_t length);

#endif  /* #ifndef __TIPPSE_ENCODING__ */
