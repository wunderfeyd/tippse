#ifndef TIPPSE_UNICODE_H
#define TIPPSE_UNICODE_H

#include <stdlib.h>
#include "types.h"

#define UNICODE_CODEPOINT_MAX 0x110000
#define UNICODE_BITFIELD_MAX ((UNICODE_CODEPOINT_MAX/sizeof(unsigned int))+1)

#include "encoding.h"
#include "encoding/utf8.h"
#include "trie.h"

struct unicode_transform_node {
  size_t length;
  codepoint_t cp[8];
};

void unicode_init(void);
void unicode_free(void);
void unicode_decode_transform(uint8_t* data, struct trie** forward, struct trie** reverse);
void unicode_decode_transform_append(struct trie* forward, size_t froms, codepoint_t* from, size_t tos, codepoint_t* to);
void unicode_decode_rle(unsigned int* table, uint16_t* rle);
void unicode_update_combining_mark(codepoint_t codepoint);
int unicode_combining_mark(codepoint_t codepoint);
size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, codepoint_t* codepoints, size_t max, size_t* advance, size_t* length);
int unicode_width(codepoint_t* codepoints, size_t max);
void unicode_width_adjust(codepoint_t cp, int width);
struct unicode_transform_node* unicode_upper(struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length);
struct unicode_transform_node* unicode_lower(struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length);
struct unicode_transform_node* unicode_transform(struct trie* transformation, struct encoding_cache* cache, size_t offset, size_t* advance, size_t* length);

#endif /* #ifndef TIPPSE_UNICODE_H */
