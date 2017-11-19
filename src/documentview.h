#ifndef TIPPSE_DOCUMENTVIEW_H
#define TIPPSE_DOCUMENTVIEW_H

#include <stdlib.h>
#include "types.h"

struct range_tree_node;

struct document_view {
  file_offset_t offset;                 // file offset

  position_t cursor_x;                  // cursor X position, without scroll offset
  position_t cursor_y;                  // cursor Y position, without scroll offset

  file_offset_t selection_start;        // start position of selection
  file_offset_t selection_end;          // end position of selection
  file_offset_t selection_low;          // smallest position of selection
  file_offset_t selection_high;         // highest position of selection
  struct range_tree_node* selection;    // information for multiple selections

  position_t scroll_x;                  // scroll X offset
  position_t scroll_y;                  // scroll Y offset
  position_t scroll_x_old;              // scroll X offset, last renedering
  position_t scroll_y_old;              // scroll Y offset, last renedering
  position_t scroll_y_max;              // maximum scroll Y offset
  position_t address_width;             // width of address column, used for line number too
  int show_scrollbar;                   // show scrollbar?
  int show_invisibles;                  // show invisibles?
  int wrapping;                         // show word wrapping?
  int continuous;                       // ignore line break?
  int line_select;                      // show whole line selected?
};

#include "rangetree.h"
#include "documentfile.h"

struct document_view* document_view_create(void);
void document_view_destroy(struct document_view* base);
void document_view_reset(struct document_view* base, struct document_file* file);
void document_view_clone(struct document_view* dst, struct document_view* src, struct document_file* file);
void document_view_filechange(struct document_view* base, struct document_file* file);

#endif /* #ifndef TIPPSE_DOCUMENTVIEW_H */
