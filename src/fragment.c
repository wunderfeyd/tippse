// Tippse - Fragment - Holds a portion of a document, is a shared object using reference counting

#include "fragment.h"

struct fragment* fragment_create_memory(uint8_t* buffer, size_t length) {
  struct fragment* base = malloc(sizeof(struct fragment));
  base->type = FRAGMENT_MEMORY;
  base->count = 0;
  base->buffer = buffer;
  base->length = length;

  fragment_reference(base, NULL);
  return base;
}

struct fragment* fragment_create_file(struct file_cache* cache, file_offset_t offset, size_t length, struct document_file* file) {
  struct fragment* base = malloc(sizeof(struct fragment));
  base->type = FRAGMENT_FILE;
  base->count = 0;
  base->cache = cache;
  base->cache_node = NULL;
  base->offset = offset;
  base->length = length;

  fragment_reference(base, file);
  file_cache_reference(base->cache);

  return base;
}

void fragment_cache(struct fragment* base) {
  if (base->type==FRAGMENT_FILE) {
    base->buffer = file_cache_use_node(base->cache, &base->cache_node, base->offset, base->length);
  }
}

void fragment_reference(struct fragment* base, struct document_file* file) {
  if (base->type==FRAGMENT_FILE && file) {
    document_file_reference_cache(file, base->cache);
  }

  base->count++;
}

void fragment_dereference(struct fragment* base, struct document_file* file) {
  if (base->type==FRAGMENT_FILE && file) {
    document_file_dereference_cache(file, base->cache);
  }

  base->count--;

  if (base->count==0) {
    if (base->type==FRAGMENT_MEMORY) {
      free(base->buffer);
    } else {
      if (base->cache_node) {
        file_cache_release_node(base->cache, base->cache_node);
      }
      file_cache_dereference(base->cache);
    }
    free(base);
  }
}
