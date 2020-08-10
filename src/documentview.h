#ifndef TIPPSE_DOCUMENTVIEW_H
#define TIPPSE_DOCUMENTVIEW_H

#include <stdlib.h>
#include "types.h"

struct document_view {
  file_offset_t offset;                 // file offset
  file_offset_t offset_calculated;      // last offset calcuted by renderer

  position_t cursor_x;                  // cursor X position, without scroll offset
  position_t cursor_y;                  // cursor Y position, without scroll offset

  file_offset_t selection_start;        // start position of selection
  file_offset_t selection_end;          // end position of selection
  int selection_reset;                  // Selection was changed
  struct range_tree_node* selection;    // information for multiple selections

  int line_cut;                         // Previous action was a line cut

  position_t scroll_x;                  // scroll X offset
  position_t scroll_y;                  // scroll Y offset
  position_t scroll_x_old;              // scroll X offset, last rendering
  position_t scroll_y_old;              // scroll Y offset, last rendering
  position_t scroll_y_max;              // maximum scroll Y offset
  position_t address_width;             // width of address column, used for line number too
  int bracket_indentation;              // the next closing bracket could modify identation
  int show_scrollbar;                   // show scrollbar?
  int64_t scrollbar_timeout;            // time of scrollbar removal
  int show_invisibles;                  // show invisibles?
  int wrapping;                         // show word wrapping?
  int line_select;                      // show whole line selected?
};

struct document_view* document_view_create(void);
void document_view_destroy(struct document_view* base);
void document_view_reset(struct document_view* base, struct document_file* file, int defaults);
void document_view_clone(struct document_view* dst, struct document_view* src, struct document_file* file);
void document_view_filechange(struct document_view* base, struct document_file* file, int defaults);

void document_view_select_all(struct document_view* base, struct document_file* file);
void document_view_select_nothing(struct document_view* base, struct document_file* file);
int document_view_select_active(struct document_view* base);
int document_view_select_next(struct document_view* base, file_offset_t offset, file_offset_t* low, file_offset_t* high);
void document_view_select_range(struct document_view* base, file_offset_t start, file_offset_t end, int inserter);
void document_view_select_invert(struct document_view* base);

#endif /* #ifndef TIPPSE_DOCUMENTVIEW_H */
