#ifndef TIPPSE_FILETYPE_LUA_H
#define TIPPSE_FILETYPE_LUA_H

#include <stdlib.h>
#include "../types.h"

#include "../filetype.h"

struct file_type_lua {
  struct file_type vtbl;

  struct trie_node* keywords;
};

struct file_type* file_type_lua_create(struct config* config, const char* type_name);
void file_type_lua_destroy(struct file_type* base);
void file_type_lua_mark(struct document_text_render_info* render_info, struct unicode_sequencer* sequencer, struct unicode_sequence* sequence);
const char* file_type_lua_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_LUA_H */
