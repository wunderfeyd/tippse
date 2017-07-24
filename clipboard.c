/* Tippse - Clipboard - Holds and sets clipboard data (interfaces to the system if possible) */

#include "clipboard.h"

struct range_tree_node* clipboard = NULL;

void clipboard_set(struct range_tree_node* data) {
  if (clipboard) {
    range_tree_destroy(clipboard);
    clipboard = NULL;
  }
  
  clipboard = data;
}

struct range_tree_node* clipboard_get() {
  return clipboard;
}
