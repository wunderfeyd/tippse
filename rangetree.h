#ifndef __TIPPSE_RANGETREE__
#define __TIPPSE_RANGETREE__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"

struct range_tree_node;

#include "utf8.h"
#include "fragment.h"
#include "visualinfo.h"
#include "filetype.h"

#define TREE_BLOCK_LENGTH_MAX 1024
#define TREE_BLOCK_LENGTH_MIN 16

#define TIPPSE_INSERTER_BEFORE 1
#define TIPPSE_INSERTER_AFTER 2
#define TIPPSE_INSERTER_READONLY 4
#define TIPPSE_INSERTER_ESCAPE 8
#define TIPPSE_INSERTER_AUTO 16

struct range_tree_node {
  struct range_tree_node* parent;
  struct range_tree_node* side[2];
  struct fragment* buffer;
  file_offset_t offset;
  file_offset_t length;
  size_t subs;
  int depth;
  int inserter;
  struct visual_info visuals;
};

void range_tree_print(struct range_tree_node* node, int depth, int side);
void range_tree_check(struct range_tree_node* node);
void range_tree_print_root(struct range_tree_node* node, int depth, int side);
void range_tree_update_calc(struct range_tree_node* node);
void range_tree_update_calc_all(struct range_tree_node* node);
void range_tree_clear(struct range_tree_node* node);
struct range_tree_node* range_tree_new_node(struct range_tree_node* parent, struct range_tree_node* side0, struct range_tree_node* side1, struct fragment* buffer, file_offset_t offset, file_offset_t length, int inserter);
struct range_tree_node* range_tree_first(struct range_tree_node* node);
struct range_tree_node* range_tree_last(struct range_tree_node* node);
struct range_tree_node* range_tree_next(struct range_tree_node* node);
struct range_tree_node* range_tree_prev(struct range_tree_node* node);
void range_tree_exchange(struct range_tree_node* node, struct range_tree_node* old, struct range_tree_node* new);
struct range_tree_node* range_tree_reorder(struct range_tree_node* node);
struct range_tree_node* range_tree_update(struct range_tree_node* node);
struct range_tree_node* range_tree_find_visual(struct range_tree_node* node, int find_type, file_offset_t find_offset, int find_x, int find_y, int find_line, int find_column, file_offset_t* offset, int* x, int* y, int* line, int* column, int* indentation, int* indentation_extra);
file_offset_t range_tree_offset(struct range_tree_node* node);
file_offset_t range_tree_distance_offset(struct range_tree_node* root, struct range_tree_node* start, struct range_tree_node* end);
struct range_tree_node* range_tree_find_offset(struct range_tree_node* node, file_offset_t offset, file_offset_t* diff);
void range_tree_retext(struct range_tree_node* node, struct file_type* type);
struct range_tree_node* range_tree_compact(struct range_tree_node* root, struct file_type* type, struct range_tree_node* first, struct range_tree_node* last);
struct range_tree_node* range_tree_insert(struct range_tree_node* root, struct file_type* type, file_offset_t offset, struct fragment* buffer, file_offset_t buffer_offset, file_offset_t buffer_length, int inserter);
struct range_tree_node* range_tree_insert_split(struct range_tree_node* root, struct file_type* type, file_offset_t offset, const char* text, size_t length, int inserter, struct range_tree_node** inserts);
struct range_tree_node* range_tree_delete(struct range_tree_node* root, struct file_type* type, file_offset_t offset, file_offset_t length, int inserter);
struct range_tree_node* range_tree_copy(struct range_tree_node* root, struct file_type* type, file_offset_t offset, file_offset_t length);
struct range_tree_node* range_tree_paste(struct range_tree_node* root, struct file_type* type, struct range_tree_node* copy, file_offset_t offset);
char* range_tree_raw(struct range_tree_node* root, file_offset_t start, file_offset_t end);

#endif /* #ifndef __TIPPSE_RANGETREE__ */
