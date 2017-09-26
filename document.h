#ifndef __TIPPSE_DOCUMENT__
#define __TIPPSE_DOCUMENT__

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

struct document;
struct splitter;

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
#define TIPPSE_KEY_VIEW_SWITCH -33
#define TIPPSE_KEY_BOOKMARK -35
#define TIPPSE_KEY_WORDWRAP -36
#define TIPPSE_KEY_DOCUMENTSELECTION -37
#define TIPPSE_KEY_RETURN -38

#define TIPPSE_MOUSE_LBUTTON 1
#define TIPPSE_MOUSE_RBUTTON 2
#define TIPPSE_MOUSE_MBUTTON 4
#define TIPPSE_MOUSE_WHEEL_UP 8
#define TIPPSE_MOUSE_WHEEL_DOWN 16

struct document {
  void (*reset)(struct document* base, struct splitter* splitter);
  void (*draw)(struct document* base, struct screen* screen, struct splitter* splitter);
  void (*keypress)(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y);
  int (*incremental_update)(struct document* base, struct splitter* splitter);
};

#include "splitter.h"
#include "document_text.h"

int document_compare(struct range_tree_node* left, file_offset_t displacement_left, struct range_tree_node* right_root, file_offset_t length);
void document_search(struct splitter* splitter, struct range_tree_node* text, file_offset_t length, int forward);
void document_directory(struct document_file* file);

#endif /* #ifndef __TIPPSE_DOCUMENT__ */
