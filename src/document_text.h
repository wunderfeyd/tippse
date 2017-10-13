#ifndef TIPPSE_DOCUMENT_TEXT_H
#define TIPPSE_DOCUMENT_TEXT_H

#include <stdlib.h>

struct document_text;

#include "misc.h"
#include "trie.h"
#include "filetype.h"
#include "rangetree.h"
#include "screen.h"
#include "clipboard.h"
#include "documentview.h"
#include "documentfile.h"
#include "encoding.h"
#include "unicode.h"
#include "document.h"
#include "splitter.h"
#include "documentundo.h"

struct document_text {
  struct document vtbl;

  int keep_status;
};

// Saved state between two render calls
struct document_text_render_info {
  int append;
  position_t x;
  position_t y;
  position_t y_view;
  struct range_tree_node* buffer;
  file_offset_t displacement;
  file_offset_t offset;
  file_offset_t offset_sync;
  position_t xs;
  position_t ys;
  int indentations;
  int indentations_extra;
  int indentation;
  int indentation_extra;
  position_t lines;
  position_t line;
  position_t columns;
  position_t column;
  file_offset_t characters;
  file_offset_t character;
  int visual_detail;
  int draw_indentation;
  position_t width;
  int keyword_color;
  int keyword_length;
  int whitespaced;
  int indented;
  struct encoding_stream stream;
  struct encoding_cache cache;
  int depth_new[VISUAL_BRACKET_MAX];
  int depth_old[VISUAL_BRACKET_MAX];
  int depth_line[VISUAL_BRACKET_MAX];
  struct visual_bracket brackets[VISUAL_BRACKET_MAX];
  struct visual_bracket brackets_line[VISUAL_BRACKET_MAX];
};

// Document position structure
struct document_text_position {
  int type;                             // Visual seek type
  int clip;                             // Clip to screen

  struct range_tree_node* buffer;       // Page
  file_offset_t displacement;           // Offset in page

  file_offset_t offset;                 // Offset in file

  position_t x;                         // Position X of viewport
  position_t x_min;                     // Left margin
  position_t x_max;                     // Right margin

  position_t y;                         // Position Y of viewport
  position_t y_drawn;                   // Target row/line was found

  position_t line;                      // Line in file
  position_t column;                    // Column in line
  file_offset_t character;              // Character in file

  int visual_detail;                    // Visual detail at current stop

  size_t bracket;                       // Bracket number
  int bracket_search;                   // Bracket depth to search for
  int bracket_match;                    // Type of bracket below the cursor

  int depth[VISUAL_BRACKET_MAX];        // Bracket information
  int depth_line[VISUAL_BRACKET_MAX];   // Bracket information
  int min_line[VISUAL_BRACKET_MAX];     // Bracket information
  position_t lines;                     // Number of lines rendered in this page
  int indented;                         // Cursor in indentation area
};

struct document* document_text_create(void);
void document_text_destroy(struct document* base);

void document_text_reset(struct document* base, struct splitter* splitter);
int document_text_incremental_update(struct document* base, struct splitter* splitter);
void document_text_draw(struct document* base, struct screen* screen, struct splitter* splitter);
void document_text_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y);

void document_text_toggle_bookmark(struct document* base, struct splitter* splitter, file_offset_t offset);

void document_text_render_clear(struct document_text_render_info* render_info, position_t width);
void document_text_render_seek(struct document_text_render_info* render_info, struct range_tree_node* buffer, struct encoding* encoding, struct document_text_position* in);
position_t document_text_render_lookahead_word_wrap(struct document_file* file, struct encoding_cache* cache, position_t max);
int document_text_render_span(struct document_text_render_info* render_info, struct screen* screen, struct splitter* splitter, struct document_view* view, struct document_file* file, struct document_text_position* in, struct document_text_position* out, int dirty_pages, int cancel);

file_offset_t document_text_cursor_position_partial(struct document_text_render_info* render_info, struct splitter* splitter, struct document_text_position* in, struct document_text_position* out, int wrap, int cancel);
file_offset_t document_text_cursor_position(struct splitter* splitter, struct document_text_position* in, struct document_text_position* out, int wrap, int cancel);

void document_text_lower_indentation(struct document* base, struct splitter* splitter, file_offset_t low, file_offset_t high);
void document_text_raise_indentation(struct document* base, struct splitter* splitter, file_offset_t low, file_offset_t high, int empty_lines);

int document_text_mark_brackets(struct document* base, struct screen* screen, struct splitter* splitter, struct document_text_position* cursor);

#endif /* #ifndef TIPPSE_DOCUMENT_TEXT_H */
