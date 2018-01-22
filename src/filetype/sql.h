#ifndef TIPPSE_FILETYPE_SQL_H
#define TIPPSE_FILETYPE_SQL_H

#include <stdlib.h>

struct file_type_sql;
struct trie_node;

#include "../filetype.h"

struct file_type_sql {
  struct file_type vtbl;

  struct trie_node* keywords;
};

#include "../trie.h"

struct file_type* file_type_sql_create(struct config* config, const char* file_type);
void file_type_sql_destroy(struct file_type* base);
void file_type_sql_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags);
const char* file_type_sql_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_SQL_H */
