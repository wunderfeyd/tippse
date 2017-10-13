#ifndef TIPPSE_FILETYPE_PHP_H
#define TIPPSE_FILETYPE_PHP_H

#include <stdlib.h>

struct file_type_php;

#include "../trie.h"
#include "../filetype.h"
#include "../rangetree.h"

struct file_type_php {
  struct file_type vtbl;

  struct trie* keywords;
};

struct file_type* file_type_php_create(void);
void file_type_php_destroy(struct file_type* base);
void file_type_php_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags);
const char* file_type_php_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_PHP_H */
