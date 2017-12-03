#ifndef TIPPSE_CONFIG_H
#define TIPPSE_CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include "types.h"
struct trie;
struct list;

struct config_value {
  int64_t value;            // cached value
  int cached;               // cached?
  codepoint_t* codepoints;  // codepoints
};

struct config {
  struct trie* keywords;    // all keys
  struct list* values;      // all values
};

struct config_cache {
  const char* text;         // key
  int64_t value;            // value
};

#include "list.h"
#include "trie.h"
#include "rangetree.h"
#include "encoding.h"
#include "documentfile.h"

void config_value_create(struct config_value* base, codepoint_t* value_codepoints, size_t value_length);
void config_value_destroy(struct config_value* base);

struct config* config_create(void);
void config_destroy(struct config* base);
void config_clear(struct config* base);

void config_load(struct config* base, const char* filename);
void config_loadpaths(struct config* base, const char* filename, int strip);

void config_update(struct config* base, codepoint_t* keyword_codepoints, size_t keyword_length, codepoint_t* value_codepoints, size_t value_length);
struct trie_node* config_find_codepoints(struct config* base, codepoint_t* keyword_codepoints, size_t keyword_length);
struct trie_node* config_find_ascii(struct config* base, const char* keyword);

struct trie_node* config_advance_codepoints(struct config* base, struct trie_node* parent, codepoint_t* keyword_codepoints, size_t keyword_length);
struct trie_node* config_advance_ascii(struct config* base, struct trie_node* parent, const char* keyword);

codepoint_t* config_value(struct trie_node* parent);

char* config_convert_ascii(struct trie_node* parent);
char* config_convert_ascii_plain(codepoint_t* codepoints);
int64_t config_convert_int64_cache(struct trie_node* parent, struct config_cache* cache);
int64_t config_convert_int64(struct trie_node* parent);
int64_t config_convert_int64_plain(codepoint_t* codepoints);

#endif /* #ifndef TIPPSE_CONFIG_H */
