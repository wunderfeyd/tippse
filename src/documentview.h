#ifndef TIPPSE_DOCUMENTVIEW_H
#define TIPPSE_DOCUMENTVIEW_H

#include <stdlib.h>
#include "types.h"
#include "library/rangetree.h"

struct document_view {
  file_offset_t offset;                 // file offset
  file_offset_t offset_calculated;      // last offset calcuted by renderer

  position_t cursor_x;                  // cursor X position, without scroll offset
  position_t cursor_y;                  // cursor Y position, without scroll offset
  position_t seek_x;                    // cursor X position, without scroll offset
  position_t seek_y;                    // cursor Y position, without scroll offset

  file_offset_t selection_start;        // start position of selection
  file_offset_t selection_end;          // end position of selection
  int selection_reset;                  // Selection was changed
  struct range_tree selection;          // information for multiple selections

  int line_cut;                         // Previous action was a line cut

  position_t max_width;                 // last known render size
  position_t client_width;              // last known width of viewport
  position_t client_height;             // last known height of viewport

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
  int overwrite;                        // overwrite enabled?
  int update_search;                    // reload search text from selection if invoked?

  int uid;                              // associated view uid for visual caching
  struct range_tree visuals;            // visualization index
};

struct document_view* document_view_create(void);
void document_view_create_inplace(struct document_view* base);
void document_view_destroy(struct document_view* base);
void document_view_reset(struct document_view* base, struct document_file* file, int defaults);
struct document_view* document_view_clone(struct document_view* base, struct document_file* file);
void document_view_filechange(struct document_view* base, struct document_file* file, int defaults);

void document_view_select_all(struct document_view* base, struct document_file* file, int update_search);
void document_view_select_nothing(struct document_view* base, struct document_file* file, int update_search);
int document_view_select_active(struct document_view* base);
int document_view_select_next(struct document_view* base, file_offset_t offset, file_offset_t* low, file_offset_t* high);
void document_view_select_range(struct document_view* base, file_offset_t start, file_offset_t end, int inserter, int update_search);
void document_view_select_invert(struct document_view* base, int update_search);

struct visual_info* document_view_visual_create(struct document_view* base, struct range_tree_node* node);
void document_view_visual_destroy(struct document_view* base, struct range_tree_node* node);
void document_view_visual_clear(struct document_view* base);

#endif /* #ifndef TIPPSE_DOCUMENTVIEW_H */
