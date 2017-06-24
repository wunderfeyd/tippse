/* Tippse - Fragment - Holds a portion of a document, is a shared object using reference counting */

#include "fragment.h"

struct fragment* fragment_create(char* buffer, size_t length) {
  struct fragment* node = malloc(sizeof(struct fragment));
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
    free(node->buffer);
    free(node);
  }
}
