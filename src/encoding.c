// Tippse - Encoding - Base for different encoding schemes (characters <-> bytes)

#include "encoding.h"
#include "encoding/utf8.h"
#include "encoding/native.h"

struct encoding* encoding_native_base = NULL;
struct encoding* encoding_utf8_base = NULL;

// Initialize static encodings
void encoding_init(void) {
  encoding_native_base = encoding_native_create();
  encoding_utf8_base = encoding_utf8_create();
}

// Free static encodings
void encoding_free(void) {
  encoding_utf8_base->destroy(encoding_utf8_base);
  encoding_native_base->destroy(encoding_native_base);
}

// Get native encoding
struct encoding* encoding_native_static(void) {
  return encoding_native_base;
}

// Get UTF-8 encoding
struct encoding* encoding_utf8_static(void) {
  return encoding_utf8_base;
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

// sequence stream to different encoding
struct range_tree* encoding_transform_stream(struct stream* stream, struct encoding* from, struct encoding* to, file_offset_t max) {
  uint8_t recoded[1024];
  size_t recode = 0;

  struct range_tree* root = range_tree_create();
  while (1) {
    if (stream_end(stream) || recode>512 || max==0) {
      root->root = range_tree_node_insert_split(root->root, root, range_tree_node_length(root->root), &recoded[0], recode, 0);
      recode = 0;
      if (stream_end(stream) || max==0) {
        break;
      }
    }

    size_t length;
    codepoint_t cp = from->decode(from, stream, &length);
    if (cp==UNICODE_CODEPOINT_BAD) {
      continue;
    }

    if (max>=(file_offset_t)length) {
      recode += to->encode(to, cp, &recoded[recode], sizeof(recoded)-recode);
      max -= length;
    } else {
      max = 0;
    }
  }

  return root;
}

// sequence plain text to different encoding
uint8_t* encoding_transform_plain(const uint8_t* buffer, size_t length, struct encoding* from, struct encoding* to) {
  struct stream stream;
  stream_from_plain(&stream, buffer, length);
  struct range_tree* root = encoding_transform_stream(&stream, from, to, FILE_OFFSET_T_MAX);
  uint8_t* raw = range_tree_node_raw(root->root, 0, root->root?root->root->length:0);
  range_tree_destroy(root, NULL);
  stream_destroy(&stream);
  return raw;
}

// sequence pages to different encoding
struct range_tree* encoding_transform_page(struct range_tree_node* root, file_offset_t start, file_offset_t length, struct encoding* from, struct encoding* to) {
  file_offset_t diff;
  struct range_tree_node* node = range_tree_node_find_offset(root, start, &diff);
  if (!node) {
    return NULL;
  }

  struct stream stream;
  stream_from_page(&stream, node, diff);
  struct range_tree* transform = encoding_transform_stream(&stream, from, to, length);
  stream_destroy(&stream);
  return transform;
}

// Check string length (within maximum)
size_t encoding_strnlen(struct encoding* base, struct stream* stream, size_t size) {
  return encoding_strnlen_based(base, base->next, stream, size);
}

size_t encoding_strnlen_based(struct encoding* base, size_t (*next)(struct encoding*, struct stream*), struct stream* stream, size_t size) {
  size_t length = 0;
  while (size!=0) {
    size_t skip = (*next)(base, stream);
    if (skip>size) {
      size = 0;
    } else {
      size -= skip;
    }

    length++;
  }

  return length;
}

// Check string length (to NUL terminator)
size_t encoding_strlen(struct encoding* base, struct stream* stream) {
  return encoding_strlen_based(base, base->decode, stream);
}

size_t encoding_strlen_based(struct encoding* base, codepoint_t (*decode)(struct encoding*, struct stream*, size_t*), struct stream* stream) {
  size_t length = 0;
  while (1) {
    size_t skip;
    codepoint_t cp = encoding_utf8_decode(base, stream, &skip);
    if (cp==0) {
      break;
    }

    length++;
  }

  return length;
}

// Skip to character position
size_t encoding_seek(struct encoding* base, struct stream* stream, size_t pos) {
  return encoding_seek_based(base, base->next, stream, pos);
}

size_t encoding_seek_based(struct encoding* base, size_t (*next)(struct encoding*, struct stream*), struct stream* stream, size_t pos) {
  size_t current = 0;
  while (pos!=0) {
    size_t skip = encoding_utf8_next(base, stream);
    current += skip;
    pos--;
  }

  return current;
}
