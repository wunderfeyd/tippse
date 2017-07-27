/* Tippse - Document view - Creates a view for a file (allows multiple views per file) */

#include "documentview.h"

void document_view_reset(struct document_view* view) {
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
  view->wrapping = 0;
  view->showall = 0;
}