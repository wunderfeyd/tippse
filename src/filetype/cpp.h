#ifndef TIPPSE_FILETYPE_CPP_H
#define TIPPSE_FILETYPE_CPP_H

#include <stdlib.h>

struct file_type_cpp;
struct trie_node;

#include "../filetype.h"

struct file_type_cpp {
  struct file_type vtbl;

  struct trie_node* keywords;
  struct trie_node* keywords_preprocessor;
};

#include "../trie.h"
#include "c.h"

struct file_type* file_type_cpp_create(struct config* config);
void file_type_cpp_destroy(struct file_type* base);
const char* file_type_cpp_name(void);
void file_type_cpp_mark(struct file_type* base, int* visual_detail, struct encoding* encoding, struct encoding_stream stream, int same_line, int* length, int* flags);

#endif  /* #ifndef TIPPSE_FILETYPE_CPP_H */
