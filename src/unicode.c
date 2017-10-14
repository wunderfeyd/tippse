/* Tippse - Unicode helpers - Unicode character combination, (de-)composition and transformations */

// TODO: codepoints are represented as simple int which a) could not work on 16bit int platforms and b) could be negative without a need
// TODO: preformance test if direct comparsion is better than a bit table in this case on supported platforms

#include "unicode.h"

unsigned int unicode_combining_marks[(UNICODE_COMBINE_MAX/sizeof(unsigned int))+1];

// Initialise static tables
void unicode_init(void) {
  memset(&unicode_combining_marks[0], 0, sizeof(unicode_combining_marks));

  int codepoint;
  // combining diacritical marks
  for (codepoint = 0x300; codepoint<0x370; codepoint++) {
    unicode_update_combining_mark(codepoint);
  }

  // combining diacritical marks extended
  for (codepoint = 0x1ab0; codepoint<0x1abf; codepoint++) {
    unicode_update_combining_mark(codepoint);
  }

  // combining diacritical marks supplement
  for (codepoint = 0x1dc0; codepoint<0x1e00; codepoint++) {
    unicode_update_combining_mark(codepoint);
  }

  // combining diacritical marks for symbols
  for (codepoint = 0x20d0; codepoint<0x20f1; codepoint++) {
    unicode_update_combining_mark(codepoint);
  }

  // combining half marks
  for (codepoint = 0xfe20; codepoint<0xfe2e; codepoint++) {
    unicode_update_combining_mark(codepoint);
  }
}

// Mark codepoint as combining in the bit table
void unicode_update_combining_mark(int codepoint) {
  unicode_combining_marks[codepoint/((int)sizeof(unsigned int)*8)] |= 1u<<(codepoint&((int)sizeof(unsigned int)*8-1));
}

// Check if codepoint is marked as combining
inline int unicode_combining_mark(int codepoint) {
  if (codepoint>=0 && codepoint<UNICODE_COMBINE_MAX) {
    return (unicode_combining_marks[codepoint/((int)sizeof(unsigned int)*8)]>>(codepoint&((int)sizeof(unsigned int)*8-1)))&1;
  }

  return 0;
}

// Return contents and length of combining character sequence
size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, int* codepoints, size_t max, size_t* advance, size_t* length) {
  size_t pos = 0;
  size_t read = 0;
  int codepoint = encoding_cache_find_codepoint(cache, offset+read++);
  if (unicode_combining_mark(codepoint)) {
    codepoints[pos++] = 'o';
  }

  codepoints[pos++] = codepoint;
  if (codepoint>0x20) {
    while (pos<max) {
      codepoint = encoding_cache_find_codepoint(cache, offset+read);
      if (unicode_combining_mark(codepoint)) {
        codepoints[pos++] = codepoint;
        read++;
        continue;
      } else if (codepoint==0x200d) { // Zero width joiner
        if (pos+1<max) {
          codepoints[pos++] = codepoint;
          read++;
          codepoints[pos++] = encoding_cache_find_codepoint(cache, offset+read++);
        }
        continue;
      }

      break;
    }
  }

  *advance = read;
  *length = 0;
  for (size_t check = 0; check<read; check++) {
    *length += encoding_cache_find_length(cache, offset+check);
  }

  return pos;
}

// Check visual width of unicode sequence
int unicode_size(int* codepoints, size_t max) {
  if (max<=0) {
    return 1;
  }

  // Check if we have CJK ideographs (which are displayed in two columns each)
  return 1;
}
