#ifndef TIPPSE_FILETYPE_C_H
#define TIPPSE_FILETYPE_C_H

#include <stdlib.h>

struct file_type_c;
struct trie_node;

#include "../filetype.h"

struct file_type_c {
  struct file_type vtbl;

  struct trie_node* syntax;
  struct trie_node* keywords;
  struct trie_node* keywords_preprocessor;
};

#include "../trie.h"

struct file_type* file_type_c_create(struct config* config, const char* file_type);
void file_type_c_destroy(struct file_type* base);
int file_type_c_mark(struct document_text_render_info* render_info);
const char* file_type_c_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_C_H */
