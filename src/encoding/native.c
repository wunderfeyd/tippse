// Tippse - Encoding native - Encode/Decode of internal code point representation (currently integer)

// TODO: define codepoint type in types.h

#include "native.h"

struct encoding* encoding_native_create(void) {
  struct encoding_native* this = malloc(sizeof(struct encoding_native));
  this->vtbl.create = encoding_native_create;
  this->vtbl.destroy = encoding_native_destroy;
  this->vtbl.name = encoding_native_name;
  this->vtbl.character_length = encoding_native_character_length;
  this->vtbl.encode = encoding_native_encode;
  this->vtbl.decode = encoding_native_decode;
  this->vtbl.visual = encoding_native_visual;
  this->vtbl.next = encoding_native_next;
  this->vtbl.strnlen = encoding_native_strnlen;
  this->vtbl.strlen = encoding_native_strlen;
  this->vtbl.seek = encoding_native_seek;

  return (struct encoding*)this;
}

void encoding_native_destroy(struct encoding* base) {
  struct encoding_native* this = (struct encoding_native*)base;
  free(this);
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

codepoint_t encoding_native_decode(struct encoding* base, struct encoding_stream* stream, size_t* used) {
  union {
    codepoint_t cp;
    uint8_t c[sizeof(int)];
  } u;

  for (size_t n = 0; n<sizeof(codepoint_t); n++) {
    u.c[n] = encoding_stream_peek(stream, n);
  }

  *used = sizeof(codepoint_t);
  return u.cp;
}

size_t encoding_native_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size) {
  *(codepoint_t*)text = cp;
  return sizeof(codepoint_t);
}

size_t encoding_native_next(struct encoding* base, struct encoding_stream* stream) {
  return sizeof(codepoint_t);
}

size_t encoding_native_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size) {
  return size;
}

size_t encoding_native_strlen(struct encoding* base, struct encoding_stream* stream) {
  size_t length = 0;
  while (1) {
    size_t next;
    codepoint_t cp = encoding_native_decode(base, stream, &next);
    if (cp==0) {
      break;
    }

    encoding_stream_forward(stream, next);
    length++;
  }

  return length;
}

size_t encoding_native_seek(struct encoding* base, struct encoding_stream* stream, size_t pos) {
  size_t current = 0;
  while (pos!=0) {
    size_t next = encoding_native_next(base, stream);
    current += next;
    encoding_stream_forward(stream, next);
    pos--;
  }

  return current;
}
