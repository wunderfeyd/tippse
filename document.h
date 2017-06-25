#ifndef __TIPPSE_DOCUMENT__
#define __TIPPSE_DOCUMENT__

#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "utf8.h"
#include "misc.h"
#include "rangetree.h"
#include "screen.h"
#include "documentview.h"
#include "documentfile.h"
#include "clipboard.h"

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

struct splitter;

int document_compare(struct range_tree_node* left, file_offset_t buffer_pos_left, struct range_tree_node* right_root, file_offset_t length);
void document_search(struct splitter* splitter, struct document_file* file, struct document_view* view, struct range_tree_node* text, file_offset_t length, int forward);
file_offset_t document_cursor_position(struct document_file* file, file_offset_t offset_search, int* cur_x, int* cur_y, int seek, int wrap, int cancel, int showall);
void document_draw(struct screen* screen, struct splitter* splitter);

void document_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_insert(struct document_file* file, file_offset_t offset, const char* text, size_t length);
void document_insert_buffer(struct document_file* file, file_offset_t offset, struct range_tree_node* buffer);

void document_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length);
void document_delete(struct document_file* file, file_offset_t offset, file_offset_t length);
int document_delete_selection(struct document_file* file, struct document_view* view);

void document_keypress(struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y);

void document_directory(struct document_file* file);

#include "splitter.h"
#include "documentundo.h"

#endif /* #ifndef __TIPPSE_DOCUMENT__ */
