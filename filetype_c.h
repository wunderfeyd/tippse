#ifndef __TIPPSE_FILETYPE_C__
#define __TIPPSE_FILETYPE_C__

#include <stdlib.h>

struct file_type_c;

#include "trie.h"
#include "filetype.h"
#include "rangetree.h"

struct file_type_c {
  struct file_type vtbl;

  struct trie* keywords;  
};

struct file_type* file_type_c_create();
void file_type_c_destroy(struct file_type* base);
struct trie* file_type_c_keywords(struct file_type* base);
void file_type_c_mark(struct file_type* base, int* visual_detail, struct range_tree_node* node, file_offset_t buffer_pos, int* length, int* flags);

#endif  /* #ifndef __TIPPSE_FILETYPE_C__ */
