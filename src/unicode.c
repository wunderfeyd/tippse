// Tippse - Unicode helpers - Unicode character information, combination, (de-)composition and transformations

#include "unicode.h"
#include "unicode_widths.h"
#include "unicode_invisibles.h"
#include "unicode_nonspacing_marks.h"
#include "unicode_spacing_marks.h"

unsigned int unicode_nonspacing_marks[UNICODE_BITFIELD_MAX];
unsigned int unicode_spacing_marks[UNICODE_BITFIELD_MAX];
unsigned int unicode_invisibles[UNICODE_BITFIELD_MAX];
unsigned int unicode_widths[UNICODE_BITFIELD_MAX];

// Initialise static tables
void unicode_init(void) {
  // Expand rle stored tables information
  unicode_decode_rle(&unicode_nonspacing_marks[0], &unicode_nonspacing_marks_rle[0]);
  unicode_decode_rle(&unicode_spacing_marks[0], &unicode_spacing_marks_rle[0]);
  unicode_decode_rle(&unicode_invisibles[0], &unicode_invisibles_rle[0]);
  unicode_decode_rle(&unicode_widths[0], &unicode_widths_rle[0]);
}

// Initialise static table from rle stream
void unicode_decode_rle(unsigned int* table, uint16_t* rle) {
  memset(table, 0, UNICODE_BITFIELD_MAX);
  codepoint_t codepoint = 0;
  while (1) {
    int codes = (int)*rle++;
    if (codes==0) {
      break;
    }

    unsigned int set = (unsigned int)(codes&1);
    codes >>= 1;
    while (codes-->0) {
      if (codepoint<UNICODE_CODEPOINT_MAX) {
        table[codepoint/((int)sizeof(unsigned int)*8)] |= set<<(codepoint&((int)sizeof(unsigned int)*8-1));
      }
      codepoint++;
    }
  }
}

// Check if codepoint is marked
inline int unicode_bitfield_check(unsigned int* table, codepoint_t codepoint) {
  if (codepoint>=0 && codepoint<UNICODE_CODEPOINT_MAX) {
    return (table[codepoint/((int)sizeof(unsigned int)*8)]>>(codepoint&((int)sizeof(unsigned int)*8-1)))&1;
  }

  return 0;
}

// Return contents and length of combining character sequence
size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, codepoint_t* codepoints, size_t max, size_t* advance, size_t* length) {
  size_t pos = 0;
  size_t read = 0;
  codepoint_t codepoint = encoding_cache_find_codepoint(cache, offset+read++);
  if (unicode_bitfield_check(&unicode_nonspacing_marks[0], codepoint) || unicode_bitfield_check(&unicode_spacing_marks[0], codepoint)) {
    codepoints[pos++] = 'o';
  }

  codepoints[pos++] = codepoint;
  if (codepoint>0x20) {
    while (pos<max) {
      codepoint = encoding_cache_find_codepoint(cache, offset+read);
      if (unicode_bitfield_check(&unicode_nonspacing_marks[0], codepoint) || unicode_bitfield_check(&unicode_spacing_marks[0], codepoint)) {
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
int unicode_width(codepoint_t* codepoints, size_t max) {
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
