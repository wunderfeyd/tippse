#ifndef TIPPSE_UNICODE_H
#define TIPPSE_UNICODE_H

#include <stdlib.h>
#include "types.h"
#include "encoding.h"

#define UNICODE_COMBINE_MAX 0x10000
#define UNICODE_CODEPOINT_MAX 0x110000

void unicode_init(void);
void unicode_update_combining_mark(int codepoint);
int unicode_combining_mark(int codepoint);
size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, int* codepoints, size_t max, size_t* advance, size_t* length);
int unicode_size(int* codepoints, size_t max);

#endif /* #ifndef TIPPSE_UNICODE_H */
