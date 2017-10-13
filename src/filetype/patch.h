#ifndef TIPPSE_FILETYPE_PATCH_H
#define TIPPSE_FILETYPE_PATCH_H

#include <stdlib.h>

struct file_type_patch;

#include "../trie.h"
#include "../filetype.h"
#include "../rangetree.h"

struct file_type_patch {
  struct file_type vtbl;
};

struct file_type* file_type_patch_create(void);
void file_type_patch_destroy(struct file_type* base);
void file_type_patch_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags);
const char* file_type_patch_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_PATCH_H */
