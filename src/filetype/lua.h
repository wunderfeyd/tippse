#ifndef TIPPSE_FILETYPE_LUA_H
#define TIPPSE_FILETYPE_LUA_H

#include <stdlib.h>

struct file_type_lua;
struct trie_node;

#include "../filetype.h"

struct file_type_lua {
  struct file_type vtbl;

  struct trie_node* keywords;
};

#include "../trie.h"

struct file_type* file_type_lua_create(struct config* config, const char* file_type);
void file_type_lua_destroy(struct file_type* base);
void file_type_lua_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int* length, int* flags);
const char* file_type_lua_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_LUA_H */
