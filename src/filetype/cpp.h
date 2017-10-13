#ifndef TIPPSE_FILETYPE_CPP_H
#define TIPPSE_FILETYPE_CPP_H

#include <stdlib.h>

struct file_type_cpp;

#include "../trie.h"
#include "../filetype.h"
#include "../rangetree.h"
#include "c.h"

struct file_type_cpp {
  struct file_type vtbl;

  struct trie* keywords;
  struct trie* keywords_preprocessor;
};

struct file_type* file_type_cpp_create(void);
void file_type_cpp_destroy(struct file_type* base);
const char* file_type_cpp_name(void);
void file_type_cpp_mark(struct file_type* base, int* visual_detail, struct encoding* encoding, struct encoding_stream stream, int same_line, int* length, int* flags);

#endif  /* #ifndef TIPPSE_FILETYPE_CPP_H */
