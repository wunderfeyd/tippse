/* Tippse - Fragment - Holds a portion of a document, is a shared object using reference counting */

#include "fragment.h"

struct fragment* fragment_create_memory(char* buffer, size_t length) {
  struct fragment* node = malloc(sizeof(struct fragment));
  node->type = FRAGMENT_MEMORY;
  node->count = 1;
  node->buffer = buffer;
  node->length = length;
  return node;
}

void fragment_reference(struct fragment* node) {
  node->count++;
}

void fragment_dereference(struct fragment* node) {
  node->count--;

  if (node->count==0) {
    if (node->type==FRAGMENT_MEMORY) {
      free(node->buffer);
    }

    free(node);
  }
}
