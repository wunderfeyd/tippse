// Tippse - Document view - Creates a view for a file (allows multiple views per file)

#include "documentview.h"

// Create view
struct document_view* document_view_create(void) {
  struct document_view* base = malloc(sizeof(struct document_view));
  base->selection = NULL;
  return base;
}

// Destroy view
void document_view_destroy(struct document_view* base) {
  if (base->selection) {
    range_tree_destroy(base->selection);
  }

  free(base);
}

// Reset view
void document_view_reset(struct document_view* base, struct document_file* file) {
  base->offset = 0;
  base->cursor_x = 0;
  base->cursor_y = 0;
  base->scroll_x = 0;
  base->scroll_y = 0;
  base->scroll_x_old = 0;
  base->scroll_y_old = 0;
  base->scroll_y_max = 0;
  base->address_width = 0;
  base->show_scrollbar = 0;
  base->selection_start = FILE_OFFSET_T_MAX;
  base->selection_end = FILE_OFFSET_T_MAX;
  base->selection_low = FILE_OFFSET_T_MAX;
  base->selection_high = FILE_OFFSET_T_MAX;
  base->line_select = 0;
  document_view_filechange(base, file);
}

// Clone view
void document_view_clone(struct document_view* dst, struct document_view* src, struct document_file* file) {
  if (dst->selection) {
    range_tree_destroy(dst->selection);
  }

  *dst = *src;
  dst->selection = NULL;
  document_view_filechange(dst, file);
}

// Copy file defaults
void document_view_filechange(struct document_view* base, struct document_file* file) {
  base->wrapping = file->defaults.wrapping;
  base->show_invisibles = file->defaults.invisibles;
  base->continuous = file->defaults.continuous;
  base->selection = range_tree_static(base->selection, file->buffer?file->buffer->length:0, 0);
}
