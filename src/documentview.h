#ifndef TIPPSE_DOCUMENTVIEW_H
#define TIPPSE_DOCUMENTVIEW_H

#include <stdlib.h>
#include "types.h"
#include "rangetree.h"

struct document_view {
  file_offset_t offset;

  position_t cursor_x;
  position_t cursor_y;

  file_offset_t selection_start;
  file_offset_t selection_end;

  file_offset_t selection_low;
  file_offset_t selection_high;
  struct range_tree_node* selection;

  position_t scroll_x;
  position_t scroll_y;
  position_t scroll_x_old;
  position_t scroll_y_old;
  position_t scroll_y_max;
  position_t address_width;
  int show_scrollbar;
  int show_invisibles;
  int wrapping;
  int continuous;
  int line_select;
};

#include "documentfile.h"

struct document_view* document_view_create(void);
void document_view_destroy(struct document_view* view);
void document_view_reset(struct document_view* view, struct document_file* file);
void document_view_clone(struct document_view* dst, struct document_view* src, struct document_file* file);
void document_view_filechange(struct document_view* view, struct document_file* file);

#endif /* #ifndef TIPPSE_DOCUMENTVIEW_H */
