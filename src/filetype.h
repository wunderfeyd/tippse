#ifndef TIPPSE_FILETYPE_H
#define TIPPSE_FILETYPE_H

#include <stdlib.h>
#include "types.h"

struct file_type {
  struct file_type* (*create)(struct config* config, const char* file_type);
  void (*destroy)(struct file_type* base);

  const char* (*name)(void);
  const char* (*type)(struct file_type* base);
  void (*mark)(struct document_text_render_info* render_info);
  int (*bracket_match)(const struct document_text_render_info* render_info);

  struct config* config;
  char* file_type;
};

struct trie_node* file_type_config_base(struct file_type* base, const char* suffix);
int file_type_keyword_config(struct file_type* base, struct unicode_sequencer* sequencer, struct trie_node* parent, int* keyword_length, int nocase);
int file_type_bracket_match(const struct document_text_render_info* render_info);
const char* file_type_file_type(struct file_type* base);

#endif  /* #ifndef TIPPSE_FILETYPE_H */
