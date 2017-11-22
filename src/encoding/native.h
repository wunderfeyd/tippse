#ifndef TIPPSE_ENCODING_NATIVE_H
#define TIPPSE_ENCODING_NATIVE_H

#include <stdlib.h>

struct encoding_native;

#include "../encoding.h"

struct encoding_native {
  struct encoding vtbl;
};

struct encoding* encoding_native_create(void);
void encoding_native_destroy(struct encoding* base);

const char* encoding_native_name(void);
size_t encoding_native_character_length(struct encoding* base);
codepoint_t encoding_native_visual(struct encoding* base, codepoint_t cp);
codepoint_t encoding_native_decode(struct encoding* base, struct encoding_stream* stream, size_t* used);
size_t encoding_native_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size);
size_t encoding_native_next(struct encoding* base, struct encoding_stream* stream);
size_t encoding_native_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size);
size_t encoding_native_strlen(struct encoding* base, struct encoding_stream* stream);
size_t encoding_native_seek(struct encoding* base, struct encoding_stream* stream, size_t pos);

#endif  /* #ifndef TIPPSE_ENCODING_NATIVE_H */
