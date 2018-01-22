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
void file_type_patch_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags);
const char* file_type_patch_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_PATCH_H */
