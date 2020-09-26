#ifndef TIPPSE_FRAGMENT_H
#define TIPPSE_FRAGMENT_H

#define FRAGMENT_MEMORY 0
#define FRAGMENT_FILE 1

#include <stdlib.h>
#include "types.h"
#include "rangetree.h"

struct fragment {
  file_offset_t count;                  // Reference counter
  int type;                             // Type of fragment
  uint8_t* buffer;                      // Pointer to buffer content
  file_offset_t offset;                 // Absolute offset to on disk file content
  struct file_cache* cache;             // Cache handler

  file_offset_t length;                 // Length of fragment data
};

struct fragment* fragment_create_memory(uint8_t* buffer, size_t length);
struct fragment* fragment_create_file(struct file_cache* cache, file_offset_t offset, file_offset_t length, struct range_tree_callback* callback);
void fragment_reference(struct fragment* base, struct range_tree_callback* callback);
void fragment_dereference(struct fragment* base, struct range_tree_callback* callback);

#endif /* #ifndef TIPPSE_FRAGMENT_H */
