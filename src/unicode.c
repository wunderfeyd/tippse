// Tippse - Unicode helpers - Unicode character information, combination, (de-)composition and transformations

#include "unicode.h"
#include "unicode/unicode_widths.h"
#include "unicode/unicode_invisibles.h"
#include "unicode/unicode_marks.h"
#include "unicode/unicode_case_folding.h"
#include "unicode/unicode_normalization.h"
#include "unicode/unicode_letters.h"
#include "unicode/unicode_whitespace.h"
#include "unicode/unicode_digits.h"

#include "encoding.h"
#include "encoding/utf8.h"
#include "trie.h"

unsigned int unicode_marks[UNICODE_BITFIELD_MAX];
unsigned int unicode_invisibles[UNICODE_BITFIELD_MAX];
unsigned int unicode_widths[UNICODE_BITFIELD_MAX];
unsigned int unicode_letters[UNICODE_BITFIELD_MAX];
unsigned int unicode_whitespaces[UNICODE_BITFIELD_MAX];
unsigned int unicode_digits[UNICODE_BITFIELD_MAX];
struct trie* unicode_transform_lower;
struct trie* unicode_transform_upper;
struct trie* unicode_transform_nfd_nfc;
struct trie* unicode_transform_nfc_nfd;

// Initialise static tables
void unicode_init(void) {
  unicode_decode_rle(&unicode_marks[0], &unicode_marks_rle[0]);
  unicode_decode_rle(&unicode_invisibles[0], &unicode_invisibles_rle[0]);
  unicode_decode_rle(&unicode_widths[0], &unicode_widths_rle[0]);
  unicode_decode_rle(&unicode_letters[0], &unicode_letters_rle[0]);
  unicode_decode_rle(&unicode_whitespaces[0], &unicode_whitespace_rle[0]);
  unicode_decode_rle(&unicode_digits[0], &unicode_digits_rle[0]);
  unicode_decode_transform(&unicode_case_folding[0], &unicode_transform_lower, &unicode_transform_upper);
  unicode_decode_transform(&unicode_normalization[0], &unicode_transform_nfc_nfd, &unicode_transform_nfd_nfc);
}

// Free resources
void unicode_free(void) {
  trie_destroy(unicode_transform_lower);
  trie_destroy(unicode_transform_upper);
  trie_destroy(unicode_transform_nfd_nfc);
  trie_destroy(unicode_transform_nfc_nfd);
}

// Initialize static table from utf8 encoded transform commands
// Encoding scheme...
// Head byte
//  (1rrrcccc exact match r = runs c = copy position)
//  (00fffttt delta f = number of input code points t = number of output code points)
//  An exact match repeats one of the previous differences (16 slots)
// Delta stream
//  input code points x utf8 encoded code point
//  output code points x utf8 encoded code point
//  if code point is below 0x20 then its delta to the previous code point at this location is encoded with offset 0x10
// The result is good but not perfect (there seem to be some repetitions in the whole base data set)
void unicode_decode_transform(uint8_t* data, struct trie** forward, struct trie** reverse) {
  *forward = trie_create(sizeof(struct unicode_sequence));
  *reverse = trie_create(sizeof(struct unicode_sequence));
  struct stream stream;
  stream_from_plain(&stream, data, SIZE_T_MAX);

  codepoint_t from_before[8];
  codepoint_t to_before[8];
  for (size_t n = 0; n<8; n++) {
    from_before[n] = 0;
    to_before[n] = 0;
  }

  size_t froms;
  codepoint_t from[8];
  size_t tos;
  codepoint_t to[8];
  struct stream copy[16];
  size_t copies = 0;
  struct stream duplicate;

  while (1) {
    uint8_t head = stream_read_forward(&stream);
    if (head==0) {
      break;
    }

    int exact = (head>>7)&0x1;
    int runs = 0;
    if (exact) {
      duplicate = copy[head&0xf];
      runs = (head>>4)&0x7;
    } else {
      stream_reverse(&stream, 1);
      copy[copies] = stream;
      copies++;
      copies &= 15;
      duplicate = stream;
    }

    while (runs>=0) {
      struct stream ref = duplicate;
      head = stream_read_forward(&ref);
      froms = (size_t)((head>>3)&0x7);
      tos = (size_t)((head>>0)&0x7);

      for (size_t n = 0; n<froms; n++) {
        size_t used = 0;
        from[n] = encoding_utf8_decode(NULL, &ref, &used);
        if (from[n]<0x20) {
          from[n] += from_before[n]-0x10;
        }
        from_before[n] = from[n];
      }

      for (size_t n = 0; n<tos; n++) {
        size_t used = 0;
        to[n] = encoding_utf8_decode(NULL, &ref, &used);
        if (to[n]<0x20) {
          to[n] += to_before[n]-0x10;
        }
        to_before[n] = to[n];
      }

      unicode_decode_transform_append(*forward, froms, &from[0], tos, &to[0]);
      unicode_decode_transform_append(*reverse, tos, &to[0], froms, &from[0]);

      if (!exact && !runs) {
        stream = ref;
      }

      runs--;
    }
  }

  stream_destroy(&stream);
}

// Append transform to trie and assign result
void unicode_decode_transform_append(struct trie* forward, size_t froms, codepoint_t* from, size_t tos, codepoint_t* to) {
  struct trie_node* parent = NULL;
  for (size_t n = 0; n<froms; n++) {
    parent = trie_append_codepoint(forward, parent, from[n], 0);
  }

  if (!parent->end) {
    parent->end = 1;
    struct unicode_sequence* sequence_from = (struct unicode_sequence*)trie_object(parent);
    sequence_from->length = tos;
    for (size_t n = 0; n<tos; n++) {
      sequence_from->cp[n] = to[n];
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

// Adjust visual width with read terminal font capabilities
void unicode_width_adjust(codepoint_t cp, int width) {
  unicode_bitfield_set(&unicode_invisibles[0], cp, (width==0)?1:0);
  unicode_bitfield_set(&unicode_widths[0], cp, (width==2)?1:0);
}

// Transform to uppercase
struct unicode_sequence* unicode_upper(struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length) {
  return unicode_transform(unicode_transform_upper, cache, offset, advance, length);
}

// Transform to lowercase
struct unicode_sequence* unicode_lower(struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length) {
  return unicode_transform(unicode_transform_upper, cache, offset, advance, length);
}

// Apply transformation if possible
struct unicode_sequence* unicode_transform(struct trie* transformation, struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length) {
  size_t read = 0;
  struct trie_node* parent = NULL;
  while (1) {
    struct encoding_cache_node node = encoding_cache_find_codepoint(cache, offset+read);
    parent = trie_find_codepoint(transformation, parent, node.cp);

    if (!parent) {
      return NULL;
    }

    *length += node.length;
    read++;

    if (parent->end) {
      break;
    }
  }

  *advance = read;
  return (struct unicode_sequence*)trie_object(parent);
}

// Test if codepoint is a letter
int unicode_letter(codepoint_t codepoint) {
  return unicode_bitfield_check(&unicode_letters[0], codepoint);
}

// Test if codepoint is a sigit
int unicode_digit(codepoint_t codepoint) {
  return unicode_bitfield_check(&unicode_digits[0], codepoint);
}

// Test if codepoint is a whitespace
int unicode_whitespace(codepoint_t codepoint) {
  return unicode_bitfield_check(&unicode_whitespaces[0], codepoint);
}
