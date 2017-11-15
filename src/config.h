#ifndef TIPPSE_CONFIG_H
#define TIPPSE_CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include "list.h"
#include "trie.h"
#include "rangetree.h"
#include "encoding.h"
#include "documentfile.h"

struct config {
  struct trie* keywords;    // all keys
  struct list* values;      // all values
};

struct config* config_create(void);
void config_destroy(struct config* config);
void config_clear(struct config* config);

void config_load(struct config* config, const char* filename);
void config_loadpaths(struct config* config, const char* filename, int strip);

void config_update(struct config* config, int* keyword_codepoints, size_t keyword_length, int* value_codepoints, size_t value_length);
struct trie_node* config_find_codepoints(struct config* config, int* keyword_codepoints, size_t keyword_length);
struct trie_node* config_find_ascii(struct config* config, const char* keyword);

struct trie_node* config_advance_codepoints(struct config* config, struct trie_node* parent, int* keyword_codepoints, size_t keyword_length);
struct trie_node* config_advance_ascii(struct config* config, struct trie_node* parent, const char* keyword);

int* config_value(struct trie_node* parent);

char* config_convert_ascii(struct trie_node* parent);
char* config_convert_ascii_plain(int* codepoints);
int64_t config_convert_int64(struct trie_node* parent);
int64_t config_convert_int64_plain(int* codepoints);

#endif /* #ifndef TIPPSE_CONFIG_H */
