// Tippse - Trie - Faster string lookups

#include "trie.h"

struct trie* trie_create(void) {
  struct trie* trie = malloc(sizeof(struct trie));
  trie->buckets = list_create();
  trie_clear(trie);
  return trie;
}

void trie_destroy(struct trie* trie) {
  trie_clear(trie);
  list_destroy(trie->buckets);
  free(trie);
}

void trie_clear(struct trie* trie) {
  while (trie->buckets->first) {
    struct trie_node* bucket = (struct trie_node*)trie->buckets->first->object;
    free(bucket);
    list_remove(trie->buckets, trie->buckets->first);
  }

  trie->fill = TRIE_NODES_PER_BUCKET;
  trie->root = NULL;
}

struct trie_node* trie_create_node(struct trie* trie) {
  if (trie->fill==TRIE_NODES_PER_BUCKET) {
    trie->fill = 0;
    struct trie_node* bucket = (struct trie_node*)malloc(sizeof(struct trie_node)*TRIE_NODES_PER_BUCKET);
    list_insert(trie->buckets, NULL, bucket);
  }

  struct trie_node* node = &((struct trie_node*)trie->buckets->first->object)[trie->fill];
  node->parent = NULL;
  node->type = 0;
  for (int side = 0; side<16; side++) {
    node->side[side] = NULL;
  }

  trie->fill++;
  return node;
}

struct trie_node* trie_append_codepoint(struct trie* trie, struct trie_node* parent, int cp, intptr_t type) {
  if (!parent) {
    parent = trie->root;
    if (!parent) {
      trie->root = trie_create_node(trie);
      parent = trie->root;
    }
  }

  for (int bit = TRIE_CODEPOINT_BIT-4; bit>=0; bit-=4) {
    int set = (cp>>bit)&15;
    struct trie_node* node = parent->side[set];
    if (node==NULL) {
      node = trie_create_node(trie);
      parent->side[set] = node;
      node->parent = parent;
    }

    node->type = (bit==0 && type!=0)?type:node->type;

    parent = node;
  }

  return parent;
}

struct trie_node* trie_find_codepoint(struct trie* trie, struct trie_node* parent, int cp) {
  if (!parent) {
    parent = trie->root;
    if (!parent) {
      return NULL;
    }
  }

  for (int bit = TRIE_CODEPOINT_BIT-4; bit>=0; bit-=4) {
    int set = (cp>>bit)&15;
    struct trie_node* node = parent->side[set];
    if (node==NULL) {
      return NULL;
    }

    parent = node;
  }

  return parent;
}
