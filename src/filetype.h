#ifndef TIPPSE_FILETYPE_H
#define TIPPSE_FILETYPE_H

#include <stdlib.h>

struct file_type;

#include "trie.h"
#include "visualinfo.h"
#include "rangetree.h"
#include "encoding.h"

struct file_type {
  struct file_type* (*create)(void);
  void (*destroy)(struct file_type*);

  const char* (*name)(void);
  void (*mark)(struct file_type*, int*, struct encoding_cache* cache, int, int*, int*);
  int (*bracket_match)(int visual_detail, int* cp, size_t length);
};

int file_type_keyword(struct encoding_cache* cache, struct trie* trie, int* keyword_length);
int file_type_bracket_match(int visual_detail, int* cp, size_t length);

#endif  /* #ifndef TIPPSE_FILETYPE_H */
