#ifndef TIPPSE_TRIE_H
#define TIPPSE_TRIE_H

#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

struct trie_node;
struct trie;
struct list;

#define TRIE_NODES_PER_BUCKET 1024
#define TRIE_CODEPOINT_BIT 20

struct trie_node {
  intptr_t type;
  struct trie_node* parent;
  struct trie_node* side[16];
};

struct trie {
  struct trie_node* root;

  struct list* buckets;
  size_t fill;
};

#include "list.h"

struct trie* trie_create(void);
void trie_destroy(struct trie* trie);
void trie_clear(struct trie* trie);
struct trie_node* trie_create_node(struct trie* trie);
struct trie_node* trie_append_codepoint(struct trie* trie, struct trie_node* parent, int cp, intptr_t type);
struct trie_node* trie_find_codepoint(struct trie* trie, struct trie_node* parent, int cp);

#endif  /* #ifndef TIPPSE_TRIE_H */
