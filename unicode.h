#ifndef __TIPPSE_UNICODE__
#define __TIPPSE_UNICODE__

#include <stdlib.h>
#include "types.h"
#include "encoding.h"

#define UNICODE_COMBINE_MAX 0x10000

void unicode_init();
void unicode_update_combining_mark(int codepoint);
int unicode_combining_mark(int codepoint);
size_t unicode_read_combined_sequence(struct encoding_cache* cache, size_t offset, int* codepoints, size_t max, size_t* advance, size_t* length);

#endif /* #ifndef __TIPPSE_UNICODE__ */
