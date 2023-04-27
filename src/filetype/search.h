#ifndef TIPPSE_FILETYPE_SEARCH_H
#define TIPPSE_FILETYPE_SEARCH_H

#include <stdlib.h>
#include "../types.h"

#include "../filetype.h"

struct file_type_search {
  struct file_type vtbl;
};

struct file_type* file_type_search_create(struct config* config, const char* type_name);
void file_type_search_destroy(struct file_type* base);
void file_type_search_mark(struct document_text_render_info* render_info, int bracket_match);
const char* file_type_search_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_SEARCH_H */
