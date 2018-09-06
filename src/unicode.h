#ifndef TIPPSE_UNICODE_H
#define TIPPSE_UNICODE_H

#include <stdlib.h>
#include "types.h"

#include "encoding.h"

#define UNICODE_CODEPOINT_MAX 0x110000
#define UNICODE_BITFIELD_MAX ((UNICODE_CODEPOINT_MAX/sizeof(unsigned int))+1)

struct unicode_transform_node {
  size_t length;        // Number of codepoint
  codepoint_t cp[8];    // Codepoints
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
TIPPSE_INLINE int unicode_bitfield_check(unsigned int* table, codepoint_t codepoint) {
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
TIPPSE_INLINE int unicode_width(codepoint_t* codepoints, size_t max) {
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
TIPPSE_INLINE size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, codepoint_t* codepoints, size_t max, size_t* advance, size_t* length) {
  size_t pos = 0;
  size_t read = 0;
  codepoint_t codepoint = encoding_cache_find_codepoint(cache, offset+read++).cp;
  if (unicode_bitfield_check(&unicode_marks[0], codepoint)) {
    codepoints[pos++] = 'o';
  }

  codepoints[pos++] = codepoint;
  if (codepoint>0x20) {
    while (pos<max) {
      codepoint = encoding_cache_find_codepoint(cache, offset+read).cp;
      if (unicode_bitfield_check(&unicode_marks[0], codepoint)) {
        codepoints[pos++] = codepoint;
        read++;
        continue;
      } else if (codepoint==0x200d) { // Zero width joiner
        if (pos+1<max) {
          codepoints[pos++] = codepoint;
          read++;
          codepoints[pos++] = encoding_cache_find_codepoint(cache, offset+read++).cp;
        }
        continue;
      }

      break;
    }
  }

  *advance = read;
  *length = 0;
  for (size_t check = 0; check<read; check++) {
    *length += encoding_cache_find_codepoint(cache, offset+check).length;
  }

  return pos;
}

#endif /* #ifndef TIPPSE_UNICODE_H */
