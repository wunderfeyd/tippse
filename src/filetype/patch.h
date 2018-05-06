#ifndef TIPPSE_FILETYPE_PATCH_H
#define TIPPSE_FILETYPE_PATCH_H

#include <stdlib.h>

struct file_type_patch;
struct trie;

#include "../filetype.h"

struct file_type_patch {
  struct file_type vtbl;
};

#include "../trie.h"

struct file_type* file_type_patch_create(struct config* config, const char* file_type);
void file_type_patch_destroy(struct file_type* base);
int file_type_patch_mark(struct document_text_render_info* render_info);
const char* file_type_patch_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_PATCH_H */
