#ifndef TIPPSE_UNICODE_H
#define TIPPSE_UNICODE_H

#include <stdlib.h>
#include "types.h"

#define UNICODE_CODEPOINT_BOM 0xfeff
#define UNICODE_CODEPOINT_MAX 0x110000
#define UNICODE_CODEPOINT_BAD UNICODE_CODEPOINT_MAX+1
#define UNICODE_CODEPOINT_UNASSIGNED UNICODE_CODEPOINT_MAX+2
#define UNICODE_CODEPOINT_MAX_BAD 0x110000+4

#define UNICODE_HINT_MAX (UNICODE_CODEPOINT_MAX_BAD+1)
#define UNICODE_SEQUENCE_MAX 8

#define UNICODE_HINT_MASK_WIDTH_SINGLE 0x01
#define UNICODE_HINT_MASK_WIDTH_DOUBLE 0x02
#define UNICODE_HINT_MASK_WIDTH (UNICODE_HINT_MASK_WIDTH_SINGLE|UNICODE_HINT_MASK_WIDTH_DOUBLE)
#define UNICODE_HINT_MASK_JOINER 0x08
#define UNICODE_HINT_MASK_LETTERS 0x10
#define UNICODE_HINT_MASK_WHITESPACES 0x20
#define UNICODE_HINT_MASK_DIGITS 0x40
#define UNICODE_HINT_MASK_WORDS 0x80

struct unicode_sequence {
  size_t length;                            // Number of codepoints in cp[]
  size_t size;                              // Length in bytes
  codepoint_t cp[UNICODE_SEQUENCE_MAX];     // Codepoints
};

#define UNICODE_SEQUENCER_MAX 32

struct unicode_sequencer {
  codepoint_t last_cp;                  // last read codepoint
  size_t last_size;                     // last read codepoint size
  size_t start;                         // current position in cache
  size_t end;                           // last position in cache
  struct encoding* encoding;            // encoding used for stream
  struct stream* stream;                // input stream
  struct unicode_sequence nodes[UNICODE_SEQUENCER_MAX];  // cache with lengths of codepoints
};

#include "encoding.h"

#ifdef TIPPSE_UNICODE_UNIT
codepoint_table_t unicode_hints[UNICODE_HINT_MAX];
#else
extern codepoint_table_t unicode_hints[UNICODE_HINT_MAX];
#endif

void unicode_init(void);
void unicode_free(void);
void unicode_decode_transform(uint8_t* data, struct trie** forward, struct trie** reverse);
void unicode_decode_transform_stream(size_t count, struct stream* ref, codepoint_t* sum, codepoint_t* last);
void unicode_decode_transform_append(struct trie* forward, size_t froms, codepoint_t* from, size_t tos, codepoint_t* to);
void unicode_decode_rle(uint8_t* rle, codepoint_table_t mask);
void unicode_update_combining_mark(codepoint_t codepoint);
int unicode_combining_mark(codepoint_t codepoint);
//size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, codepoint_t* codepoints, size_t max, size_t* advance, size_t* length);
void unicode_width_adjust(codepoint_t cp, int width);
struct unicode_sequence* unicode_upper(struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length);
struct unicode_sequence* unicode_lower(struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length);
struct unicode_sequence* unicode_transform(struct trie* transformation, struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length);

// Check if codepoint is marked
TIPPSE_INLINE codepoint_table_t unicode_hint_check(codepoint_t codepoint, codepoint_table_t mask) {
  return unicode_hints[codepoint]&mask;
}

// Mark or reset bit for specific codepoint
TIPPSE_INLINE void unicode_hint_set(codepoint_t codepoint, codepoint_table_t mask, codepoint_table_t combine) {
  if (codepoint<UNICODE_CODEPOINT_MAX) {
    unicode_hints[codepoint] &= ~mask;
    unicode_hints[codepoint] |= combine;
  }
}

void unicode_hint_clear(codepoint_table_t mask);
void unicode_hint_combine(codepoint_table_t mask_search, codepoint_table_t mask);

// Test if codepoint is a letter
TIPPSE_INLINE bool_t unicode_letter(codepoint_t codepoint) {
  return unicode_hint_check(codepoint, UNICODE_HINT_MASK_LETTERS)?1:0;
}

