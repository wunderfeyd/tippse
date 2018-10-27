// Tippse - Encoding UTF-8 - Encode/Decode of UTF-8 codepoints

#include "utf8.h"

struct encoding* encoding_utf8_create(void) {
  struct encoding_utf8* self = malloc(sizeof(struct encoding_utf8));
  self->vtbl.create = encoding_utf8_create;
  self->vtbl.destroy = encoding_utf8_destroy;
  self->vtbl.name = encoding_utf8_name;
  self->vtbl.character_length = encoding_utf8_character_length;
  self->vtbl.encode = encoding_utf8_encode;
  self->vtbl.decode = encoding_utf8_decode;
  self->vtbl.visual = encoding_utf8_visual;
  self->vtbl.next = encoding_utf8_next;

  return (struct encoding*)self;
}

void encoding_utf8_destroy(struct encoding* base) {
  struct encoding_utf8* self = (struct encoding_utf8*)base;
  free(self);
}

const char* encoding_utf8_name(void) {
  return "UTF-8";
}

size_t encoding_utf8_character_length(struct encoding* base) {
  return 4;
}

codepoint_t encoding_utf8_visual(struct encoding* base, codepoint_t cp) {
  if (cp<0) {
    return UNICODE_CODEPOINT_BAD;
  } else if (cp<0x20) {
    return cp+0x2400;
  }

  return cp;
}

codepoint_t encoding_utf8_decode(struct encoding* base, struct stream* stream, size_t* used) {
  uint8_t c = stream_read_forward(stream);
  if ((c&0x80)==0) {
    *used = 1;
    return (codepoint_t)c;
  }

  if ((c&0xe0)==0xc0) {
    if ((c&0x1f)<2) {
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    *used = 2;
    return ((c&0x1f)<<6)|((c1&0x7f)<<0);
  } else if ((c&0xf0)==0xe0) {
    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      stream_reverse(stream, 1);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      stream_reverse(stream, 1);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    uint8_t c2 = stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      stream_reverse(stream, 2);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    *used = 3;
    return ((c&0x0f)<<12)|((c1&0x7f)<<6)|((c2&0x7f)<<0);
  } else if ((c&0xf8)==0xf0) {
    if (c>0xf4) {
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      stream_reverse(stream, 1);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      stream_reverse(stream, 1);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    uint8_t c2 = stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      stream_reverse(stream, 2);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    uint8_t c3 = stream_read_forward(stream);
    if ((c3&0xc0)!=0x80) {
      stream_reverse(stream, 3);
      *used = 1;
      return UNICODE_CODEPOINT_BAD;
    }

    *used = 4;
    return ((c&0x07)<<18)|((c1&0x7f)<<12)|((c2&0x7f)<<6)|((c3&0x7f)<<0);
  }

  *used = 1;
  return UNICODE_CODEPOINT_BAD;
}

size_t encoding_utf8_encode(struct encoding* base, codepoint_t cp, uint8_t* text, size_t size) {
  if (cp<0x80) {
    if (size<1) {
      return 0;
    }

    *text++ = (uint8_t)cp;
    return 1;
  } else if (cp<0x800) {
    if (size<2) {
      return 0;
    }

    *text++ = 0xc0+(uint8_t)(cp>>6);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 2;
  } else if (cp<0x10000) {
    if (size<3) {
      return 0;
    }

    *text++ = 0xe0+(uint8_t)(cp>>12);
    *text++ = 0x80+(uint8_t)((cp>>6)&0x3f);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 3;
  } else if (cp<0x101000) {
    if (size<4) {
      return 0;
    }

    *text++ = 0xf0+(uint8_t)(cp>>18);
    *text++ = 0x80+(uint8_t)((cp>>12)&0x3f);
    *text++ = 0x80+(uint8_t)((cp>>6)&0x3f);
    *text++ = 0x80+(uint8_t)(cp&0x3f);
    return 4;
  }

  return 0;
}

size_t encoding_utf8_next(struct encoding* base, struct stream* stream) {
  uint8_t c = stream_read_forward(stream);
  if ((c&0x80)==0) {
    return 1;
  }

  if ((c&0xe0)==0xc0) {
    if ((c&0x1f)<2) {
      return 1;
    }

    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      return 1;
    }

    return 2;
  } else if ((c&0xf0)==0xe0) {
    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      stream_reverse(stream, 1);
      return 1;
    }

    uint8_t c2 = stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      stream_reverse(stream, 2);
      return 1;
    }

    return 3;
  } else if ((c&0xf8)==0xf0) {
    if (c>0xf4) {
      return 1;
    }

    uint8_t c1 = stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      stream_reverse(stream, 1);
      return 1;
    }
    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      stream_reverse(stream, 1);
      return 1;
    }

    uint8_t c2 = stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      stream_reverse(stream, 2);
      return 1;
    }

    uint8_t c3 = stream_read_forward(stream);
    if ((c3&0xc0)!=0x80) {
      stream_reverse(stream, 3);
      return 1;
    }

    return 4;
  }

  return 1;
}
