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
#define SEARCH_NODE_TYPE_POSSESSIVE 8
#define SEARCH_NODE_TYPE_MATCHING_TYPE 16
#define SEARCH_NODE_TYPE_GROUP_START 32
#define SEARCH_NODE_TYPE_GROUP_END 64
#define SEARCH_NODE_TYPE_BYTE 128

#define SEARCH_NODE_SET_CODES 0x110000
#define SEARCH_NODE_SET_BUCKET (sizeof(uint32_t)*8)
#define SEARCH_NODE_SET_SIZE (SEARCH_NODE_SET_CODES/SEARCH_NODE_SET_BUCKET)
#define SEARCH_NODE_TYPE_NATIVE_COUNT 8

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
  struct list* group_start; // list of groups entered by the node
  struct list* group_end; // list of groups exited by the node
  struct encoding_stream start; // start of group content during scan
  struct encoding_stream end;   // end of group content during scan
};

struct search_group {
  struct search_node* node_start; // start group node
  struct search_node* node_end; // end group node
  struct encoding_stream start; // start of group content best hit
  struct encoding_stream end;   // end of group content best hit
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
  size_t groups;                    // active groups
  struct search_group* group_hits;  // group hits
  int reverse;                      // search up?
  struct encoding_stream hit_start; // found match start location
  struct encoding_stream hit_end;   // found match end location
  size_t stack_size;                // stack size for search loop
  struct search_stack* stack;       // active stack
  struct encoding* encoding;        // encoding the search is compiled for
};

struct search_node* search_node_create(int type);
void search_node_destroy(struct search_node* base);
void search_node_empty(struct search_node* base);
void search_node_move(struct search_node* base, struct search_node* copy);
void search_node_destroy_recursive(struct search_node* node);
int search_node_count(struct search_node* node);
void search_node_set(struct search_node* node, size_t index);
void search_node_set_decode_rle(struct search_node* node, int invert, uint16_t* rle);

struct search* search_create_plain(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding);
struct search* search_create_regex(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding);
void search_destroy(struct search* base);
size_t search_append_unicode(struct search_node* last, int ignore_case, struct encoding_cache* cache, size_t offset, struct encoding* output_encoding);
struct search_node* search_append_next_index(struct search_node* last, size_t index, int type);
void search_append_next_codepoint(struct search_node* last, codepoint_t* buffer, size_t size);
void search_append_next_byte(struct search_node* last, uint8_t* buffer, size_t size);
void search_debug_tree(struct search* base, struct search_node* node, size_t depth, int length, int stop);
int search_find(struct search* base, struct encoding_stream* text);
int search_find_loop(struct search* base, struct search_node* node, struct encoding_stream* text);

void search_optimize(struct search* base, struct encoding* encoding);
void search_optimize_mark_end(struct search_node* node);
int search_optimize_reduce_branch(struct search_node* node);
int search_optimize_combine_branch(struct search_node* node);
int search_optimize_flatten_branch(struct search_node* node);
void search_optimize_native(struct encoding* encoding, struct search_node* node);
void search_optimize_plain(struct search_node* node);
void search_prepare(struct search* base, struct search_node* node, struct search_node* prev);

void search_test(void);
#endif /* #ifndef TIPPSE_SEARCH_H */
