#ifndef TIPPSE_FILETYPE_H
#define TIPPSE_FILETYPE_H

#include <stdlib.h>
#include "types.h"

struct file_type {
  struct file_type* (*create)(struct config* config, const char* type_name);
  void (*destroy)(struct file_type* base);

  const char* (*name)(void);
  const char* (*type)(struct file_type* base);
  void (*mark)(struct document_text_render_info* render_info, int bracket_match);
  int (*bracket_match)(const struct document_text_render_info* render_info);

  struct config* config;
  char* type_name;
};

struct trie_node* file_type_config_base(const struct file_type* base, const char* suffix);
int file_type_keyword_config(const struct file_type* base, struct unicode_sequencer* sequencer, struct trie_node* parent, long* keyword_length, int nocase);
int file_type_bracket_match(const struct document_text_render_info* render_info);
const char* file_type_file_type(struct file_type* base);

#endif  /* #ifndef TIPPSE_FILETYPE_H */
