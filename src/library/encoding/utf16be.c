// Tippse - Encoding UTF-16BE - Encode/Decode of UTF-16BE codepoints

#include "utf16be.h"

struct encoding* encoding_utf16be_create(void) {
  struct encoding_utf16be* self = (struct encoding_utf16be*)malloc(sizeof(struct encoding_utf16be));
  self->vtbl.create = encoding_utf16be_create;
  self->vtbl.destroy = encoding_utf16be_destroy;
  self->vtbl.name = encoding_utf16be_name;
  self->vtbl.character_length = encoding_utf16be_character_length;
  self->vtbl.encode = encoding_utf16be_encode;
  self->vtbl.decode = encoding_utf16be_decode;
  self->vtbl.visual = encoding_utf16be_visual;
  self->vtbl.next = encoding_utf16be_next;

  return (struct encoding*)self;
}

void encoding_utf16be_destroy(struct encoding* base) {
  struct encoding_utf16be* self = (struct encoding_utf16be*)base;
  free(self);
}

const char* encoding_utf16be_name(void) {
  return "UTF-16BE";
}

size_t encoding_utf16be_character_length(struct encoding* base) {
  return 4;
}

codepoint_t encoding_utf16be_visual(struct encoding* base, codepoint_t cp) {
  if (cp>UNICODE_CODEPOINT_MAX) {
    return UNICODE_CODEPOINT_BAD;
  } else if (cp<0x20) {
    return cp+0x2400;
  }

  return cp;
}

codepoint_t encoding_utf16be_decode(struct encoding* base, struct stream* stream, size_t* used) {
  uint8_t c1 = stream_read_forward(stream);
  uint8_t c2 = stream_read_forward(stream);
  codepoint_t cp = (codepoint_t)((c1<<8)|c2);
  if (cp>=0xd800 && cp<0xdc00) {
    uint8_t c1 = stream_read_forward(stream);
    uint8_t c2 = stream_read_forward(stream);
    if (cp>=0xdc00 && cp<0xe000) {
      cp <<= 10;
      cp |= (codepoint_t)((c1<<8)|c2);
      cp += 0x10000;
      *used = 4;
    } else {
      stream_reverse(stream, 2);
      *used = 2;
    }
  } else {
    *used = 2;
  }

  return cp;
}

size_t encoding_utf16be_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size) {
  if (cp<0x10000) {
    *text++ = (uint8_t)((cp>>8)&0xff);
    *text++ = (uint8_t)(cp&0xff);
    return 2;
  } else {
    cp -= 0x10000;
    *text++ = (uint8_t)(((cp>>18)&0x03)|0xd8);
    *text++ = (uint8_t)((cp>>10)&0xff);
    *text++ = (uint8_t)(((cp>>8)&0x03)|0xdc);
    *text++ = (uint8_t)((cp>>0)&0xff);
    return 4;
  }
}

size_t encoding_utf16be_next(struct encoding* base, struct stream* stream) {
  uint8_t c1 = stream_read_forward(stream);
  stream_read_forward(stream);
  if (c1>=0xd8 && c1<0xdc) {
    uint8_t c1 = stream_read_forward(stream);
    stream_read_forward(stream);
    if (c1>=0xdc && c1<0xe0) {
      return 4;
    }
    stream_reverse(stream, 2);
  }

  return 2;
}
