#ifndef TIPPSE_SEARCH_H
#define TIPPSE_SEARCH_H

#include <stdlib.h>
#include <stdio.h>
#include "types.h"
#include "misc.h"
#include "list.h"
#include "encoding.h"

#define SEARCH_NODE_TYPE_SET 1
#define SEARCH_NODE_TYPE_BRANCH 2
#define SEARCH_NODE_TYPE_GREEDY 4

#define SEARCH_NODE_SET_BUCKET (sizeof(uint32_t)*8)
#define SEARCH_NODE_SET_SIZE (256/SEARCH_NODE_SET_BUCKET)

struct search_stack;

struct search_node {
  int type;               // type of node
  struct search_stack* stack; // pointer to current stack
  int group;              // regex group id (if closing)
  size_t min;             // min repeat count
  size_t max;             // max repeat count
  size_t size;            // size of plain string
  uint8_t* plain;         // plain string
  uint32_t set[SEARCH_NODE_SET_SIZE]; // hits
  struct search_node* next; // list of next node on hit
  struct search_node* forward; // list of next node on hit
  struct list* sub;       // list of sub nodes
};

struct search_group {
  struct encoding_stream start; // start of group content
  struct encoding_stream end;   // end of group content
};

struct search_stack {
  struct search_node* node;     // current node to crawl
  size_t loop;                  // current loop
  int hits;                     // last hit count
  struct list_node* sub;        // current alternative
  struct encoding_stream stream; // end of group content
  struct search_stack* previous; // stack entry before
};

struct search {
  struct search_node* root;         // root node of search tree
  struct list* groups;              // active groups
  int reverse;                      // search up?
  struct encoding_stream hit_start; // found match start location
  struct encoding_stream hit_end;   // found match end location
  size_t stack_size;                // stack size for search loop
  struct search_stack* stack;       // active stack

};

struct search_node* search_node_create(int type);

struct search* search_create_plain(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding);
struct search* search_create_regex(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding);
void search_destroy(struct search* base);
size_t search_append_unicode(struct search_node* last, int ignore_case, struct encoding_cache* cache, size_t offset, struct encoding* output_encoding);
void search_append_next_char(struct search_node* last, uint8_t* buffer, size_t size);
void search_debug_tree(struct search* base, struct search_node* node, size_t depth, int length, int stop);
int search_find(struct search* base, struct encoding_stream* text);
int search_find_loop(struct search* base, struct search_node* node, struct encoding_stream* text);

void search_optimize(struct search* base);
void search_optimize_mark_end(struct search_node* node);
int search_optimize_reduce_branch(struct search_node* node);
int search_optimize_combine_branch(struct search_node* node);
int search_optimize_flatten_branch(struct search_node* node);
void search_optimize_plain(struct search_node* node);
void search_optimize_next_list(struct search_node* node, struct search_node* prev);

void search_test(void);
#endif /* #ifndef TIPPSE_SEARCH_H */
