#ifndef __TIPPSE_FILETYPE_C__
#define __TIPPSE_FILETYPE_C__

#include <stdlib.h>
#include "trie.h"
#include "filetype.h"

struct file_type_c {
  struct file_type vtbl;

  struct trie* keywords;  
};

struct file_type* file_type_c_create();
void file_type_c_destroy(struct file_type* base);
struct trie* file_type_c_keywords(struct file_type* base);

#endif  /* #ifndef __TIPPSE_FILETYPE_C__ */
