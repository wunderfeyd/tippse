#ifndef TIPPSE_ENCODING_UTF8_H
#define TIPPSE_ENCODING_UTF8_H

#include <stdlib.h>

struct encoding_utf8;

#include "../encoding.h"

struct encoding_utf8 {
  struct encoding vtbl;
};

struct encoding* encoding_utf8_create(void);
void encoding_utf8_destroy(struct encoding* base);

const char* encoding_utf8_name(void);
size_t encoding_utf8_character_length(struct encoding* base);
codepoint_t encoding_utf8_visual(struct encoding* base, codepoint_t cp);
codepoint_t encoding_utf8_decode(struct encoding* base, struct encoding_stream* stream, size_t* used);
size_t encoding_utf8_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size);
size_t encoding_utf8_next(struct encoding* base, struct encoding_stream* stream);
size_t encoding_utf8_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size);
size_t encoding_utf8_strlen(struct encoding* base, struct encoding_stream* stream);
size_t encoding_utf8_seek(struct encoding* base, struct encoding_stream* stream, size_t pos);

#endif  /* #ifndef TIPPSE_ENCODING_UTF8_H */
