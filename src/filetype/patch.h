#ifndef TIPPSE_FILETYPE_PATCH_H
#define TIPPSE_FILETYPE_PATCH_H

#include <stdlib.h>
#include "../types.h"

#include "../filetype.h"

struct file_type_patch {
  struct file_type vtbl;
};

struct file_type* file_type_patch_create(struct config* config, const char* type_name);
void file_type_patch_destroy(struct file_type* base);
void file_type_patch_mark(struct document_text_render_info* render_info, struct unicode_sequencer* sequencer, struct unicode_sequence* sequence);
const char* file_type_patch_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_PATCH_H */
