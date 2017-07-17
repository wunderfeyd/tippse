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

  struct trie* (*keywords)(struct file_type*);
  void (*mark)(struct file_type*, int*, struct encoding* encoding, struct encoding_stream stream, int, int*, int*);
};

int file_type_keyword(struct encoding* encoding, struct encoding_stream stream, struct trie* trie, int* keyword_length);

#endif  /* #ifndef __TIPPSE_FILETYPE__ */
