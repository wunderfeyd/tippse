#ifndef __TIPPSE_DOCUMENT__
#define __TIPPSE_DOCUMENT__

#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

struct document;

#include "utf8.h"
#include "misc.h"
#include "trie.h"
#include "filetype.h"
#include "rangetree.h"
#include "screen.h"
#include "clipboard.h"
#include "documentview.h"
#include "documentfile.h"

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

#define TIPPSE_MOUSE_LBUTTON 1
#define TIPPSE_MOUSE_RBUTTON 2
#define TIPPSE_MOUSE_MBUTTON 4
#define TIPPSE_MOUSE_WHEEL_UP 8
#define TIPPSE_MOUSE_WHEEL_DOWN 16

struct document {
  struct document_file* file;
  struct document_view view;

  int keep_status;
  int content_document;
};

#include "splitter.h"
#include "documentundo.h"

struct document_render_info {
  int x;
  int y;
  int y_view;
  struct range_tree_node* buffer;
  file_offset_t buffer_pos;
  file_offset_t offset;
  int rows;
  int columns;
  int indentations;
  int indentation;
  int lines;
  int stop;
  int visual_detail;
  int mark_indentation;
  int width;
};

void document_render_info_clear(struct document_render_info* render_info, int width);
void document_render_info_seek(struct document_render_info* render_info, struct range_tree_node* buffer, int x, int y, file_offset_t search);
int document_render_info_span(struct document_render_info* render_info, struct screen* screen, struct splitter* splitter, struct document_view* view, struct document_file* file, int x_find, int y_find, file_offset_t offset_find, int* x_out, int* y_out, file_offset_t* offset_out, int* x_min, int* x_max, int* line, int* column);

int document_compare(struct range_tree_node* left, file_offset_t buffer_pos_left, struct range_tree_node* right_root, file_offset_t length);
void document_search(struct splitter* splitter, struct document* document, struct range_tree_node* text, file_offset_t length, int forward);
file_offset_t document_cursor_position(struct splitter* splitter, file_offset_t offset_search, int* cur_x, int* cur_y, int seek, int wrap, int cancel);
void document_draw(struct screen* screen, struct splitter* splitter);

void document_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_insert(struct document* document, file_offset_t offset, const char* text, size_t length);
void document_insert_buffer(struct document* document, file_offset_t offset, struct range_tree_node* buffer);

void document_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_delete(struct document* document, file_offset_t offset, file_offset_t length);
int document_delete_selection(struct document* document);

void document_keypress(struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y);

void document_directory(struct document* document);

#endif /* #ifndef __TIPPSE_DOCUMENT__ */
