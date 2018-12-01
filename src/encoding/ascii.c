// Tippse - Encoding ASCII - Encode/Decode of ASCII

#include "ascii.h"

struct encoding* encoding_ascii_create(void) {
  struct encoding_ascii* self = (struct encoding_ascii*)malloc(sizeof(struct encoding_ascii));
  self->vtbl.create = encoding_ascii_create;
  self->vtbl.destroy = encoding_ascii_destroy;
  self->vtbl.name = encoding_ascii_name;
  self->vtbl.character_length = encoding_ascii_character_length;
  self->vtbl.encode = encoding_ascii_encode;
  self->vtbl.decode = encoding_ascii_decode;
  self->vtbl.visual = encoding_ascii_visual;
  self->vtbl.next = encoding_ascii_next;

  return (struct encoding*)self;
}

void encoding_ascii_destroy(struct encoding* base) {
  struct encoding_ascii* self = (struct encoding_ascii*)base;
  free(self);
}

const char* encoding_ascii_name(void) {
  return "ASCII";
}

size_t encoding_ascii_character_length(struct encoding* base) {
  return 1;
}

codepoint_t encoding_ascii_visual(struct encoding* base, codepoint_t cp) {
  if (cp>255) {
    return UNICODE_CODEPOINT_BAD;
  } else if (cp<0x20) {
    return cp+0x2400;
  } else if (cp==0x7f) {
    return 0x2421;
  } else if (cp>=0x80) {
    return 0xfffd;
  }

  return cp;
}

codepoint_t encoding_ascii_decode(struct encoding* base, struct stream* stream, size_t* used) {
  uint8_t c = stream_read_forward(stream);
  *used = 1;
  return (codepoint_t)c;
}

size_t encoding_ascii_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size) {
  *text = (uint8_t)cp;
  return 1;
}

size_t encoding_ascii_next(struct encoding* base, struct stream* stream) {
  return 1;
}
