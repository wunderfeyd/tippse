/* Tippse - UTF8 - Encode/Decode of UTF8 codepoints */

#include "utf8.h"

const char* utf8_decode(int* cp, const char* text, size_t size, size_t skip) {
  if (size==0) {
    return text;
  }

  unsigned char c = *(unsigned char*)(text+0);
  if ((c&0x80)==0) {
    *cp = (int)c;
    return text+1;
  }

  if ((c&0xe0)==0xc0 && size>1) {
    if ((c&0x1f)<2) {
      return text+skip;
    }

    unsigned char c1 = *(unsigned char*)(text+1);
    if ((c1&0xc0)!=0x80) {
      return text+skip;
    }

    *cp = ((c&0x1f)<<6)|((c1&0x7f)<<0);
    return text+2;
  } else if ((c&0xf0)==0xe0 && size>2) {
    unsigned char c1 = *(unsigned char*)(text+1);
    if ((c1&0xc0)!=0x80) {
      return text+skip;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      return text+skip;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      return text+skip;
    }

    unsigned char c2 = *(unsigned char*)(text+2);
    if ((c2&0xc0)!=0x80) {
      return text+skip;
    }

    *cp = ((c&0x0f)<<12)|((c1&0x7f)<<6)|((c2&0x7f)<<0);
    return text+3;
  } else if ((c&0xf8)==0xf0 && size>3) {
    if (c>0xf4) {
      return text+skip;
    }

    unsigned char c1 = *(unsigned char*)(text+1);
    if ((c1&0xc0)!=0x80) {
      return text+skip;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      return text+skip;
    }

    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      return text+skip;
    }

    unsigned char c2 = *((unsigned char*)text+2);
    if ((c2&0xc0)!=0x80) {
      return text+skip;
    }

    unsigned char c3 = *(unsigned char*)(text+3);
    if ((c3&0xc0)!=0x80) {
      return text+skip;
    }

    *cp = ((c&0x07)<<18)|((c1&0x7f)<<12)|((c2&0x7f)<<6)|((c3&0x7f)<<0);
    return text+4;
  }

  return text+skip;
}

char* utf8_encode(int cp, char* text, size_t size) {
  if (cp<0x80) {
    if (size<1) {
      return text;
    }

    *(unsigned char*)text++ = (unsigned char)cp;
    return text;
  } else if (cp<0x800) {
    if (size<2) {
      return text;
    }

    *(unsigned char*)text++ = 0xc0+(unsigned char)(cp>>6);
    *(unsigned char*)text++ = 0x80+(unsigned char)(cp&0x3f);
    return text;
  } else if (cp<0x10000) {
    if (size<3) {
      return text;
    }

    *(unsigned char*)text++ = 0xe0+(unsigned char)(cp>>12);
    *(unsigned char*)text++ = 0x80+(unsigned char)((cp>>6)&0x3f);
    *(unsigned char*)text++ = 0x80+(unsigned char)(cp&0x3f);
    return text;
  } else if (cp<0x101000) {
    if (size<4) {
      return text;
    }

    *(unsigned char*)text++ = 0xf0+(unsigned char)(cp>>18);
    *(unsigned char*)text++ = 0x80+(unsigned char)((cp>>12)&0x3f);
    *(unsigned char*)text++ = 0x80+(unsigned char)((cp>>6)&0x3f);
    *(unsigned char*)text++ = 0x80+(unsigned char)(cp&0x3f);
    return text;
  }

  return text;
}

const char* utf8_next(const char* text, size_t size, size_t skip) {
  if (size==0) {
    return text;
  }

  unsigned char c = *(unsigned char*)text;
  if ((c&0x80)==0) {
    return text+1;
  }

  if ((c&0xe0)==0xc0 && size>1) {
    if ((c&0x1f)<2) {
      return text+skip;
    }

    unsigned char c1 = *(unsigned char*)(text+1);
    if ((c1&0xc0)!=0x80) {
      return text+skip;
    }

    return text+2;
  } else if ((c&0xf0)==0xe0 && size>2) {
    unsigned char c1 = *(unsigned char*)(text+1);
    if ((c1&0xc0)!=0x80) {
      return text+skip;
    }

    if (c==0xe0 && (c1<0xa0 || c1>=0xc0)) {
      return text+skip;
    }

    if (c==0xed && (c1<0x80 || c1>=0xa0)) {
      return text+skip;
    }

    unsigned char c2 = *(unsigned char*)(text+2);
    if ((c2&0xc0)!=0x80) {
      return text+skip;
    }

    return text+3;
  } else if ((c&0xf8)==0xf0 && size>3) {
    if (c>0xf4) {
      return text+skip;
    }

    unsigned char c1 = *(unsigned char*)(text+1);
    if ((c1&0xc0)!=0x80) {
      return text+skip;
    }

    if (c==0xf0 && (c1<0x90 || c1>=0xc0)) {
      return text+skip;
    }
    if (c==0xf4 && (c1<0x80 || c1>=0x90)) {
      return text+skip;
    }

    unsigned char c2 = *((unsigned char*)text+2);
    if ((c2&0xc0)!=0x80) {
      return text+skip;
    }

    unsigned char c3 = *((unsigned char*)text+3);
    if ((c3&0xc0)!=0x80) {
      return text+skip;
    }

    return text+4;
  }

  return text+skip;
}

size_t utf8_strnlen(const char* text, size_t size) {
  const char* end = text+size;
  size_t length = 0;
  while (text!=end) {
    text = utf8_next(text, end-text, 1);
    length++;
  }

  return length;
}

size_t utf8_strlen(const char* text) {
  size_t length = 0;
  while (*text) {
    text = utf8_next(text, ~0, 1);
    length++;
  }

  return length;
}

const char* utf8_seek(const char* text, size_t pos) {
  while (pos!=0) {
    text = utf8_next(text, ~0, 1);
    pos--;
  }

  return text;
}