// Test if codepoint is a digit
TIPPSE_INLINE bool_t unicode_digit(codepoint_t codepoint) {
  return unicode_hint_check(codepoint, UNICODE_HINT_MASK_DIGITS)?1:0;
}

// Test if codepoint is a whitespace
TIPPSE_INLINE bool_t unicode_whitespace(codepoint_t codepoint) {
  return unicode_hint_check(codepoint, UNICODE_HINT_MASK_WHITESPACES)?1:0;
}

// Test if codepoint belongs to a word
TIPPSE_INLINE bool_t unicode_word(codepoint_t codepoint) {
  return unicode_hint_check(codepoint, UNICODE_HINT_MASK_WORDS)?1:0;
}

// Test if codepoint is joiner or mark
TIPPSE_INLINE codepoint_table_t unicode_joiner(const codepoint_t codepoint) {
  return unicode_hint_check(codepoint, UNICODE_HINT_MASK_JOINER);
}

// Check visual width of unicode sequence
TIPPSE_INLINE codepoint_table_t unicode_width(const codepoint_t* codepoints, size_t max) {
  if (max<=0) {
    return 1;
  }

  return unicode_hint_check(codepoints[0], UNICODE_HINT_MASK_WIDTH);
}

void unicode_sequencer_clear(struct unicode_sequencer* base, struct encoding* encoding, struct stream* stream);
void unicode_sequencer_clone(struct unicode_sequencer* dst, struct unicode_sequencer* src);

// Read next codepoint from stream
TIPPSE_INLINE void unicode_sequencer_read(struct unicode_sequencer* base) {
  base->last_cp = (*base->encoding->decode)(base->encoding, base->stream, &base->last_size);
}

// Decode sequence (combined unicodes with marks or joiner)
TIPPSE_INLINE void unicode_sequencer_decode(struct unicode_sequencer* base, struct unicode_sequence* sequence) {
  codepoint_t* codepoints = &sequence->cp[0];
  codepoints[0] = base->last_cp;
  size_t size = base->last_size;
  unicode_sequencer_read(base);

  size_t length = 1;
  while (UNLIKELY(unicode_joiner(base->last_cp)) && length<UNICODE_SEQUENCE_MAX-1) {
    if (UNLIKELY(base->last_cp==0x200d)) { // Zero width joiner
      codepoints[length] = base->last_cp;
      size += base->last_size;
      length++;
      unicode_sequencer_read(base);
    }

    codepoints[length] = base->last_cp;
    size += base->last_size;
    length++;
    unicode_sequencer_read(base);
  }

  sequence->length = length;
  sequence->size = size;
}

// Return sequence from cache or lookup next sequence
TIPPSE_INLINE struct unicode_sequence* unicode_sequencer_find(struct unicode_sequencer* base, size_t offset) {
  if (offset+base->start>=base->end) {
    if (base->start==base->end) {
      base->start = base->end = 0;
    }

    unicode_sequencer_decode(base, &base->nodes[(base->end++)&(UNICODE_SEQUENCER_MAX-1)]);
  }

  return &base->nodes[(offset+base->start)&(UNICODE_SEQUENCER_MAX-1)];

}

// Move to next sequence in cache
TIPPSE_INLINE void unicode_sequencer_advance(struct unicode_sequencer* base, size_t advance) {
  base->start += advance;
}

// Since the sequencer can hold some sequences only, the sequencer can be copied and used in forward mode then and we can switch back to the original cached sequence later
TIPPSE_INLINE bool_t unicode_sequencer_alternate(struct unicode_sequencer** base, size_t advance, struct unicode_sequencer* alt_sequencer, struct stream* alt_stream) {
  if (advance==UNICODE_SEQUENCER_MAX-1) {
    unicode_sequencer_clone(alt_sequencer, *base);
    *base = alt_sequencer;
    stream_clone(alt_stream, (*base)->stream);
    (*base)->stream = alt_stream;
    return 1;
  }

  return 0;
}

// Restore state from forward mode
TIPPSE_INLINE void unicode_sequencer_drop_alternate(struct unicode_sequencer* base) {
  stream_destroy(base->stream);
}
#endif /* #ifndef TIPPSE_UNICODE_H */
