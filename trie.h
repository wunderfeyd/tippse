#ifndef __TIPPSE_TRIE__
#define __TIPPSE_TRIE__

#include <stdlib.h>
#include "list.h"

#define TRIE_NODES_PER_BUCKET 1024
#define TRIE_CODEPOINT_BIT 20

struct trie_static {
  const char* text;
  int type;
};

struct trie_node {
  int type;
  struct trie_node* parent;
  struct trie_node* side[2];
};

struct trie {
  struct trie_node* root;

  struct list* buckets;
  size_t fill;
};

struct trie* trie_create();
void trie_destroy(struct trie* trie);
struct trie_node* trie_create_node(struct trie* trie);
struct trie_node* trie_append_codepoint(struct trie* trie, struct trie_node* parent, int cp, int type);
struct trie_node* trie_find_codepoint(struct trie* trie, struct trie_node* parent, int cp);
void trie_append_string(struct trie* trie, const char* text, int type);
void trie_load_array(struct trie* trie, const struct trie_static* array);

#endif  /* #ifndef __TIPPSE_TRIE__ */
