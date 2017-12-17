// Tippse - Unicode helpers - Unicode character information, combination, (de-)composition and transformations

#include "unicode.h"
#include "unicode_widths.h"
#include "unicode_invisibles.h"
#include "unicode_nonspacing_marks.h"
#include "unicode_spacing_marks.h"
#include "unicode_case_folding.h"
#include "unicode_normalization.h"

unsigned int unicode_nonspacing_marks[UNICODE_BITFIELD_MAX];
unsigned int unicode_spacing_marks[UNICODE_BITFIELD_MAX];
unsigned int unicode_invisibles[UNICODE_BITFIELD_MAX];
unsigned int unicode_widths[UNICODE_BITFIELD_MAX];
struct trie* unicode_transform_lower;
struct trie* unicode_transform_upper;
struct trie* unicode_transform_nfd_nfc;
struct trie* unicode_transform_nfc_nfd;

// Initialise static tables
void unicode_init(void) {
  // Expand encoded tables
  unicode_decode_rle(&unicode_nonspacing_marks[0], &unicode_nonspacing_marks_rle[0]);
  unicode_decode_rle(&unicode_spacing_marks[0], &unicode_spacing_marks_rle[0]);
  unicode_decode_rle(&unicode_invisibles[0], &unicode_invisibles_rle[0]);
  unicode_decode_rle(&unicode_widths[0], &unicode_widths_rle[0]);
  unicode_decode_transform(&unicode_case_folding[0], &unicode_transform_lower, &unicode_transform_upper);
  unicode_decode_transform(&unicode_normalization[0], &unicode_transform_nfc_nfd, &unicode_transform_nfd_nfc);
}

// Initialise static table from utf8 encoded transform commands
void unicode_decode_transform(uint8_t* data, struct trie** forward, struct trie** reverse) {
  *forward = trie_create(sizeof(struct unicode_transform_node));
  *reverse = trie_create(sizeof(struct unicode_transform_node));
  struct encoding_stream stream;
  encoding_stream_from_plain(&stream, data, SIZE_T_MAX);
  while (1) {
    uint8_t head = encoding_stream_peek(&stream, 0);
    if (head==0) {
      break;
    }

    size_t froms = (head>>4)&0xf;
    size_t tos = (head>>0)&0xf;
    encoding_stream_forward(&stream, 1);

    codepoint_t from[16];
    for (size_t n = 0; n<froms; n++) {
      size_t used = 0;
      from[n] = encoding_utf8_decode(NULL, &stream, &used);
      encoding_stream_forward(&stream, used);
    }

    codepoint_t to[16];
    for (size_t n = 0; n<tos; n++) {
      size_t used = 0;
      to[n] = encoding_utf8_decode(NULL, &stream, &used);
      encoding_stream_forward(&stream, used);
    }

    unicode_decode_transform_append(*forward, froms, &from[0], tos, &to[0]);
    unicode_decode_transform_append(*reverse, tos, &to[0], froms, &from[0]);
  }
}

// Append transform to trie and assign result
void unicode_decode_transform_append(struct trie* forward, size_t froms, codepoint_t* from, size_t tos, codepoint_t* to) {
  struct trie_node* parent = NULL;
  for (size_t n = 0; n<froms; n++) {
    parent = trie_append_codepoint(forward, parent, from[n], 0);
  }

  if (!parent->end) {
    parent->end = 1;
    struct unicode_transform_node* transform_from = (struct unicode_transform_node*)trie_object(parent);
    transform_from->length = tos;
    for (size_t n = 0; n<tos; n++) {
      transform_from->cp[n] = to[n];
    }
  }
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

// Transform to uppercase
struct unicode_transform_node* unicode_upper(struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length) {
  return unicode_transform(unicode_transform_upper, cache, offset, advance, length);
}

// Transform to lowercase
struct unicode_transform_node* unicode_lower(struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length) {
  return unicode_transform(unicode_transform_upper, cache, offset, advance, length);
}

// Apply transformation if possible
struct unicode_transform_node* unicode_transform(struct trie* transformation, struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length) {
  size_t read = 0;
  struct trie_node* parent = NULL;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(cache, offset+read);
    parent = trie_find_codepoint(transformation, parent, cp);

    if (!parent) {
      return NULL;
    }

    *length += encoding_cache_find_length(cache, offset+read);
    read++;

    if (parent->end) {
      break;
    }
  }

  *advance = read;
  return (struct unicode_transform_node*)trie_object(parent);
}
