#ifndef TIPPSE_FILETYPE_SQL_H
#define TIPPSE_FILETYPE_SQL_H

#include <stdlib.h>
#include "../types.h"

#include "../filetype.h"

struct file_type_sql {
  struct file_type vtbl;

  struct trie_node* keywords;
};

struct file_type* file_type_sql_create(struct config* config, const char* file_type);
void file_type_sql_destroy(struct file_type* base);
void file_type_sql_mark(struct document_text_render_info* render_info);
const char* file_type_sql_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_SQL_H */
