#ifndef TIPPSE_ENCODING_UTF16LE_H
#define TIPPSE_ENCODING_UTF16LE_H

#include <stdlib.h>

struct encoding_utf16le;

#include "../encoding.h"

struct encoding_utf16le {
  struct encoding vtbl;
};

struct encoding* encoding_utf16le_create(void);
void encoding_utf16le_destroy(struct encoding* base);

const char* encoding_utf16le_name(void);
size_t encoding_utf16le_character_length(struct encoding* base);
codepoint_t encoding_utf16le_visual(struct encoding* base, codepoint_t cp);
codepoint_t encoding_utf16le_decode(struct encoding* base, struct stream* stream, size_t* used);
size_t encoding_utf16le_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size);
size_t encoding_utf16le_next(struct encoding* base, struct stream* stream);

#endif  /* #ifndef TIPPSE_ENCODING_UTF16LE_H */
