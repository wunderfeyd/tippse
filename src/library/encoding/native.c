// Tippse - Encoding native - Encode/Decode of internal code point representation (currently integer)

#include "native.h"

struct encoding* encoding_native_create(void) {
  struct encoding_native* self = (struct encoding_native*)malloc(sizeof(struct encoding_native));
  self->vtbl.create = encoding_native_create;
  self->vtbl.destroy = encoding_native_destroy;
  self->vtbl.name = encoding_native_name;
  self->vtbl.character_length = encoding_native_character_length;
  self->vtbl.encode = encoding_native_encode;
  self->vtbl.decode = encoding_native_decode;
  self->vtbl.visual = encoding_native_visual;
  self->vtbl.next = encoding_native_next;

  return (struct encoding*)self;
}

void encoding_native_destroy(struct encoding* base) {
  struct encoding_native* self = (struct encoding_native*)base;
  free(self);
}

const char* encoding_native_name(void) {
  return "Native";
}

size_t encoding_native_character_length(struct encoding* base) {
  return sizeof(codepoint_t);
}

codepoint_t encoding_native_visual(struct encoding* base, codepoint_t cp) {
  return cp;
}

codepoint_t encoding_native_decode(struct encoding* base, struct stream* stream, size_t* used) {
  union {
    codepoint_t cp;
    uint8_t c[sizeof(codepoint_t)];
  } u;

  for (size_t n = 0; n<sizeof(codepoint_t); n++) {
    u.c[n] = stream_read_forward(stream);
  }

  *used = sizeof(codepoint_t);
  return u.cp;
}

size_t encoding_native_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size) {
  *(codepoint_t*)text = cp;
  return sizeof(codepoint_t);
}

size_t encoding_native_next(struct encoding* base, struct stream* stream) {
  return sizeof(codepoint_t);
}
