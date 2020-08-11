#ifndef TIPPSE_RANGETREE_H
#define TIPPSE_RANGETREE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"

// 1EiB page size for file load
#define TREE_BLOCK_LENGTH_MAX 1024ull*1024ull*1024ull*1024ull*1024ull
// 1GiB page size for direct memory access operations
#define TREE_BLOCK_LENGTH_MID 1024*1024*1024
// 4kiB page size for file render split
#define TREE_BLOCK_LENGTH_MIN 4096

#define TIPPSE_INSERTER_LEAF 1
#define TIPPSE_INSERTER_FILE 2
#define TIPPSE_INSERTER_MARK 4
#define TIPPSE_INSERTER_HIGHLIGHT 8
#define TIPPSE_INSERTER_NOFUSE 16
#define TIPPSE_INSERTER_VISUALS 32

#define TIPPSE_INSERTER_HIGHLIGHT_COLOR_SHIFT 16

#include "visualinfo.h"

struct range_tree_node {
  struct range_tree_node* parent;   // parent node
  struct range_tree_node* side[2];  // binary split (left and right side)
  struct range_tree_node* next;     // Next element in overlay list for the leaf nodes
  struct range_tree_node* prev;     // Previous element in overlay list for the leaf nodes
  file_offset_t length;             // Length of node
  int depth;                        // Current depth level
  int inserter;                     // Combined flags to modify interaction rules

  // TODO: shrink structure dynamically depending on type of tree
  int64_t fuse_id;                  // Only fuse neighbor nodes with the same fuse identification
  struct fragment* buffer;          // Buffer to file content
  file_offset_t offset;             // Relative start offset to the beginning of the file content buffer
  void* user_data;                  // User defined data
  struct visual_info visuals;       // Visual information
};

struct range_tree {
  struct range_tree_node* root;
};

struct fragment;

struct range_tree* range_tree_create();
void range_tree_create_inplace(struct range_tree* base);
void range_tree_destroy(struct range_tree* base, struct document_file* file);
void range_tree_destroy_inplace(struct range_tree* base, struct document_file* file);

void range_tree_node_print(const struct range_tree_node* node, int depth, int side);
void range_tree_node_check(const struct range_tree_node* node);
void range_tree_node_print_root(const struct range_tree_node* node, int depth, int side);
void range_tree_node_update_calc(struct range_tree_node* node);
void range_tree_node_update_calc_all(struct range_tree_node* node);
void range_tree_node_destroy(struct range_tree_node* node, struct document_file* file);
struct range_tree_node* range_tree_node_create(struct range_tree_node* parent, struct range_tree* tree, struct range_tree_node* side0, struct range_tree_node* side1, struct fragment* buffer, file_offset_t offset, file_offset_t length, int inserter, int64_t fuse_id, struct document_file* file, void* user_data);
struct range_tree_node* range_tree_node_first(struct range_tree_node* node);
struct range_tree_node* range_tree_node_last(struct range_tree_node* node);
TIPPSE_INLINE struct range_tree_node* range_tree_node_next(const struct range_tree_node* node) {return node?node->next:NULL;}
TIPPSE_INLINE struct range_tree_node* range_tree_node_prev(const struct range_tree_node* node) {return node?node->prev:NULL;}
TIPPSE_INLINE file_offset_t range_tree_node_length(const struct range_tree_node* node) {return node?node->length:0;}
void range_tree_node_exchange(struct range_tree_node* node, struct range_tree_node* old, struct range_tree_node* update);
struct range_tree_node* range_tree_node_rotate(struct range_tree_node* node, int side);
struct range_tree_node* range_tree_node_balance(struct range_tree_node* node);
struct range_tree_node* range_tree_node_update(struct range_tree_node* node);
struct range_tree_node* range_tree_node_find_visual(struct range_tree_node* node, int find_type, file_offset_t find_offset, position_t find_x, position_t find_y, position_t find_line, position_t find_column, file_offset_t* offset, position_t* x, position_t* y, position_t* line, position_t* column, int* indentation, int* indentation_extra, file_offset_t* character, int retry, file_offset_t before);
int range_tree_node_find_bracket(const struct range_tree_node* node, size_t bracket);
struct range_tree_node* range_tree_node_find_bracket_forward(struct range_tree_node* node, size_t bracket, int search);
struct range_tree_node* range_tree_node_find_bracket_backward(struct range_tree_node* node, size_t bracket, int search);
void range_tree_node_find_bracket_lowest(const struct range_tree_node* node, int* mins, const struct range_tree_node* last);
struct range_tree_node* range_tree_node_find_indentation_last(struct range_tree_node* node, position_t lines, struct range_tree_node* last);
int range_tree_node_find_indentation(const struct range_tree_node* node);
int range_tree_node_find_whitespaced(const struct range_tree_node* node);
file_offset_t range_tree_node_offset(const struct range_tree_node* node);
struct range_tree_node* range_tree_node_find_offset(struct range_tree_node* node, file_offset_t offset, file_offset_t* diff);
void range_tree_node_invalidate(struct range_tree_node* node);
struct range_tree_node* range_tree_node_fuse(struct range_tree_node* root, struct range_tree_node* first, struct range_tree_node* last, struct document_file* file);
struct range_tree_node* range_tree_node_insert(struct range_tree_node* root, struct range_tree* tree, file_offset_t offset, struct fragment* buffer, file_offset_t buffer_offset, file_offset_t buffer_length, int inserter, int64_t fuse_id, struct document_file* file, void* user_data);
struct range_tree_node* range_tree_node_insert_split(struct range_tree_node* root, struct range_tree* tree, file_offset_t offset, const uint8_t* text, size_t length, int inserter);
struct range_tree_node* range_tree_node_delete(struct range_tree_node* root, struct range_tree* tree, file_offset_t offset, file_offset_t length, int inserter, struct document_file* file);
struct range_tree_node* range_tree_node_copy_insert(struct range_tree_node* root_from, file_offset_t offset_from, struct range_tree_node* root_to, struct range_tree* tree_to, file_offset_t offset_to, file_offset_t length, struct document_file* file);
struct range_tree* range_tree_node_copy(struct range_tree_node* root, file_offset_t offset, file_offset_t length);
void range_tree_node_paste(struct range_tree* tree, struct range_tree_node* copy, file_offset_t offset, struct document_file* file);
uint8_t* range_tree_node_raw(struct range_tree_node* root, file_offset_t start, file_offset_t end);

struct range_tree_node* range_tree_node_split(struct range_tree_node* root, struct range_tree* tree, struct range_tree_node** node, file_offset_t split, struct document_file* file, int invalidate);
void range_tree_node_mark(struct range_tree* tree, file_offset_t offset, file_offset_t length, int inserter);
int range_tree_node_marked(const struct range_tree_node* node, file_offset_t offset, file_offset_t length, int inserter);
struct range_tree_node* range_tree_node_invert_mark(struct range_tree_node* node, int inserter);
void range_tree_node_static(struct range_tree* tree, file_offset_t length, int inserter);
void range_tree_node_resize(struct range_tree* tree, file_offset_t length, int inserter);
void range_tree_node_expand(struct range_tree* tree, file_offset_t offset, file_offset_t length);
void range_tree_node_reduce(struct range_tree* tree, file_offset_t offset, file_offset_t length);
int range_tree_node_marked_next(struct range_tree_node* root, file_offset_t offset, file_offset_t* low, file_offset_t* high, int skip_first);
int range_tree_node_marked_prev(struct range_tree_node* root, file_offset_t offset, file_offset_t* low, file_offset_t* high, int skip_first);
#endif /* #ifndef TIPPSE_RANGETREE_H */
