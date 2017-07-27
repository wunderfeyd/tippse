#ifndef __TIPPSE_ENCODING_UTF8__
#define __TIPPSE_ENCODING_UTF8__

#include <stdlib.h>

struct encoding_utf8;

#include "encoding.h"

struct encoding_utf8 {
  struct encoding vtbl;
};

struct encoding* encoding_utf8_create();
void encoding_utf8_destroy(struct encoding* base);

const char* encoding_utf8_name();
size_t encoding_utf8_character_length(struct encoding* base);
int encoding_utf8_decode(struct encoding* base, struct encoding_stream* stream, size_t* used);
size_t encoding_utf8_encode(struct encoding* base, int cp, uint8_t* text, size_t size);
size_t encoding_utf8_next(struct encoding* base, struct encoding_stream* stream);
size_t encoding_utf8_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size);
size_t encoding_utf8_strlen(struct encoding* base, struct encoding_stream* stream);
size_t encoding_utf8_seek(struct encoding* base, struct encoding_stream* stream, size_t pos);

#endif  /* #ifndef __TIPPSE_ENCODING_UTF8__ */
