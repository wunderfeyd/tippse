// Tippse - Fragment - Holds a portion of a document, is a shared object using reference counting

#include "fragment.h"

#include "filecache.h"

// Return referenced fragment to a memory location
struct fragment* fragment_create_memory(uint8_t* buffer, size_t length) {
  struct fragment* base = (struct fragment*)malloc(sizeof(struct fragment));
  base->type = FRAGMENT_MEMORY;
  base->count = 0;
  base->buffer = buffer;
  base->length = length;

  fragment_reference(base, NULL);
  return base;
}

// Return referenced fragment to an on disk file location
struct fragment* fragment_create_file(struct file_cache* cache, file_offset_t offset, file_offset_t length, struct range_tree_callback* callback) {
  struct fragment* base = (struct fragment*)malloc(sizeof(struct fragment));
  base->type = FRAGMENT_FILE;
  base->count = 0;
  base->cache = cache;
  base->offset = offset;
  base->length = length;

  fragment_reference(base, callback);
  file_cache_reference(base->cache);

  return base;
}

// Increment reference counter
void fragment_reference(struct fragment* base, struct range_tree_callback* callback) {
  if (callback) {
    (*callback->fragment_reference)(callback, base);
  }

  base->count++;
}

// Decrement reference counter and remove object if it hits zero instances
void fragment_dereference(struct fragment* base, struct range_tree_callback* callback) {
  if (callback) {
    (*callback->fragment_dereference)(callback, base);
  }

  base->count--;

  if (base->count==0) {
    if (base->type==FRAGMENT_MEMORY) {
      free(base->buffer);
    } else {
      file_cache_dereference(base->cache);
    }
    free(base);
  }
}
