#ifndef TIPPSE_FILETYPE_PHP_H
#define TIPPSE_FILETYPE_PHP_H

#include <stdlib.h>

struct file_type_php;
struct trie_node;

#include "../filetype.h"

struct file_type_php {
  struct file_type vtbl;

  struct trie_node* keywords;
};

#include "../trie.h"

struct file_type* file_type_php_create(struct config* config, const char* file_type);
void file_type_php_destroy(struct file_type* base);
int file_type_php_mark(struct document_text_render_info* render_info);
const char* file_type_php_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_PHP_H */
