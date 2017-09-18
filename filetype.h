#ifndef __TIPPSE_FILETYPE__
#define __TIPPSE_FILETYPE__

#include <stdlib.h>

struct file_type;

#include "trie.h"
#include "visualinfo.h"
#include "rangetree.h"
#include "encoding.h"

struct file_type {
  struct file_type* (*create)();
  void (*destroy)(struct file_type*);

  const char* (*name)();
  void (*mark)(struct file_type*, int*, struct encoding_cache* cache, int, int*, int*);
  int (*bracket_match)(int visual_detail, int* cp, int length);
};

int file_type_keyword(struct encoding_cache* cache, struct trie* trie, int* keyword_length);
int file_type_bracket_match(int visual_detail, int* cp, int length);

#endif  /* #ifndef __TIPPSE_FILETYPE__ */
