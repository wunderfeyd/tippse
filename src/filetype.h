#ifndef TIPPSE_FILETYPE_H
#define TIPPSE_FILETYPE_H

#include <stdlib.h>

struct file_type;
struct encoding_cache;
struct config;

struct file_type {
  struct file_type* (*create)(struct config* config);
  void (*destroy)(struct file_type* base);

  const char* (*name)(void);
  void (*mark)(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags);
  int (*bracket_match)(int visual_detail, int* cp, size_t length);

  struct config* config;
};

#include "trie.h"
#include "visualinfo.h"
#include "rangetree.h"
#include "encoding.h"
#include "config.h"

struct trie_node* file_type_config_base(struct file_type* base, const char* suffix);
int file_type_keyword_config(struct file_type* base, struct encoding_cache* cache, struct trie_node* parent, int* keyword_length, int nocase);
int file_type_bracket_match(int visual_detail, int* cp, size_t length);

#endif  /* #ifndef TIPPSE_FILETYPE_H */
