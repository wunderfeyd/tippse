/* Tippse - Document view - Creates a view for a file (allows multiple views per file) */

#include "documentview.h"

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
  view->wrapping = file->defaults.wrapping;
  view->showall = file->defaults.showall;
  view->continuous = file->defaults.continuous;
  view->line_select = 0;
  view->selection = range_tree_static(view->selection, file->buffer?file->buffer->length:0, 0);
}