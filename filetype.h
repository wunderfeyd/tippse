#ifndef __TIPPSE_FILETYPE__
#define __TIPPSE_FILETYPE__

#include <stdlib.h>

struct file_type;

#include "trie.h"
#include "visualinfo.h"
#include "rangetree.h"

struct file_type {
  struct file_type* (*create)();
  void (*destroy)(struct file_type*);

  struct trie* (*keywords)(struct file_type*);
  void (*mark)(struct file_type*, int*, struct range_tree_node*, file_offset_t, int*, int*);
};

#endif  /* #ifndef __TIPPSE_FILETYPE__ */
