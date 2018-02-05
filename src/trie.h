#ifndef TIPPSE_TRIE_H
#define TIPPSE_TRIE_H

#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include "types.h"

struct trie_node;
struct trie;
struct list;

#define TRIE_NODES_PER_BUCKET 1024
#define TRIE_CODEPOINT_BIT 20

struct trie_node {
  int end;
  struct trie_node* parent;
  struct trie_node* side[16];
};

struct trie {
  struct trie_node* root;

  struct list* buckets;
  size_t fill;
  size_t node_size;
};

#include "list.h"

struct trie* trie_create(size_t node_size);
void trie_destroy(struct trie* base);
void trie_clear(struct trie* base);
inline void* trie_object(struct trie_node* node) {return (void*)(node+1);}
struct trie_node* trie_create_node(struct trie* base);
struct trie_node* trie_append_codepoint(struct trie* base, struct trie_node* parent, codepoint_t cp, int end);
struct trie_node* trie_find_codepoint(struct trie* base, struct trie_node* parent, codepoint_t cp);
struct trie_node* trie_find_codepoint_min(struct trie* base, struct trie_node* parent, codepoint_t cp, codepoint_t* out);
struct trie_node* trie_find_codepoint_recursive(struct trie* base, struct trie_node* parent, codepoint_t cp, codepoint_t* out, codepoint_t build, int bit);
codepoint_t trie_find_codepoint_single(struct trie* base, struct trie_node* parent);

#endif  /* #ifndef TIPPSE_TRIE_H */
