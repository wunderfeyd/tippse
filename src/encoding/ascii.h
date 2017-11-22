#ifndef TIPPSE_ENCODING_ASCII_H
#define TIPPSE_ENCODING_ASCII_H

#include <stdlib.h>

struct encoding_ascii;

#include "../encoding.h"

struct encoding_ascii {
  struct encoding vtbl;
};

struct encoding* encoding_ascii_create(void);
void encoding_ascii_destroy(struct encoding* base);

const char* encoding_ascii_name(void);
size_t encoding_ascii_character_length(struct encoding* base);
codepoint_t encoding_ascii_visual(struct encoding* base, codepoint_t cp);
codepoint_t encoding_ascii_decode(struct encoding* base, struct encoding_stream* stream, size_t* used);
size_t encoding_ascii_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size);
size_t encoding_ascii_next(struct encoding* base, struct encoding_stream* stream);
size_t encoding_ascii_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size);
size_t encoding_ascii_strlen(struct encoding* base, struct encoding_stream* stream);
size_t encoding_ascii_seek(struct encoding* base, struct encoding_stream* stream, size_t pos);

#endif  /* #ifndef TIPPSE_ENCODING_ASCII_H */
