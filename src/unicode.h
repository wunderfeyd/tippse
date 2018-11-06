#ifndef TIPPSE_UNICODE_H
#define TIPPSE_UNICODE_H

#include <stdlib.h>
#include "types.h"

#define UNICODE_CODEPOINT_BAD -1
#define UNICODE_CODEPOINT_UNASSIGNED -2
#define UNICODE_CODEPOINT_BOM 0xfeff
#define UNICODE_CODEPOINT_MAX 0x110000
#define UNICODE_BITFIELD_MAX ((UNICODE_CODEPOINT_MAX/sizeof(codepoint_table_t))+1)
#define UNICODE_SEQUENCE_MAX 8

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

codepoint_table_t unicode_marks[UNICODE_BITFIELD_MAX];
codepoint_table_t unicode_invisibles[UNICODE_BITFIELD_MAX];
codepoint_table_t unicode_widths[UNICODE_BITFIELD_MAX];
codepoint_table_t unicode_letters[UNICODE_BITFIELD_MAX];
codepoint_table_t unicode_whitespaces[UNICODE_BITFIELD_MAX];
codepoint_table_t unicode_digits[UNICODE_BITFIELD_MAX];
codepoint_table_t unicode_words[UNICODE_BITFIELD_MAX];

void unicode_init(void);
void unicode_free(void);
void unicode_decode_transform(uint8_t* data, struct trie** forward, struct trie** reverse);
void unicode_decode_transform_append(struct trie* forward, size_t froms, codepoint_t* from, size_t tos, codepoint_t* to);
void unicode_decode_rle(codepoint_table_t* table, uint16_t* rle);
void unicode_update_combining_mark(codepoint_t codepoint);
int unicode_combining_mark(codepoint_t codepoint);
//size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, codepoint_t* codepoints, size_t max, size_t* advance, size_t* length);
void unicode_width_adjust(codepoint_t cp, int width);
struct unicode_sequence* unicode_upper(struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length);
struct unicode_sequence* unicode_lower(struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length);
struct unicode_sequence* unicode_transform(struct trie* transformation, struct unicode_sequencer* sequencer, size_t offset, size_t* advance, size_t* length);

// Check if codepoint is marked
TIPPSE_INLINE int unicode_bitfield_check(const codepoint_table_t* table, codepoint_t codepoint) {
  if (LIKELY(codepoint>=0) && LIKELY(codepoint<UNICODE_CODEPOINT_MAX)) {
    return (int)((table[(size_t)codepoint/(sizeof(codepoint_table_t)*8)]>>((size_t)codepoint&(sizeof(codepoint_table_t)*8-1)))&1);
  }

  return 0;
}

// Mark or reset bit for specific codepoint
TIPPSE_INLINE void unicode_bitfield_set(codepoint_table_t* table, codepoint_t codepoint, int set) {
  if (codepoint>=0 && codepoint<UNICODE_CODEPOINT_MAX) {
    if (!set) {
      table[(size_t)codepoint/(sizeof(codepoint_table_t)*8)] &= ~(((codepoint_table_t)1)<<((size_t)codepoint&(sizeof(codepoint_table_t)*8-1)));
    } else {
      table[(size_t)codepoint/(sizeof(codepoint_table_t)*8)] |= ((codepoint_table_t)1)<<((size_t)codepoint&(sizeof(codepoint_table_t)*8-1));
    }
  }
}

void unicode_bitfield_clear(codepoint_table_t* table);
void unicode_bitfield_combine(codepoint_table_t* table, codepoint_table_t* other);

// Test if codepoint is a letter
TIPPSE_INLINE int unicode_letter(codepoint_t codepoint) {
  return unicode_bitfield_check(&unicode_letters[0], codepoint);
}

// Test if codepoint is a sigit
TIPPSE_INLINE int unicode_digit(codepoint_t codepoint) {
  return unicode_bitfield_check(&unicode_digits[0], codepoint);
}

// Test if codepoint is a whitespace
TIPPSE_INLINE int unicode_whitespace(codepoint_t codepoint) {
  return unicode_bitfield_check(&unicode_whitespaces[0], codepoint);
}

// Test if codepoint is a whitespace
TIPPSE_INLINE int unicode_word(codepoint_t codepoint) {
  return unicode_bitfield_check(&unicode_words[0], codepoint);
}

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
  if (codepoints[0]>0x20) {
    if (UNLIKELY(unicode_bitfield_check(&unicode_marks[0], codepoints[0]))) {
      codepoints[length] = codepoints[length-1];
      codepoints[length-1] = 'o';
      length++;
    }

    while (length<UNICODE_SEQUENCE_MAX) {
      if (unicode_bitfield_check(&unicode_marks[0], base->last_cp)) {
        codepoints[length] = base->last_cp;
        size += base->last_size;
        length++;
        unicode_sequencer_read(base);
        continue;
      } else if (base->last_cp==0x200d) { // Zero width joiner
        if (UNLIKELY(length+1<UNICODE_SEQUENCE_MAX)) {
          codepoints[length] = base->last_cp;
          size += base->last_size;
          length++;
          unicode_sequencer_read(base);
          codepoints[length] = base->last_cp;
          size += base->last_size;
          length++;
          unicode_sequencer_read(base);
        }
        continue;
      }

      break;
    }
  }

  sequence->length = length;
  sequence->size = size;
}

// Return sequence from cache or lookup next sequence
TIPPSE_INLINE struct unicode_sequence* unicode_sequencer_find(struct unicode_sequencer* base, size_t offset) {

  size_t pos = (offset+base->start)&(UNICODE_SEQUENCER_MAX-1);
  if (offset+base->start>=base->end) {
    unicode_sequencer_decode(base, &base->nodes[pos]);
    base->end++;
  }

  return &base->nodes[pos];
}

// Move to next sequence in cache
TIPPSE_INLINE void unicode_sequencer_advance(struct unicode_sequencer* base, size_t advance) {
  base->start += advance;
}

// Since the sequencer can hold some sequences only, the sequencer can be copied and used in forward mode then and we can switch back to the original cached sequence later
TIPPSE_INLINE void unicode_sequencer_alternate(struct unicode_sequencer** base, size_t advance, struct unicode_sequencer* alt_sequencer, struct stream* alt_stream) {
  if (advance==UNICODE_SEQUENCER_MAX-1) {
    unicode_sequencer_clone(alt_sequencer, *base);
    *base = alt_sequencer;
    stream_clone(alt_stream, (*base)->stream);
    (*base)->stream = alt_stream;
  }
}

// Restore state from forward mode
TIPPSE_INLINE void unicode_sequencer_drop_alternate(struct unicode_sequencer* base, size_t advance, struct unicode_sequencer* alt_sequencer, struct stream* alt_stream) {
  if (advance>UNICODE_SEQUENCER_MAX-1) {
    stream_destroy(alt_stream);
  }
}
#endif /* #ifndef TIPPSE_UNICODE_H */
