#ifndef TIPPSE_CLIPBOARD_H
#define TIPPSE_CLIPBOARD_H

#include <stdlib.h>
#include "types.h"

void clipboard_free(void);
void clipboard_cache_invalidate(struct file_cache* cache);
void clipboard_set(struct range_tree* data, int binary, struct encoding* encoding);
struct range_tree* clipboard_get(struct encoding** encoding, int* binary);
#ifdef _WINDOWS
struct range_tree* clipboard_command_get(struct encoding** encoding);
void clipboard_command_set(struct range_tree* data, struct encoding* encoding);
#else
struct range_tree* clipboard_command_get(struct encoding** encoding, const char* command);
void clipboard_command_set(struct range_tree* data, struct encoding* encoding, const char* command);
#endif

#endif  /* #ifndef TIPPSE_CLIPBOARD_H */
