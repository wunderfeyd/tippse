#ifndef TIPPSE_ENCODING_NATIVE_H
#define TIPPSE_ENCODING_NATIVE_H

#include <stdlib.h>
#include "../encoding.h"

struct encoding_native {
  struct encoding vtbl;
};

struct encoding* encoding_native_create(void);
void encoding_native_destroy(struct encoding* base);

const char* encoding_native_name(void);
size_t encoding_native_character_length(struct encoding* base);
codepoint_t encoding_native_visual(struct encoding* base, codepoint_t cp);
codepoint_t encoding_native_decode(struct encoding* base, struct stream* stream, size_t* used);
size_t encoding_native_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size);
size_t encoding_native_next(struct encoding* base, struct stream* stream);

#endif  /* #ifndef TIPPSE_ENCODING_NATIVE_H */
