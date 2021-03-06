#ifndef TIPPSE_SEARCH_H
#define TIPPSE_SEARCH_H

#include <stdlib.h>
#include <stdio.h>
#include "types.h"

#include "list.h"
#include "stream.h"

#define SEARCH_NODE_TYPE_SET 1
#define SEARCH_NODE_TYPE_BRANCH 2
#define SEARCH_NODE_TYPE_GREEDY 4
#define SEARCH_NODE_TYPE_POSSESSIVE 8
#define SEARCH_NODE_TYPE_MATCHING_TYPE 16
#define SEARCH_NODE_TYPE_GROUP_START 32
#define SEARCH_NODE_TYPE_GROUP_END 64
#define SEARCH_NODE_TYPE_BYTE 128
#define SEARCH_NODE_TYPE_BACKREFERENCE 256
#define SEARCH_NODE_TYPE_START_POSITION 512
#define SEARCH_NODE_TYPE_END_POSITION 1024

#define SEARCH_NODE_SET_CODES UNICODE_CODEPOINT_MAX
#define SEARCH_NODE_SET_BUCKET (sizeof(uint32_t)*8)
#define SEARCH_NODE_TYPE_NATIVE_COUNT 8

struct search_stack;

#define SEARCH_SKIP_NODES 64

struct search_skip_node {
  size_t index[256];
};

#include "rangetree.h"

struct search_node {
  int type;               // type of node
  struct search_stack* stack; // pointer to current stack
  size_t min;             // min repeat count
  size_t max;             // max repeat count
  size_t size;            // size of plain string
  uint8_t* plain;         // plain string
  struct search_node* next; // list of next node on hit
  struct search_node* forward; // list of next node on hit
  struct list sub;        // list of sub nodes
  struct list group_start; // list of groups entered by the node
  struct list group_end;  // list of groups exited by the node
  struct stream start;    // start of group content during scan
  struct stream end;      // end of group content during scan
  struct range_tree set; // matching codepoints/bytes
  uint32_t bitset[256/SEARCH_NODE_SET_BUCKET+1]; // simple bit table for byte matching
  size_t group;           // group number in back reference
};

struct search_group {
  struct search_node* node_start; // start group node
  struct search_node* node_end; // end group node
  struct stream start; // start of group content best hit
  struct stream end;   // end of group content best hit
};

struct search_stack {
  struct search_node* node;     // current node to crawl
  size_t loop;                  // current loop
  int hits;                     // last hit count
  struct list_node* sub;        // current alternative
  struct stream stream; // end of group content
  struct search_stack* previous; // stack entry before
};

struct search {
  struct search_node* root;         // root node of search tree
  size_t groups;                    // active groups
  struct search_group* group_hits;  // group hits
  int reverse;                      // search up?
  struct stream hit_start; // found match start location
  struct stream hit_end;   // found match end location
  size_t stack_size;                // stack size for search loop
  struct list stack;                // active stack frames
  struct encoding* encoding;        // encoding the search is compiled for

  struct search_skip_node skip[SEARCH_SKIP_NODES];    // skip tree/nodes
  size_t skip_length;               // number of tree nodes
  int skip_rescan;                  // rescan hit with basic search
};

typedef int (*search_optimize_callback)(struct encoding* encoding, struct search_node* node);

struct search_node* search_node_create(int type);
void search_node_destroy(struct search_node* base);
void search_node_empty(struct search_node* base);
void search_node_move(struct search_node* base, struct search_node* copy);
void search_node_destroy_recursive(struct search_node* node);
int search_node_count(struct search_node* node);
void search_node_set_build(struct search_node* node);
void search_node_set(struct search_node* node, size_t index);
void search_node_set_decode_rle(struct search_node* node, int invert, uint8_t* rle);

struct search* search_create(int reverse, struct encoding* output_encoding);
struct search* search_create_plain(int ignore_case, int reverse, struct stream* needle, struct encoding* needle_encoding, struct encoding* output_encoding);
struct search* search_create_regex(int ignore_case, int reverse, struct stream* needle, struct encoding* needle_encoding, struct encoding* output_encoding);
struct range_tree* search_replacement(struct search* base, struct range_tree* replacement_tree, struct encoding* replacement_encoding, struct range_tree* document_tree);
void search_destroy(struct search* base);
struct search_node* search_append_class(struct search_node* last, codepoint_t cp, int create);
size_t search_append_set(struct search_node* last, int ignore_case, struct unicode_sequencer* sequencer, size_t offset);
size_t search_append_unicode(struct search_node* last, int ignore_case, struct unicode_sequencer* sequencer, size_t offset, struct search_node* shorten, size_t min);
struct search_node* search_append_next_index(struct search_node* last, size_t index, int type);
void search_append_next_codepoint(struct search_node* last, codepoint_t* buffer, size_t size);
void search_append_next_byte(struct search_node* last, uint8_t* buffer, size_t size);
void search_debug_tree(struct search* base, struct search_node* node, size_t depth, int length, int stop);

int search_find(struct search* base, struct stream* text, file_offset_t* left, int* abort);
int search_find_check(struct search* base, struct stream* text);
int search_find_loop(struct search* base, struct search_node* node, struct stream* text);

void search_optimize(struct search* base, struct encoding* encoding);
int search_optimize_flat(struct encoding* encoding, struct search_node* node, search_optimize_callback callback_before, search_optimize_callback callback_after);

int search_optimize_reduce_branch_before(struct encoding* encoding, struct search_node* node);
int search_optimize_combine_branch_before(struct encoding* encoding, struct search_node* node);
int search_optimize_native_after(struct encoding* encoding, struct search_node* node);
int search_optimize_plain_before(struct encoding* encoding, struct search_node* node);
int search_optimize_plain_after(struct encoding* encoding, struct search_node* node);

void search_prepare(struct search* base, struct search_node* node, struct search_node* prev);
void search_prepare_skip(struct search* base, struct search_node* node);

void search_test(void);
#endif /* #ifndef TIPPSE_SEARCH_H */
