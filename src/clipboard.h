#ifndef TIPPSE_CLIPBOARD_H
#define TIPPSE_CLIPBOARD_H

#include <stdlib.h>
struct range_tree_node;

void clipboard_free(void);
void clipboard_set(struct range_tree_node* data, int binary);
void clipboard_command_set(struct range_tree_node* data, int binary, const char* command);
void clipboard_windows_set(struct range_tree_node* data, int binary);
struct range_tree_node* clipboard_get(void);
struct range_tree_node* clipboard_command_get(const char* command);
struct range_tree_node* clipboard_windows_get(void);

#include "rangetree.h"
#include "stream.h"

#endif  /* #ifndef TIPPSE_CLIPBOARD_H */
