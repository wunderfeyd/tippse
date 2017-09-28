#ifndef __TIPPSE_ENCODING_CP850__
#define __TIPPSE_ENCODING_CP850__

#include <stdlib.h>

struct encoding_cp850;

#include "../encoding.h"

struct encoding_cp850 {
  struct encoding vtbl;
};

struct encoding* encoding_cp850_create();
void encoding_cp850_destroy(struct encoding* base);

const char* encoding_cp850_name();
size_t encoding_cp850_character_length(struct encoding* base);
int encoding_cp850_visual(struct encoding* base, int cp);
int encoding_cp850_decode(struct encoding* base, struct encoding_stream* stream, size_t* used);
size_t encoding_cp850_encode(struct encoding* base, int cp, uint8_t* text, size_t size);
size_t encoding_cp850_next(struct encoding* base, struct encoding_stream* stream);
size_t encoding_cp850_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size);
size_t encoding_cp850_strlen(struct encoding* base, struct encoding_stream* stream);
size_t encoding_cp850_seek(struct encoding* base, struct encoding_stream* stream, size_t pos);

#endif  /* #ifndef __TIPPSE_ENCODING_CP850__ */
