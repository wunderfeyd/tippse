/* Tippse - Encoding UTF8 - Encode/Decode of UTF8 codepoints */

#include "encoding_utf8.h"

struct encoding* encoding_utf8_create() {
  struct encoding_utf8* this = malloc(sizeof(struct encoding_utf8));
  this->vtbl.create = encoding_utf8_create;
  this->vtbl.destroy = encoding_utf8_destroy;
  this->vtbl.character_length = encoding_utf8_character_length;
  this->vtbl.decode = encoding_utf8_decode;
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

size_t encoding_utf8_character_length(struct encoding* base) {
  return 4;
}

int encoding_utf8_decode(struct encoding* base, struct encoding_stream* stream, size_t size, size_t* used) {
  if (size==0) {
    *used = 0;
    return -1;
  }

  unsigned char c = encoding_stream_peek(stream, 0);
  if ((c&0x80)==0) {
    *used = 1;
    return (int)c;
  }

  if ((c&0xe0)==0xc0 && size>1) {
    if ((c&0x1f)<2) {
      *used = 1;
      return -1;
    }

    unsigned char c1 = encoding_stream_peek(stream, 1);
    if ((c1&0xc0)!=0x80) {
      *used = 1;
      return -1;
    }

    *used = 2;
    return ((c&0x1f)<<6)|((c1&0x7f)<<0);
  } else if ((c&0xf0)==0xe0 && size>2) {
    unsigned char c1 = encoding_stream_peek(stream, 1);
    if ((c1&0xc0)!=0x80) {
      *used = 1;
      return -1;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      *used = 1;
      return -1;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      *used = 1;
      return -1;
    }

    unsigned char c2 = encoding_stream_peek(stream, 2);
    if ((c2&0xc0)!=0x80) {
      *used = 1;
      return -1;
    }

    *used = 3;
    return ((c&0x0f)<<12)|((c1&0x7f)<<6)|((c2&0x7f)<<0);
  } else if ((c&0xf8)==0xf0 && size>3) {
    if (c>0xf4) {
      *used = 1;
      return -1;
    }

    unsigned char c1 = encoding_stream_peek(stream, 1);
    if ((c1&0xc0)!=0x80) {
      *used = 1;
      return -1;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      *used = 1;
      return -1;
    }

    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      *used = 1;
      return -1;
    }

    unsigned char c2 = encoding_stream_peek(stream, 2);
    if ((c2&0xc0)!=0x80) {
      *used = 1;
      return -1;
    }

    unsigned char c3 = encoding_stream_peek(stream, 3);
    if ((c3&0xc0)!=0x80) {
      *used = 1;
      return -1;
    }

    *used = 4;
    return ((c&0x07)<<18)|((c1&0x7f)<<12)|((c2&0x7f)<<6)|((c3&0x7f)<<0);
  }

  *used = 1;
  return -1;
}

size_t encoding_utf8_encode(struct encoding* base, int cp, struct encoding_stream* stream, size_t size) {
  /*if (cp<0x80) {
    if (size<1) {
      return 0;
    }

    *(unsigned char*)text++ = (unsigned char)cp;
    return 1;
  } else if (cp<0x800) {
    if (size<2) {
      return 0;
    }

    *(unsigned char*)text++ = 0xc0+(unsigned char)(cp>>6);
    *(unsigned char*)text++ = 0x80+(unsigned char)(cp&0x3f);
    return 2;
  } else if (cp<0x10000) {
    if (size<3) {
      return 0;
    }

    *(unsigned char*)text++ = 0xe0+(unsigned char)(cp>>12);
    *(unsigned char*)text++ = 0x80+(unsigned char)((cp>>6)&0x3f);
    *(unsigned char*)text++ = 0x80+(unsigned char)(cp&0x3f);
    return 3;
  } else if (cp<0x101000) {
    if (size<4) {
      return 0;
    }

    *(unsigned char*)text++ = 0xf0+(unsigned char)(cp>>18);
    *(unsigned char*)text++ = 0x80+(unsigned char)((cp>>12)&0x3f);
    *(unsigned char*)text++ = 0x80+(unsigned char)((cp>>6)&0x3f);
    *(unsigned char*)text++ = 0x80+(unsigned char)(cp&0x3f);
    return 4;
  }*/

  return 0;
}

size_t encoding_utf8_next(struct encoding* base, struct encoding_stream* stream, size_t size) {
  if (size==0) {
    return 0;
  }

  unsigned char c = encoding_stream_peek(stream, 0);
  if ((c&0x80)==0) {
    return 1;
  }

  if ((c&0xe0)==0xc0 && size>1) {
    if ((c&0x1f)<2) {
      return 1;
    }

    unsigned char c1 = encoding_stream_peek(stream, 1);
    if ((c1&0xc0)!=0x80) {
      return 1;
    }

    return 2;
  } else if ((c&0xf0)==0xe0 && size>2) {
    unsigned char c1 = encoding_stream_peek(stream, 1);
    if ((c1&0xc0)!=0x80) {
      return 1;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      return 1;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      return 1;
    }

    unsigned char c2 = encoding_stream_peek(stream, 2);
    if ((c2&0xc0)!=0x80) {
      return 1;
    }

    return 3;
  } else if ((c&0xf8)==0xf0 && size>3) {
    if (c>0xf4) {
      return 1;
    }

    unsigned char c1 = encoding_stream_peek(stream, 1);
    if ((c1&0xc0)!=0x80) {
      return 1;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      return 1;
    }
    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      return 1;
    }

    unsigned char c2 = encoding_stream_peek(stream, 2);
    if ((c2&0xc0)!=0x80) {
      return 1;
    }

    unsigned char c3 = encoding_stream_peek(stream, 3);
    if ((c3&0xc0)!=0x80) {
      return 1;
    }

    return 4;
  }

  return 1;
}

size_t encoding_utf8_strnlen(struct encoding* base, struct encoding_stream* stream, size_t size) {
  size_t length = 0;
  while (size!=0) {
    size_t next = encoding_utf8_next(base, stream, size);
    if (next>size) {
      size = 0;
    } else {
      size -= next;
    }

    encoding_stream_forward(stream, next);
    length++;
  }

  return length;
}

size_t encoding_utf8_strlen(struct encoding* base, struct encoding_stream* stream) {
  size_t length = 0;
  while (1) {
    size_t next;
    int cp = encoding_utf8_decode(base, stream, ~0, &next);
    if (cp==0) {
      break;
    }

    encoding_stream_forward(stream, next);
    length++;
  }

  return length;
}

size_t encoding_utf8_seek(struct encoding* base, struct encoding_stream* stream, size_t pos) {
  size_t current = 0;
  while (pos!=0) {
    size_t next = encoding_utf8_next(base, stream, ~0);
    current += next;
    encoding_stream_forward(stream, next);
    pos--;
  }

  return current;
}
