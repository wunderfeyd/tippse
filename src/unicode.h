#ifndef TIPPSE_UNICODE_H
#define TIPPSE_UNICODE_H

#include <stdlib.h>
#include "types.h"

#define UNICODE_CODEPOINT_BAD -1
#define UNICODE_CODEPOINT_BOM 0xfeff
#define UNICODE_CODEPOINT_MAX 0x110000
#define UNICODE_BITFIELD_MAX ((UNICODE_CODEPOINT_MAX/sizeof(unsigned int))+1)
#define UNICODE_TRANSFORM_MAX 8

#include "encoding.h"

struct unicode_transform_node {
  size_t length;                            // Number of codepoints in cp[]
  codepoint_t cp[UNICODE_TRANSFORM_MAX];    // Codepoints
  size_t advance;                           // Number of codepoints read
  size_t size;                              // Length in bytes
};

void unicode_init(void);
void unicode_free(void);
void unicode_decode_transform(uint8_t* data, struct trie** forward, struct trie** reverse);
void unicode_decode_transform_append(struct trie* forward, size_t froms, codepoint_t* from, size_t tos, codepoint_t* to);
void unicode_decode_rle(unsigned int* table, uint16_t* rle);
void unicode_update_combining_mark(codepoint_t codepoint);
int unicode_combining_mark(codepoint_t codepoint);
//size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, codepoint_t* codepoints, size_t max, size_t* advance, size_t* length);
void unicode_width_adjust(codepoint_t cp, int width);
int unicode_letter(codepoint_t codepoint);
int unicode_digit(codepoint_t codepoint);
int unicode_whitespace(codepoint_t codepoint);
struct unicode_transform_node* unicode_upper(struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length);
struct unicode_transform_node* unicode_lower(struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length);
struct unicode_transform_node* unicode_transform(struct trie* transformation, struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length);

// Check if codepoint is marked
TIPPSE_INLINE int unicode_bitfield_check(const unsigned int* table, codepoint_t codepoint) {
  if (codepoint>=0 && codepoint<UNICODE_CODEPOINT_MAX) {
    return (int)((table[(size_t)codepoint/(sizeof(unsigned int)*8)]>>((size_t)codepoint&(sizeof(unsigned int)*8-1)))&1);
  }

  return 0;
}

// Mark or reset bit for specific codepoint
TIPPSE_INLINE void unicode_bitfield_set(unsigned int* table, codepoint_t codepoint, int set) {
  if (codepoint>=0 && codepoint<UNICODE_CODEPOINT_MAX) {
    if (!set) {
      table[(size_t)codepoint/(sizeof(unsigned int)*8)] &= ~(((unsigned int)1)<<((size_t)codepoint&(sizeof(unsigned int)*8-1)));
    } else {
      table[(size_t)codepoint/(sizeof(unsigned int)*8)] |= ((unsigned int)1)<<((size_t)codepoint&(sizeof(unsigned int)*8-1));
    }
  }
}

unsigned int unicode_marks[UNICODE_BITFIELD_MAX];
unsigned int unicode_invisibles[UNICODE_BITFIELD_MAX];
unsigned int unicode_widths[UNICODE_BITFIELD_MAX];
unsigned int unicode_letters[UNICODE_BITFIELD_MAX];
unsigned int unicode_whitespaces[UNICODE_BITFIELD_MAX];
unsigned int unicode_digits[UNICODE_BITFIELD_MAX];

// Check visual width of unicode sequence
TIPPSE_INLINE int unicode_width(const codepoint_t* codepoints, size_t max) {
  if (max<=0) {
    return 1;
  }

  // Return width zero if character is invisible
  if (unicode_bitfield_check(&unicode_invisibles[0], codepoints[0])) {
    return 0;
  }

  // Check if we have CJK ideographs (which are displayed in two columns each)
  return unicode_bitfield_check(&unicode_widths[0], codepoints[0])+1;
}

// Return contents and length of combining character sequence
TIPPSE_INLINE void unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, struct unicode_transform_node* transform) {
  codepoint_t* codepoints = &transform->cp[0];
  codepoints[0] = encoding_cache_find_codepoint(cache, offset).cp;
  size_t length = 1;
  size_t advance = 1;
  if (codepoints[0]>0x20) {
    if (unicode_bitfield_check(&unicode_marks[0], codepoints[0])) {
      codepoints[length] = codepoints[length-1];
      codepoints[length-1] = 'o';
      length++;
    }

    while (length<UNICODE_TRANSFORM_MAX) {
      codepoints[length] = encoding_cache_find_codepoint(cache, offset+advance).cp;
      if (unicode_bitfield_check(&unicode_marks[0], codepoints[length])) {
        length++;
        advance++;
        continue;
      } else if (codepoints[length]==0x200d) { // Zero width joiner
        if (length+1<UNICODE_TRANSFORM_MAX) {
          length++;
          advance++;
          codepoints[length++] = encoding_cache_find_codepoint(cache, offset+advance++).cp;
        }
        continue;
      }

      break;
    }
  }

  transform->length = length;
  transform->advance = advance;
}

TIPPSE_INLINE void unicode_read_combined_sequence_size(struct encoding_cache* cache, size_t offset, struct unicode_transform_node* transform) {
  size_t size = 0;
  for (size_t check = 0; check<transform->advance; check++) {
    size += encoding_cache_find_codepoint(cache, offset+check).length;
  }
  transform->size = size;
}

#endif /* #ifndef TIPPSE_UNICODE_H */
