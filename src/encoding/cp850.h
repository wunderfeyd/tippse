#ifndef TIPPSE_ENCODING_CP850_H
#define TIPPSE_ENCODING_CP850_H

#include <stdlib.h>

struct encoding_cp850;

#include "../encoding.h"

struct encoding_cp850 {
  struct encoding vtbl;
};

struct encoding* encoding_cp850_create(void);
void encoding_cp850_destroy(struct encoding* base);

const char* encoding_cp850_name(void);
size_t encoding_cp850_character_length(struct encoding* base);
codepoint_t encoding_cp850_visual(struct encoding* base, codepoint_t cp);
codepoint_t encoding_cp850_decode(struct encoding* base, struct stream* stream, size_t* used);
size_t encoding_cp850_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size);
size_t encoding_cp850_next(struct encoding* base, struct stream* stream);

#endif  /* #ifndef TIPPSE_ENCODING_CP850_H */
