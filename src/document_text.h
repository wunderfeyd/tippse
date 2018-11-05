#ifndef TIPPSE_DOCUMENT_TEXT_H
#define TIPPSE_DOCUMENT_TEXT_H

#include <stdlib.h>
#include "types.h"

#include "document.h"
#include "encoding.h"
#include "stream.h"
#include "visualinfo.h"

// Scan at max the first 2MiB of the file for autocomplete information
#define TIPPSE_AUTOCOMPLETE_MAX (1024*1024*2)
#define TIPPSE_TAB_MAX (64)

struct document_text {
  struct document vtbl;             // virtual table of document
};

// Saved state between two render calls
struct document_text_render_info {
  bool_t append;                    // continue status?
  position_t x;                     // virtual screen X position, without scroll offset
  position_t y;                     // virtual screen Y position, without scroll offset
  position_t y_view;                // virtual screen Y position for rendering, without scroll offset
  struct range_tree_node* buffer;   // access to document buffer, current page in tree
  struct file_type* file_type;
  file_offset_t displacement;       // displacement for next Unicode character
  file_offset_t offset;             // file offset
  file_offset_t offset_sync;        // last position for keyword
  position_t xs;                    // relative screen X position
  position_t ys;                    // relative screen Y position
  int indentations;                 // number of normal identations in block
  int indentations_extra;           // number of extra identations in block, e.g. line wrapping
  int indentation;                  // number of normal identation in whole document
  int indentation_extra;            // number of extra identation in whole document
  position_t lines;                 // line in block
  position_t line;                  // line in whole document
  position_t columns;               // column in block
  position_t column;                // column in whole document
  file_offset_t characters;         // number of characters in block
  file_offset_t character;          // number of characters in whole document
  int visual_detail;                // flags for visual details
  position_t width;                 // screen width for rendering
  struct unicode_sequence sequence;
  int keyword_color;                // keyword color
  int keyword_length;               // keyword length remaining
  bool_t indented;                  // begin of line is indentation only?
  struct range_tree_node* selection_root; // root of selection buffer
  const struct range_tree_node* selection; // access to selection buffer, current page in tree
  file_offset_t selection_displacement; // position in current selection page
  int depth_new[VISUAL_BRACKET_MAX]; //depth of bracket matching at cursor position
  int depth_old[VISUAL_BRACKET_MAX]; //depth of bracket matching at page start
  int depth_line[VISUAL_BRACKET_MAX]; //depth of bracket matching at line
  struct visual_bracket brackets[VISUAL_BRACKET_MAX]; // block structure for bracket matching
  bool_t bracketed_line;            // bracket found on current line
  struct visual_bracket brackets_line[VISUAL_BRACKET_MAX]; // block structure for bracket matching at line
  struct stream stream;    // access to byte stream
  struct encoding_cache cache;      // access to Unicode cache
};

// Document position structure
struct document_text_position {
  int type;                             // visual seek type
  int clip;                             // clip to screen

  struct range_tree_node* buffer;       // access to document buffer, current page in tree
  file_offset_t displacement;           // offset in page

  file_offset_t offset;                 // offset in file

  position_t x;                         // position X of viewport
  position_t x_min;                     // left margin
  position_t x_max;                     // right margin

  position_t y;                         // position Y of viewport
  position_t y_drawn;                   // target row/line was found

  position_t line;                      // line in file
  position_t column;                    // column in line
  file_offset_t character;              // character in file

  int visual_detail;                    // visual detail at current stop

  size_t bracket;                       // bracket number
  int bracket_search;                   // bracket depth to search for
  int bracket_match;                    // type of bracket below the cursor

  int depth[VISUAL_BRACKET_MAX];        // Bracket information
  int depth_line[VISUAL_BRACKET_MAX];   // Bracket information
  int min_line[VISUAL_BRACKET_MAX];     // Bracket information
  position_t lines;                     // number of lines rendered in this page
  int indented;                         // cursor in indentation area
};

struct document* document_text_create(void);
void document_text_destroy(struct document* base);

void document_text_reset(struct document* base, struct splitter* splitter);

position_t document_text_line_width(struct splitter* splitter);
int document_text_incremental_update(struct document* base, struct splitter* splitter);
void document_text_draw(struct document* base, struct screen* screen, struct splitter* splitter);
void document_text_keypress(struct document* base, struct splitter* splitter, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y);
void document_text_keypress_line_select(struct document* base, struct splitter* splitter, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y);

void document_text_toggle_bookmark(struct document* base, struct splitter* splitter, file_offset_t offset);

void document_text_render_clear(struct document_text_render_info* render_info, position_t width, struct range_tree_node* selection);
void document_text_render_destroy(struct document_text_render_info* render_info);
void document_text_render_seek(struct document_text_render_info* render_info, struct range_tree_node* buffer, struct encoding* encoding, const struct document_text_position* in);
position_t document_text_render_lookahead_word_wrap(struct encoding_cache* cache, position_t max);
int document_text_split_buffer(struct range_tree_node* buffer, struct document_file* file);
int document_text_collect_span(struct document_text_render_info* render_info, const struct document_view* view, struct document_file* file, const struct document_text_position* restrict in, struct document_text_position* out, int dirty_pages, int cancel);
int document_text_prerender_span(struct document_text_render_info* render_info, struct screen* screen, struct splitter* splitter, const struct document_view* view, struct document_file* file, const struct document_text_position* in, struct document_text_position* out, int dirty_pages, int cancel);
int document_text_render_span(struct document_text_render_info* render_info, struct screen* screen, struct splitter* splitter, const struct document_view* view, struct document_file* file, const struct document_text_position* in, struct document_text_position* out, int dirty_pages, int cancel);

file_offset_t document_text_cursor_position_partial(struct document_text_render_info* render_info, struct splitter* splitter, struct document_text_position* in, struct document_text_position* out, int wrap, int cancel);
file_offset_t document_text_cursor_position(struct splitter* splitter, struct document_text_position* in, struct document_text_position* out, int wrap, int cancel);

void document_text_lower_indentation(struct document* base, struct splitter* splitter, file_offset_t low, file_offset_t high);
void document_text_raise_indentation(struct document* base, struct splitter* splitter, file_offset_t low, file_offset_t high, int empty_lines);

void document_text_move_block(struct document* base, struct splitter* splitter, file_offset_t low, file_offset_t high, int up);

void document_text_select_line(struct document* base, struct splitter* splitter);

int document_text_mark_brackets(struct document* base, struct screen* screen, struct splitter* splitter, struct document_text_position* cursor);

void document_text_goto(struct document* base, struct splitter* splitter, position_t line, position_t column);
file_offset_t document_text_line_start_offset(struct document* base, struct splitter* splitter);

file_offset_t document_text_word_transition_next(struct document* base, struct splitter* splitter, file_offset_t offset);
file_offset_t document_text_word_transition_prev(struct document* base, struct splitter* splitter, file_offset_t offset);

void document_text_transform(struct document* base, struct trie* transformation, struct document_file* file, file_offset_t from, file_offset_t to);

#endif /* #ifndef TIPPSE_DOCUMENT_TEXT_H */
