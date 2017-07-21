#ifndef __TIPPSE_FILETYPE_LUA__
#define __TIPPSE_FILETYPE_LUA__

#include <stdlib.h>

struct file_type_lua;

#include "../trie.h"
#include "../filetype.h"
#include "../rangetree.h"

struct file_type_lua {
  struct file_type vtbl;

  struct trie* keywords;
};

struct file_type* file_type_lua_create();
void file_type_lua_destroy(struct file_type* base);
void file_type_lua_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags);
const char* file_type_lua_name();

#endif  /* #ifndef __TIPPSE_FILETYPE_C__ */
