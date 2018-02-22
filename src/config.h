#ifndef TIPPSE_CONFIG_H
#define TIPPSE_CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "list.h"
struct trie;

struct config_argument {
  codepoint_t* codepoints;  // argument codepoints
  size_t length;            // argument length
};

struct config_command {
  int cached;               // cached?
  int64_t value;            // cached value
  struct config_argument* arguments;  // all arguments separated
  size_t length;            // number of arguments
};

struct config_value {
  int cached;               // was first command cached?
  int64_t value;            // cached value of first command
  int parsed;               // already parsed?
  struct list commands;     // parsed commands
  codepoint_t* codepoints;  // codepoints
  size_t length;            // number of codepoints
};

struct config {
  struct trie* keywords;    // all keys
  struct list* values;      // all values
};

struct config_cache {
  const char* text;         // key
  int64_t value;            // value
  const char* description;  // description
};

#include "trie.h"
#include "rangetree.h"
#include "encoding.h"
#include "documentfile.h"

void config_command_create(struct config_command* base);
void config_command_destroy_inplace(struct config_command* base);
void config_command_destroy(struct config_command* base);
struct config_command* config_command_clone(struct config_command* clone);
int64_t config_command_cache(struct config_command* base, struct config_cache* cache);

struct config_value* config_value_create(codepoint_t* value_codepoints, size_t value_length);
void config_value_create_inplace(struct config_value* base, codepoint_t* value_codepoints, size_t value_length);
void config_value_destroy(struct config_value* base);
void config_value_destroy_inplace(struct config_value* base);
struct config_value* config_value_clone(struct config_value* base);
void config_value_parse_command(struct config_value* base);

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

struct config_argument* config_value(struct trie_node* parent);

uint8_t* config_convert_encoding(struct trie_node* parent, struct encoding* encoding);
uint8_t* config_convert_encoding_plain(struct config_argument* argument, struct encoding* encoding);
int64_t config_convert_int64_cache(struct trie_node* parent, struct config_cache* cache);
int64_t config_convert_int64(struct trie_node* parent);
int64_t config_convert_int64_plain(struct config_argument* argument);

#endif /* #ifndef TIPPSE_CONFIG_H */
