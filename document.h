#ifndef __TIPPSE_DOCUMENT__
#define __TIPPSE_DOCUMENT__

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

struct document;

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

#define TAB_WIDTH 2

#define TIPPSE_KEY_MOD_SHIFT 1
#define TIPPSE_KEY_MOD_CTRL 2
#define TIPPSE_KEY_MOD_ALT 4

#define TIPPSE_KEY_UP -1
#define TIPPSE_KEY_DOWN -2
#define TIPPSE_KEY_RIGHT -3
#define TIPPSE_KEY_LEFT -4
#define TIPPSE_KEY_CLOSE -5
#define TIPPSE_KEY_PAGEUP -6
#define TIPPSE_KEY_PAGEDOWN -7
#define TIPPSE_KEY_BACKSPACE -8
#define TIPPSE_KEY_DELETE -9
#define TIPPSE_KEY_INSERT -10
#define TIPPSE_KEY_FIRST -11
#define TIPPSE_KEY_LAST -12
#define TIPPSE_KEY_HOME -13
#define TIPPSE_KEY_END -14
#define TIPPSE_KEY_SEARCH -15
#define TIPPSE_KEY_SEARCH_NEXT -16
#define TIPPSE_KEY_UNDO -17
#define TIPPSE_KEY_REDO -18
#define TIPPSE_KEY_CUT -19
#define TIPPSE_KEY_COPY -20
#define TIPPSE_KEY_PASTE -21
#define TIPPSE_KEY_TAB -22
#define TIPPSE_KEY_UNTAB -23
#define TIPPSE_KEY_SAVE -24
#define TIPPSE_KEY_BRACKET_PASTE_START -25
#define TIPPSE_KEY_BRACKET_PASTE_STOP -26
#define TIPPSE_KEY_TIPPSE_MOUSE_INPUT -27
#define TIPPSE_KEY_SEARCH_PREV -28
#define TIPPSE_KEY_OPEN -29
#define TIPPSE_KEY_NEW_VERT_TAB -30
#define TIPPSE_KEY_SHOWALL -31
#define TIPPSE_KEY_BROWSER -32

#define TIPPSE_MOUSE_LBUTTON 1
#define TIPPSE_MOUSE_RBUTTON 2
#define TIPPSE_MOUSE_MBUTTON 4
#define TIPPSE_MOUSE_WHEEL_UP 8
#define TIPPSE_MOUSE_WHEEL_DOWN 16

struct document {
  struct document_file* file;
  struct document_view view;

  int show_scrollbar;
  int keep_status;
  int content_document;
};

#include "splitter.h"
#include "documentundo.h"

// Saved state between two render calls
struct document_render_info {
  int x;
  int y;
  int y_view;
  struct range_tree_node* buffer;
  file_offset_t displacement;
  file_offset_t offset;
  file_offset_t offset_sync;
  int xs;
  int ys;
  int indentations;
  int indentations_extra;
  int indentation;
  int indentation_extra;
  int lines;
  int line;
  int columns;
  int column;
  file_offset_t characters;
  file_offset_t character;
  int visual_detail;
  int draw_indentation;
  int width;
  int keyword_color;
  int keyword_length;
  int whitespaced;
  struct encoding_stream stream;
  struct encoding_cache cache;
};

// Document position structure
struct document_render_info_position {
  int type;                             // Visual seek type
  int clip;                             // Clip to screen

  struct range_tree_node* buffer;       // Page
  file_offset_t displacement;           // Offset in page

  file_offset_t offset;                 // Offset in file

  int x;                                // Position X of viewport
  int x_min;                            // Left margin
  int x_max;                            // Right margin

  int y;                                // Position Y of viewport
  int y_drawn;                          // Target row/line was found

  int line;                             // Line in file
  int column;                           // Column in line
  file_offset_t character;              // Character in file
};

void document_render_info_clear(struct document_render_info* render_info, int width);
void document_render_info_seek(struct document_render_info* render_info, struct range_tree_node* buffer, struct encoding* encoding, struct document_render_info_position* in);
int document_render_lookahead_word_wrap(struct document_file* file, struct encoding_cache* cache, int max);
int document_render_info_span(struct document_render_info* render_info, struct screen* screen, struct splitter* splitter, struct document_view* view, struct document_file* file, struct document_render_info_position* in, struct document_render_info_position* out, int dirty_pages, int cancel);

int document_compare(struct range_tree_node* left, file_offset_t displacement_left, struct range_tree_node* right_root, file_offset_t length);
void document_search(struct splitter* splitter, struct document* document, struct range_tree_node* text, file_offset_t length, int forward);
file_offset_t document_cursor_position(struct splitter* splitter, struct document_render_info_position* in, struct document_render_info_position* out, int wrap, int cancel);
int document_incremental_update(struct splitter* splitter);
void document_draw(struct screen* screen, struct splitter* splitter);

void document_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_insert(struct document* document, file_offset_t offset, const uint8_t* text, size_t length);
void document_insert_buffer(struct document* document, file_offset_t offset, struct range_tree_node* buffer);

void document_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_delete(struct document* document, file_offset_t offset, file_offset_t length);
int document_delete_selection(struct document* document);

void document_keypress(struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y);

void document_directory(struct document* document);

#endif /* #ifndef __TIPPSE_DOCUMENT__ */
