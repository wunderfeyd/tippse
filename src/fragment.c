/* Tippse - Fragment - Holds a portion of a document, is a shared object using reference counting */

#include "fragment.h"

struct fragment* fragment_create_memory(uint8_t* buffer, size_t length) {
  struct fragment* node = malloc(sizeof(struct fragment));
  node->type = FRAGMENT_MEMORY;
  node->count = 1;
  node->buffer = buffer;
  node->length = length;
  return node;
}

struct fragment* fragment_create_file(struct file_cache* cache, file_offset_t offset, size_t length) {
  struct fragment* node = malloc(sizeof(struct fragment));
  node->type = FRAGMENT_FILE;
  node->count = 1;
  node->cache = cache;
  node->cache_node = NULL;
  node->offset = offset;
  node->length = length;

  file_cache_reference(node->cache);

  return node;
}

void fragment_cache(struct fragment* node) {
  if (node->type==FRAGMENT_FILE) {
    node->buffer = file_cache_use_node(node->cache, &node->cache_node, node->offset, node->length);
  }
}

void fragment_reference(struct fragment* node) {
  node->count++;
}

void fragment_dereference(struct fragment* node) {
  node->count--;

  if (node->count==0) {
    if (node->type==FRAGMENT_MEMORY) {
      free(node->buffer);
    } else {
      file_cache_dereference(node->cache);
    }

    free(node);
  }
}
