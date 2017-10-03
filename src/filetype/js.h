#ifndef __TIPPSE_FILETYPE_JS__
#define __TIPPSE_FILETYPE_JS__

#include <stdlib.h>

struct file_type_js;

#include "../trie.h"
#include "../filetype.h"
#include "../rangetree.h"

struct file_type_js {
  struct file_type vtbl;

  struct trie* keywords;
};

struct file_type* file_type_js_create();
void file_type_js_destroy(struct file_type* base);
void file_type_js_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags);
const char* file_type_js_name();

#endif  /* #ifndef __TIPPSE_FILETYPE_JS__ */
