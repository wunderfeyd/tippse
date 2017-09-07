#ifndef __TIPPSE_DOCUMENTVIEW__
#define __TIPPSE_DOCUMENTVIEW__

#include <stdlib.h>
#include "types.h"
#include "rangetree.h"

struct document_view {
  file_offset_t offset;

  int cursor_x;
  int cursor_y;

  file_offset_t selection_start;
  file_offset_t selection_end;

  file_offset_t selection_low;
  file_offset_t selection_high;
  struct range_tree_node* selection;

  int scroll_x;
  int scroll_y;

  int scroll_x_old;
  int scroll_y_old;

  int showall;
  int wrapping;
  int continuous;
  int line_select;
};

#include "documentfile.h"

struct document_view* document_view_create();
void document_view_destroy(struct document_view* view);
void document_view_reset(struct document_view* view, struct document_file* file);
void document_view_clone(struct document_view* dst, struct document_view* src, struct document_file* file);
void document_view_filechange(struct document_view* view, struct document_file* file);

#endif /* #ifndef __TIPPSE_DOCUMENTVIEW__ */
