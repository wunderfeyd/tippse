// Tippse - Unicode helpers - Unicode character information, combination, (de-)composition and transformations

#define TIPPSE_UNICODE_UNIT

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

struct trie* unicode_transform_lower;
struct trie* unicode_transform_upper;
struct trie* unicode_transform_nfd_nfc;
struct trie* unicode_transform_nfc_nfd;

// Initialise static tables
void unicode_init(void) {
  unicode_hint_clear(0);
  unicode_decode_rle(&unicode_invisibles_rle[0], UNICODE_HINT_MASK_WIDTH_SINGLE);
  unicode_decode_rle(&unicode_widths_rle[0], UNICODE_HINT_MASK_WIDTH_DOUBLE);
  unicode_decode_rle(&unicode_marks_rle[0], UNICODE_HINT_MASK_JOINER);
  unicode_decode_rle(&unicode_letters_rle[0], UNICODE_HINT_MASK_LETTERS);
  unicode_decode_rle(&unicode_whitespace_rle[0], UNICODE_HINT_MASK_WHITESPACES);
  unicode_decode_rle(&unicode_digits_rle[0], UNICODE_HINT_MASK_DIGITS);
  for (size_t n = 0; n<UNICODE_HINT_MAX; n++) {
    if (unicode_hints[n]&UNICODE_HINT_MASK_WIDTH_SINGLE) {
      unicode_hints[n] &= ~UNICODE_HINT_MASK_WIDTH;
    } else if (unicode_hints[n]&UNICODE_HINT_MASK_WIDTH_DOUBLE) {
      unicode_hints[n] &= ~UNICODE_HINT_MASK_WIDTH_SINGLE;
    } else {
      unicode_hints[n] |= UNICODE_HINT_MASK_WIDTH_SINGLE;
    }
  }

  unicode_hint_combine(UNICODE_HINT_MASK_LETTERS, UNICODE_HINT_MASK_WORDS);
  unicode_hint_combine(UNICODE_HINT_MASK_JOINER, UNICODE_HINT_MASK_WORDS);
  unicode_hint_combine(UNICODE_HINT_MASK_DIGITS, UNICODE_HINT_MASK_WORDS);
  unicode_hint_set('_', UNICODE_HINT_MASK_WORDS, UNICODE_HINT_MASK_WORDS);

  unicode_hint_set(0x200d, UNICODE_HINT_MASK_JOINER, UNICODE_HINT_MASK_JOINER);

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

// Decode variable length integer for rle and lz codes
codepoint_t unicode_decode_var(uint8_t** data) {
  codepoint_t cp = 0;
  int cpm = 0;
  while (1) {
    uint8_t c = *((*data)++);
    cp |= ((codepoint_t)c&0x7f)<<cpm;
    cpm += 7;
    if ((c&0x80)==0) {
      break;
    }
  }

  return cp;
}

// Initialize static table from utf8 encoded transform commands
// Simple LZ variant based on variable length encoding and with delta to previous string
void unicode_decode_transform(uint8_t* data, struct trie** forward, struct trie** reverse) {
  *forward = trie_create(sizeof(struct unicode_sequence));
  *reverse = trie_create(sizeof(struct unicode_sequence));

  codepoint_t* history = (codepoint_t*)malloc(128000*sizeof(codepoint_t));
  size_t history_pos = 0;

  uint8_t* read = data;
  while (1) {
    codepoint_t cp = unicode_decode_var(&read);
    if (cp==0) {
      break;
    }

    if (cp&1) {
      size_t load = (cp>>1);
      size_t distance = unicode_decode_var(&read);
      for (size_t copy = 0; copy<load; copy++) {
         history[history_pos] = history[history_pos-distance];
         history_pos++;
      }
    } else{
      history[history_pos++] = cp>>1;
    }
  }

  size_t froms = 0;
  size_t tos = 0;
  size_t state = 0;
  codepoint_t before[16];
  for (size_t n = 0; n<16; n++) {
    before[n] = 0;
  }

  int codes = 0;
  for (size_t n = 0; n<history_pos; n++) {
    codepoint_t cp = history[n];
    if (cp!=1) {
      int pos = cp&1;
      cp >>= 1;
      if (pos) {
        before[state] += cp-1;
      } else {
        before[state] -= cp;
      }

      state++;
    } else {
      if (state<8) {
        froms = state;
        state = 8;
      } else {
        tos = state-8;

        unicode_decode_transform_append(*forward, froms, &before[0], tos, &before[8]);
        unicode_decode_transform_append(*reverse, tos, &before[8], froms, &before[0]);
        codes++;
        state = 0;
      }
    }
  }

  free(history);
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
void unicode_decode_rle(uint8_t* rle, codepoint_table_t mask) {
  uint8_t* read = rle;
  codepoint_t codepoint = 0;
  while (1) {
    int codes = unicode_decode_var(&read);
    if (codes==0) {
      break;
    }

    codepoint_table_t set = (codepoint_table_t)(codes&1);
    codes >>= 1;
    while (codes-->0) {
      if (codepoint<UNICODE_CODEPOINT_MAX && set) {
        unicode_hints[codepoint] |= mask;
      }
      codepoint++;
    }
  }
}

// Adjust visual width with read terminal font capabilities
void unicode_width_adjust(codepoint_t cp, int width) {
  unicode_hint_set(cp, UNICODE_HINT_MASK_WIDTH, width);
}

// Transform to uppercase
struct unicode_sequence* unicode_upper(struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length) {
  return unicode_transform(unicode_transform_upper, sequencer, offset, advance, length);
}

// Transform to lowercase
struct unicode_sequence* unicode_lower(struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length) {
  return unicode_transform(unicode_transform_upper, sequencer, offset, advance, length);
}

// Apply transformation if possible
struct unicode_sequence* unicode_transform(struct trie* transformation, struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length) {
  size_t read = 0;
  struct trie_node* parent = NULL;
  size_t best_read = 0;
  struct trie_node* best_parent = NULL;
  size_t size = 0;
  size_t best_size = 0;
  while (1) {
    struct unicode_sequence* sequence = unicode_sequencer_find(sequencer, offset+read);
    for (size_t n = 0; n<sequence->length; n++) {
      parent = trie_find_codepoint(transformation, parent, sequence->cp[n]);

      if (!parent) {
        break;
      }
    }

    if (!parent) {
      break;
    }

    size += sequence->size;
    read++;

    if (parent->end) {
      best_parent = parent;
      best_read = read;
      best_size = size;
    }
  }

  if (!best_parent) {
    return NULL;
  }

  *length += best_size;
  *advance = best_read;
  return (struct unicode_sequence*)trie_object(best_parent);
}

// Reset bitfield
void unicode_hint_clear(codepoint_table_t mask) {
  for (size_t n = 0; n<UNICODE_HINT_MAX; n++) {
    unicode_hints[n] &= mask;
  }
}

// Enable codepoints from the other table
void unicode_hint_combine(codepoint_table_t mask_search, codepoint_table_t mask) {
  for (size_t n = 0; n<UNICODE_HINT_MAX; n++) {
    if (unicode_hints[n]&mask_search) {
      unicode_hints[n] |= mask;
    }
  }
}

// Unicode sequence decoder from stream and encoding
void unicode_sequencer_clear(struct unicode_sequencer* base, struct encoding* encoding, struct stream* stream) {
  base->start = 0;
  base->end = 0;
  base->encoding = encoding;
  base->stream = stream;
  unicode_sequencer_read(base);
}

// Clone unicode sequencer into another datastructure
void unicode_sequencer_clone(struct unicode_sequencer* dst, struct unicode_sequencer* src) {
  dst->start = src->start;
  dst->end = src->end;
  dst->encoding = src->encoding;
  dst->stream = src->stream;
  dst->last_cp = src->last_cp;
  dst->last_size = src->last_size;
}
