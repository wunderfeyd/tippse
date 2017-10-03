#ifndef __TIPPSE_CLIPBOARD__
#define __TIPPSE_CLIPBOARD__

#include <stdlib.h>
#include "rangetree.h"

void clipboard_set(struct range_tree_node* data, int binary);
void clipboard_command_set(struct range_tree_node* data, int binary, const char* command);
void clipboard_windows_set(struct range_tree_node* data, int binary);
struct range_tree_node* clipboard_get();
struct range_tree_node* clipboard_command_get(const char* command);
struct range_tree_node* clipboard_windows_get();

#endif  /* #ifndef __TIPPSE_UTF8__ */
