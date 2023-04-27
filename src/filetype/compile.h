#ifndef TIPPSE_FILETYPE_COMPILE_H
#define TIPPSE_FILETYPE_COMPILE_H

#include <stdlib.h>
#include "../types.h"

#include "../filetype.h"

struct file_type_compile {
  struct file_type vtbl;

  struct trie_node* keywords;
};

struct file_type* file_type_compile_create(struct config* config, const char* type_name);
void file_type_compile_destroy(struct file_type* base);
void file_type_compile_mark(struct document_text_render_info* render_info, int bracket_match);
const char* file_type_compile_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_COMPILE_H */
