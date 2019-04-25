#ifndef TIPPSE_FILETYPE_MARKDOWN_H
#define TIPPSE_FILETYPE_MARKDOWN_H

#include <stdlib.h>
#include "../types.h"

#include "../filetype.h"

struct file_type_markdown {
  struct file_type vtbl;
};

struct file_type* file_type_markdown_create(struct config* config, const char* file_type);
void file_type_markdown_destroy(struct file_type* base);
void file_type_markdown_mark(struct document_text_render_info* render_info);
const char* file_type_markdown_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_MARKDOWN_H */
