#ifndef __TIPPSE_ENCODING_ASCII__
#define __TIPPSE_ENCODING_ASCII__

#include <stdlib.h>

struct encoding_ascii;

#include "../encoding.h"

struct encoding_ascii {
  struct encoding vtbl;
};

struct encoding* encoding_ascii_create();
void encoding_ascii_destroy(struct encoding* base);

const char* encoding_ascii_name();
size_t encoding_ascii_character_length(struct encoding* base);
int encoding_ascii_visual(struct encoding* base, int cp);
int encoding_ascii_decode(struct encoding* base, struct encoding_stream* stream, size_t* used);
size_t encoding_ascii_encode(struct encoding* base, int cp, uint8_t* text, size_t size);
size_t encoding_ascii_next(struct encoding* base, struct encoding_stream* stream);
size_t encoding_ascii_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size);
size_t encoding_ascii_strlen(struct encoding* base, struct encoding_stream* stream);
size_t encoding_ascii_seek(struct encoding* base, struct encoding_stream* stream, size_t pos);

#endif  /* #ifndef __TIPPSE_ENCODING_ASCII__ */
