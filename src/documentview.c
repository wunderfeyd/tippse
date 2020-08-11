// Tippse - Document view - Creates a view for a file (allows multiple views per file)

#include "documentview.h"

#include "documentfile.h"
#include "rangetree.h"

// Create view
struct document_view* document_view_create(void) {
  struct document_view* base = (struct document_view*)malloc(sizeof(struct document_view));
  range_tree_create_inplace(&base->selection, NULL, 0);
  return base;
}

// Destroy view
void document_view_destroy(struct document_view* base) {
  range_tree_destroy_inplace(&base->selection);
  free(base);
}

// Reset view
void document_view_reset(struct document_view* base, struct document_file* file, int defaults) {
  base->offset = 0;
  base->offset_calculated = FILE_OFFSET_T_MAX;
  base->selection_reset = 1;
  base->line_cut = 0;
  base->cursor_x = 0;
  base->cursor_y = 0;
  base->scroll_x = 0;
  base->scroll_y = 0;
  base->scroll_x_old = 0;
  base->scroll_y_old = 0;
  base->scroll_y_max = 0;
  base->show_scrollbar = 0;
  base->scrollbar_timeout = 0;
  base->selection_start = FILE_OFFSET_T_MAX;
  base->selection_end = FILE_OFFSET_T_MAX;
  document_view_filechange(base, file, defaults);
}

// Clone view
void document_view_clone(struct document_view* dst, struct document_view* src, struct document_file* file) {
  range_tree_destroy_inplace(&dst->selection);

  *dst = *src;
  range_tree_create_inplace(&dst->selection, NULL, 0);
  struct range_tree* copy = range_tree_node_copy(&src->selection, 0, range_tree_node_length(src->selection.root), file);
  dst->selection.root = copy->root;
  copy->root = NULL; // TODO: Not nice
  range_tree_destroy(copy);
  document_view_filechange(dst, file, 0);
}

// Copy file defaults
void document_view_filechange(struct document_view* base, struct document_file* file, int defaults) {
  if (defaults) {
    base->wrapping = file->defaults.wrapping;
    base->show_invisibles = file->defaults.invisibles;
    base->address_width = file->defaults.address_width;
  }

  base->line_select = file->line_select;
  base->bracket_indentation = 0;
  range_tree_node_resize(&base->selection, range_tree_node_length(file->buffer.root), 0);
}

// Select all
void document_view_select_all(struct document_view* base, struct document_file* file) {
  range_tree_node_static(&base->selection, range_tree_node_length(file->buffer.root), TIPPSE_INSERTER_MARK);
  base->selection_reset = 1;
}

// Select nothing
void document_view_select_nothing(struct document_view* base, struct document_file* file) {
  range_tree_node_static(&base->selection, range_tree_node_length(file->buffer.root), 0);
  base->selection_reset = 1;
}

// Active selection?
int document_view_select_active(struct document_view* base) {
  return (base->selection.root && (base->selection.root->inserter&TIPPSE_INSERTER_MARK))?1:0;
}

// Retrieve next active selection
int document_view_select_next(struct document_view* base, file_offset_t offset, file_offset_t* low, file_offset_t* high) {
  return range_tree_node_marked_next(&base->selection, offset, low, high, 0);
}

// Activate or deactivate selection for specified range
void document_view_select_range(struct document_view* base, file_offset_t start, file_offset_t end, int inserter) {
  if (start==end) {
    return;
  }

  if (start<end) {
    range_tree_node_mark(&base->selection, start, end-start, inserter);
  } else {
    range_tree_node_mark(&base->selection, end, start-end, inserter);
  }
}

// Invert selection
void document_view_select_invert(struct document_view* base) {
  base->selection.root = range_tree_node_invert_mark(base->selection.root, &base->selection, TIPPSE_INSERTER_MARK);
}
