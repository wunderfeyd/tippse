/* Tippse - Document view - Creates a view for a file (allows multiple views per file) */

#include "documentview.h"

struct document_view* document_view_create() {
  struct document_view* view = malloc(sizeof(struct document_view));
  view->selection = NULL;
  return view;
}

void document_view_destroy(struct document_view* view) {
  if (view->selection) {
    range_tree_destroy(view->selection);
  }

  free(view);
}

void document_view_reset(struct document_view* view, struct document_file* file) {
  view->offset = 0;
  view->cursor_x = 0;
  view->cursor_y = 0;
  view->scroll_x = 0;
  view->scroll_y = 0;
  view->scroll_x_old = 0;
  view->scroll_y_old = 0;
  view->selection_start = ~0;
  view->selection_end = ~0;
  view->selection_low = ~0;
  view->selection_high = ~0;
  view->line_select = 0;
  document_view_filechange(view, file);
}

void document_view_clone(struct document_view* dst, struct document_view* src, struct document_file* file) {
  if (dst->selection) {
    range_tree_destroy(dst->selection);
  }

  *dst = *src;
  dst->selection = NULL;
  document_view_filechange(dst, file);
}

void document_view_filechange(struct document_view* view, struct document_file* file) {
  view->wrapping = file->defaults.wrapping;
  view->showall = file->defaults.showall;
  view->continuous = file->defaults.continuous;
  view->selection = range_tree_static(view->selection, file->buffer?file->buffer->length:0, 0);
}
