#ifndef __TIPPSE_FILETYPE_SQL__
#define __TIPPSE_FILETYPE_SQL__

#include <stdlib.h>

struct file_type_sql;

#include "../trie.h"
#include "../filetype.h"
#include "../rangetree.h"

struct file_type_sql {
  struct file_type vtbl;

  struct trie* keywords;
};

struct file_type* file_type_sql_create();
void file_type_sql_destroy(struct file_type* base);
void file_type_sql_mark(struct file_type* base, int* visual_detail, struct encoding* encoding, struct encoding_stream stream, int same_line, int* length, int* flags);
const char* file_type_sql_name();

#endif  /* #ifndef __TIPPSE_FILETYPE_SQL__ */
