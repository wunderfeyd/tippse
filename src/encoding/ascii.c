// Tippse - Encoding ASCII - Encode/Decode of ASCII

#include "ascii.h"

struct encoding* encoding_ascii_create(void) {
  struct encoding_ascii* this = malloc(sizeof(struct encoding_ascii));
  this->vtbl.create = encoding_ascii_create;
  this->vtbl.destroy = encoding_ascii_destroy;
  this->vtbl.name = encoding_ascii_name;
  this->vtbl.character_length = encoding_ascii_character_length;
  this->vtbl.decode = encoding_ascii_decode;
  this->vtbl.visual = encoding_ascii_visual;
  this->vtbl.next = encoding_ascii_next;
  this->vtbl.strnlen = encoding_ascii_strnlen;
  this->vtbl.strlen = encoding_ascii_strlen;
  this->vtbl.seek = encoding_ascii_seek;

  return (struct encoding*)this;
}

void encoding_ascii_destroy(struct encoding* base) {
  struct encoding_ascii* this = (struct encoding_ascii*)base;
  free(this);
}

const char* encoding_ascii_name(void) {
  return "ASCII";
}

size_t encoding_ascii_character_length(struct encoding* base) {
  return 1;
}

int encoding_ascii_visual(struct encoding* base, int cp) {
  if (cp<0 || cp>255) {
    return -1;
  } else if (cp<0x20) {
    return cp+0x2400;
  } else if (cp==0x7f) {
    return 0x2421;
  } else if (cp>=0x80) {
    return 0xfffd;
  }

  return cp;
}

int encoding_ascii_decode(struct encoding* base, struct encoding_stream* stream, size_t* used) {
  uint8_t c = encoding_stream_peek(stream, 0);
  *used = 1;
  return (int)c;
}

size_t encoding_ascii_encode(struct encoding* base, int cp, uint8_t* text, size_t size) {
  *text = (uint8_t)cp;
  return 1;
}

size_t encoding_ascii_next(struct encoding* base, struct encoding_stream* stream) {
  return 1;
}

size_t encoding_ascii_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size) {
  return size;
}

size_t encoding_ascii_strlen(struct encoding* base, struct encoding_stream* stream) {
  size_t length = 0;
  while (1) {
    size_t next;
    int cp = encoding_ascii_decode(base, stream, &next);
    if (cp==0) {
      break;
    }

    encoding_stream_forward(stream, next);
    length++;
  }

  return length;
}

size_t encoding_ascii_seek(struct encoding* base, struct encoding_stream* stream, size_t pos) {
  size_t current = 0;
  while (pos!=0) {
    size_t next = encoding_ascii_next(base, stream);
    current += next;
    encoding_stream_forward(stream, next);
    pos--;
  }

  return current;
}
