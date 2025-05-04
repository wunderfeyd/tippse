#ifndef TIPPSE_FILETYPE_TEXT_H
#define TIPPSE_FILETYPE_TEXT_H

#include <stdlib.h>
#include "../types.h"

#include "../filetype.h"

struct file_type_text {
  struct file_type vtbl;
};

struct file_type* file_type_text_create(struct config* config, const char* type_name);
void file_type_text_destroy(struct file_type* base);
void file_type_text_mark(struct document_text_render_info* render_info, struct unicode_sequencer* sequencer, struct unicode_sequence* sequence);
const char* file_type_text_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_TEXT_H */
