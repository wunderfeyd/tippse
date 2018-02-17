#ifndef TIPPSE_ENCODING_UTF16BE_H
#define TIPPSE_ENCODING_UTF16BE_H

#include <stdlib.h>

struct encoding_utf16be;

#include "../encoding.h"

struct encoding_utf16be {
  struct encoding vtbl;
};

struct encoding* encoding_utf16be_create(void);
void encoding_utf16be_destroy(struct encoding* base);

const char* encoding_utf16be_name(void);
size_t encoding_utf16be_character_length(struct encoding* base);
codepoint_t encoding_utf16be_visual(struct encoding* base, codepoint_t cp);
codepoint_t encoding_utf16be_decode(struct encoding* base, struct stream* stream, size_t* used);
size_t encoding_utf16be_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size);
size_t encoding_utf16be_next(struct encoding* base, struct stream* stream);

#endif  /* #ifndef TIPPSE_ENCODING_UTF16BE_H */
