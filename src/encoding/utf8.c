// Tippse - Encoding UTF-8 - Encode/Decode of UTF-8 codepoints

#include "utf8.h"

struct encoding* encoding_utf8_create(void) {
  struct encoding_utf8* this = malloc(sizeof(struct encoding_utf8));
  this->vtbl.create = encoding_utf8_create;
  this->vtbl.destroy = encoding_utf8_destroy;
  this->vtbl.name = encoding_utf8_name;
  this->vtbl.character_length = encoding_utf8_character_length;
  this->vtbl.encode = encoding_utf8_encode;
  this->vtbl.decode = encoding_utf8_decode;
  this->vtbl.visual = encoding_utf8_visual;
  this->vtbl.next = encoding_utf8_next;
  this->vtbl.strnlen = encoding_utf8_strnlen;
  this->vtbl.strlen = encoding_utf8_strlen;
  this->vtbl.seek = encoding_utf8_seek;

  return (struct encoding*)this;
}

void encoding_utf8_destroy(struct encoding* base) {
  struct encoding_utf8* this = (struct encoding_utf8*)base;
  free(this);
}

const char* encoding_utf8_name(void) {
  return "UTF-8";
}

size_t encoding_utf8_character_length(struct encoding* base) {
  return 4;
}

codepoint_t encoding_utf8_visual(struct encoding* base, codepoint_t cp) {
  if (cp<0) {
    return -1;
  } else if (cp<0x20) {
    return cp+0x2400;
  }

  return cp;
}

codepoint_t encoding_utf8_decode(struct encoding* base, struct encoding_stream* stream, size_t* used) {
  uint8_t c = encoding_stream_read_forward(stream);
  if ((c&0x80)==0) {
    *used = 1;
    return (codepoint_t)c;
  }

  if ((c&0xe0)==0xc0) {
    if ((c&0x1f)<2) {
      *used = 1;
      return -1;
    }

    uint8_t c1 = encoding_stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 1);
      *used = 1;
      return -1;
    }

    *used = 2;
    return ((c&0x1f)<<6)|((c1&0x7f)<<0);
  } else if ((c&0xf0)==0xe0) {
    uint8_t c1 = encoding_stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 1);
      *used = 1;
      return -1;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      encoding_stream_reverse(stream, 1);
      *used = 1;
      return -1;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      encoding_stream_reverse(stream, 1);
      *used = 1;
      return -1;
    }

    uint8_t c2 = encoding_stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 2);
      *used = 1;
      return -1;
    }

    *used = 3;
    return ((c&0x0f)<<12)|((c1&0x7f)<<6)|((c2&0x7f)<<0);
  } else if ((c&0xf8)==0xf0) {
    if (c>0xf4) {
      *used = 1;
      return -1;
    }

    uint8_t c1 = encoding_stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 1);
      *used = 1;
      return -1;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      encoding_stream_reverse(stream, 1);
      *used = 1;
      return -1;
    }

    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      encoding_stream_reverse(stream, 1);
      *used = 1;
      return -1;
    }

    uint8_t c2 = encoding_stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 2);
      *used = 1;
      return -1;
    }

    uint8_t c3 = encoding_stream_read_forward(stream);
    if ((c3&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 3);
      *used = 1;
      return -1;
    }

    *used = 4;
    return ((c&0x07)<<18)|((c1&0x7f)<<12)|((c2&0x7f)<<6)|((c3&0x7f)<<0);
  }

  *used = 1;
  return -1;
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

size_t encoding_utf8_next(struct encoding* base, struct encoding_stream* stream) {
  uint8_t c = encoding_stream_read_forward(stream);
  if ((c&0x80)==0) {
    return 1;
  }

  if ((c&0xe0)==0xc0) {
    if ((c&0x1f)<2) {
      return 1;
    }

    uint8_t c1 = encoding_stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 1);
      return 1;
    }

    return 2;
  } else if ((c&0xf0)==0xe0) {
    uint8_t c1 = encoding_stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      encoding_stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      encoding_stream_reverse(stream, 1);
      return 1;
    }

    uint8_t c2 = encoding_stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 2);
      return 1;
    }

    return 3;
  } else if ((c&0xf8)==0xf0) {
    if (c>0xf4) {
      return 1;
    }

    uint8_t c1 = encoding_stream_read_forward(stream);
    if ((c1&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 1);
      return 1;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      encoding_stream_reverse(stream, 1);
      return 1;
    }
    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      encoding_stream_reverse(stream, 1);
      return 1;
    }

    uint8_t c2 = encoding_stream_read_forward(stream);
    if ((c2&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 2);
      return 1;
    }

    uint8_t c3 = encoding_stream_read_forward(stream);
    if ((c3&0xc0)!=0x80) {
      encoding_stream_reverse(stream, 3);
      return 1;
    }

    return 4;
  }

  return 1;
}

size_t encoding_utf8_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size) {
  size_t length = 0;
  while (size!=0) {
    size_t next = encoding_utf8_next(base, stream);
    if (next>size) {
      size = 0;
    } else {
      size -= next;
    }

    length++;
  }

  return length;
}

size_t encoding_utf8_strlen(struct encoding* base, struct encoding_stream* stream) {
  size_t length = 0;
  while (1) {
    size_t next;
    codepoint_t cp = encoding_utf8_decode(base, stream, &next);
    if (cp==0) {
      break;
    }

    length++;
  }

  return length;
}

size_t encoding_utf8_seek(struct encoding* base, struct encoding_stream* stream, size_t pos) {
  size_t current = 0;
  while (pos!=0) {
    size_t next = encoding_utf8_next(base, stream);
    current += next;
    pos--;
  }

  return current;
}
