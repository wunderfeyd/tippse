/* Tippse - Trie - Faster string lookups */

#include "trie.h"

struct trie* trie_create() {
  struct trie* trie = malloc(sizeof(struct trie));
  trie->fill = TRIE_NODES_PER_BUCKET;
  trie->buckets = list_create();
  trie->root = trie_create_node(trie);

  return trie;
}

void trie_destroy(struct trie* trie) {
  struct list_node* node = trie->buckets->first;
  while (node) {
    struct trie_node* bucket = (struct trie_node*)node->object;
    free(bucket);
    node = node->next;
  }

  free(trie);
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
  node->side[0] = NULL;
  node->side[1] = NULL;
  trie->fill++;
  return node;
}

struct trie_node* trie_append_codepoint(struct trie* trie, struct trie_node* parent, int cp, int type) {
  if (parent==NULL) {
    parent = trie->root;
  }

  int bit;
  for (bit = TRIE_CODEPOINT_BIT-1; bit>=0; bit--) {
    int set = (cp>>bit)&1;
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
  if (parent==NULL) {
    parent = trie->root;
  }

  int bit;
  for (bit = TRIE_CODEPOINT_BIT-1; bit>=0; bit--) {
    int set = (cp>>bit)&1;
    struct trie_node* node = parent->side[set];
    if (node==NULL) {
      return NULL;
    }

    parent = node;
  }

  return parent;
}

void trie_append_string(struct trie* trie, const char* text, int type) {
  struct trie_node* parent = NULL;
  while (*text) {
    parent = trie_append_codepoint(trie, parent, *(unsigned char*)text, (text[1]=='\0')?type:0);
    text++;
  }
}

void trie_load_array(struct trie* trie, const struct trie_static* array) {
  while (array->text) {
    trie_append_string(trie, array->text, array->type);
    array++;
  }
}

void trie_append_string_nocase(struct trie* trie, struct trie_node* parent, const char* text, int type) {
  if (*text) {
    trie_append_string_nocase(trie, trie_append_codepoint(trie, parent, *(unsigned char*)text, (text[1]=='\0')?type:0), text+1, type);
    trie_append_string_nocase(trie, trie_append_codepoint(trie, parent, toupper(*(unsigned char*)text), (text[1]=='\0')?type:0), text+1, type);
  }
}

void trie_load_array_nocase(struct trie* trie, const struct trie_static* array) {
  while (array->text) {
    trie_append_string_nocase(trie, NULL, array->text, array->type);
    array++;
  }
}
