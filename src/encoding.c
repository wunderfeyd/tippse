// Tippse - Encoding - Base for different encoding schemes (characters <-> bytes)

#include "encoding.h"

// Reset code point cache
void encoding_cache_clear(struct encoding_cache* base, struct encoding* encoding, struct stream* stream) {
  base->start = 0;
  base->end = 0;
  base->encoding = encoding;
  base->stream = stream;
}

// Fill code point cache
void encoding_cache_buffer(struct encoding_cache* base, size_t offset) {
  while (base->end-base->start<=offset) {
    size_t pos = base->end%ENCODING_CACHE_SIZE;
    base->codepoints[pos] = (*base->encoding->decode)(base->encoding, base->stream, &base->lengths[pos]);
    base->end++;
  }
}

// Invert lookup table for unicode and codepage translation
uint16_t* encoding_reverse_table(uint16_t* table, size_t length, size_t max) {
  uint16_t* output = (uint16_t*)malloc(sizeof(uint16_t)*max);
  for (size_t n = 0; n<max; n++) {
    output[n] = (uint16_t)~0u;
  }

  for (size_t n = 0; n<length; n++) {
    size_t set = (size_t)table[n];
    if (set<max) {
      output[set] = (uint16_t)n;
    }
  }

  return output;
}
