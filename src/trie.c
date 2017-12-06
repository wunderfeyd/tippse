// Tippse - Trie - Faster string lookups

#include "trie.h"

struct trie* trie_create(void) {
  struct trie* base = malloc(sizeof(struct trie));
  base->buckets = list_create(sizeof(struct trie_node)*TRIE_NODES_PER_BUCKET);
  trie_clear(base);
  return base;
}

void trie_destroy(struct trie* base) {
  trie_clear(base);
  list_destroy(base->buckets);
  free(base);
}

void trie_clear(struct trie* base) {
  while (base->buckets->first) {
    list_remove(base->buckets, base->buckets->first);
  }

  base->fill = TRIE_NODES_PER_BUCKET;
  base->root = NULL;
}

struct trie_node* trie_create_node(struct trie* base) {
  if (base->fill==TRIE_NODES_PER_BUCKET) {
    base->fill = 0;
    list_object(list_insert_empty(base->buckets, NULL));
  }

  struct trie_node* node = &((struct trie_node*)list_object(base->buckets->first))[base->fill];
  node->parent = NULL;
  node->type = 0;
  for (int side = 0; side<16; side++) {
    node->side[side] = NULL;
  }

  base->fill++;
  return node;
}

struct trie_node* trie_append_codepoint(struct trie* base, struct trie_node* parent, codepoint_t cp, intptr_t type) {
  if (!parent) {
    parent = base->root;
    if (!parent) {
      base->root = trie_create_node(base);
      parent = base->root;
    }
  }

  for (int bit = TRIE_CODEPOINT_BIT-4; bit>=0; bit-=4) {
    int set = (cp>>bit)&15;
    struct trie_node* node = parent->side[set];
    if (!node) {
      node = trie_create_node(base);
      parent->side[set] = node;
      node->parent = parent;
    }

    node->type = (bit==0 && type!=0)?type:node->type;

    parent = node;
  }

  return parent;
}

struct trie_node* trie_find_codepoint(struct trie* base, struct trie_node* parent, codepoint_t cp) {
  if (!parent) {
    parent = base->root;
    if (!parent) {
      return NULL;
    }
  }

  for (int bit = TRIE_CODEPOINT_BIT-4; bit>=0; bit-=4) {
    int set = (cp>>bit)&15;
    struct trie_node* node = parent->side[set];
    if (!node) {
      return NULL;
    }

    parent = node;
  }

  return parent;
}

// Find next codepoint (higher than the one before)
struct trie_node* trie_find_codepoint_min(struct trie* base, struct trie_node* parent, codepoint_t cp, codepoint_t* out) {
  if (!parent) {
    parent = base->root;
    if (!parent) {
      return NULL;
    }
  }

  return trie_find_codepoint_recursive(base, parent, cp, out, 0, TRIE_CODEPOINT_BIT-4);
}

// Find next codepoint (higher than the one before)
struct trie_node* trie_find_codepoint_recursive(struct trie* base, struct trie_node* parent, codepoint_t cp, codepoint_t* out, codepoint_t build, int bit) {

  struct trie_node* node = NULL;
  for (int set = 0; set<16; set++) {
    if (parent->side[set]) {
      codepoint_t new = build|((codepoint_t)set<<bit);
      if (new+(1<<bit)-1>cp) {
        if (bit==0) {
          *out = new;
          return parent->side[set];
        } else {
          node = trie_find_codepoint_recursive(base, parent->side[set], cp, out, new, bit-4);
          if (node) {
            break;
          }
        }
      }
    }
  }

  return node;
}
