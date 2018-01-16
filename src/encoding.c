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

// Transform stream to different encoding
struct range_tree_node* encoding_transform_stream(struct stream* stream, struct encoding* from, struct encoding* to) {
  uint8_t recoded[1024];
  size_t recode = 0;

  struct encoding_cache cache;
  encoding_cache_clear(&cache, from, stream);
  struct range_tree_node* root = NULL;
  while (1) {
    if (stream_end(stream) || recode>512) {
      root = range_tree_insert_split(root, root?root->length:0, &recoded[0], recode, 0);
      recode = 0;
      if (stream_end(stream)) {
        break;
      }
    }

    codepoint_t cp = encoding_cache_find_codepoint(&cache, 0);
    encoding_cache_advance(&cache, 1);
    if (cp==-1) {
      continue;
    }

    recode += to->encode(to, cp, &recoded[recode], 8);
  }

  return root;
}

// Transform plain text to different encoding
uint8_t* encoding_transform_plain(const uint8_t* buffer, size_t length, struct encoding* from, struct encoding* to) {
  struct stream stream;
  stream_from_plain(&stream, buffer, length);
  struct range_tree_node* root = encoding_transform_stream(&stream, from, to);
  uint8_t* raw = range_tree_raw(root, 0, root?root->length:0);
  range_tree_destroy(root, NULL);
  return raw;
}
