#ifndef TIPPSE_EDITOR_H
#define TIPPSE_EDITOR_H

#include <stdlib.h>
#include <string.h>
#include "types.h"

struct screen;
struct list;
struct trie;
struct document_file;
struct splitter;

#define CONSOLE_TYPE_NORMAL 0
#define CONSOLE_TYPE_WARNING 1
#define CONSOLE_TYPE_ERROR 2

#define TIPPSE_KEY_MOD_SHIFT 0x1000
#define TIPPSE_KEY_MOD_CTRL 0x2000
#define TIPPSE_KEY_MOD_ALT 0x4000
#define TIPPSE_KEY_MASK 0xfff

#define TIPPSE_KEY_CHARACTER 0
#define TIPPSE_KEY_UP 1
#define TIPPSE_KEY_DOWN 2
#define TIPPSE_KEY_RIGHT 3
#define TIPPSE_KEY_LEFT 4
#define TIPPSE_KEY_PAGEUP 5
#define TIPPSE_KEY_PAGEDOWN 6
#define TIPPSE_KEY_BACKSPACE 7
#define TIPPSE_KEY_DELETE 8
#define TIPPSE_KEY_INSERT 9
#define TIPPSE_KEY_FIRST 10
#define TIPPSE_KEY_LAST 11
#define TIPPSE_KEY_TAB 12
#define TIPPSE_KEY_MOUSE 13
#define TIPPSE_KEY_RETURN 14
#define TIPPSE_KEY_BRACKET_PASTE_START 15
#define TIPPSE_KEY_BRACKET_PASTE_STOP 16
#define TIPPSE_KEY_ESCAPE 17
#define TIPPSE_KEY_F1 18
#define TIPPSE_KEY_F2 19
#define TIPPSE_KEY_F3 20
#define TIPPSE_KEY_F4 21
#define TIPPSE_KEY_F5 22
#define TIPPSE_KEY_F6 23
#define TIPPSE_KEY_F7 24
#define TIPPSE_KEY_F8 25
#define TIPPSE_KEY_F9 26
#define TIPPSE_KEY_F10 27
#define TIPPSE_KEY_F11 28
#define TIPPSE_KEY_F12 29
#define TIPPSE_KEY_MAX 30

#define TIPPSE_CMD_CHARACTER 0
#define TIPPSE_CMD_QUIT 1
#define TIPPSE_CMD_UP 2
#define TIPPSE_CMD_DOWN 3
#define TIPPSE_CMD_RIGHT 4
#define TIPPSE_CMD_LEFT 5
#define TIPPSE_CMD_PAGEUP 6
#define TIPPSE_CMD_PAGEDOWN 7
#define TIPPSE_CMD_FIRST 8
#define TIPPSE_CMD_LAST 9
#define TIPPSE_CMD_HOME 10
#define TIPPSE_CMD_END 11
#define TIPPSE_CMD_BACKSPACE 12
#define TIPPSE_CMD_DELETE 13
#define TIPPSE_CMD_INSERT 14
#define TIPPSE_CMD_SEARCH 15
#define TIPPSE_CMD_SEARCH_NEXT 16
#define TIPPSE_CMD_UNDO 17
#define TIPPSE_CMD_REDO 18
#define TIPPSE_CMD_CUT 19
#define TIPPSE_CMD_COPY 20
#define TIPPSE_CMD_PASTE 21
#define TIPPSE_CMD_TAB 22
#define TIPPSE_CMD_UNTAB 23
#define TIPPSE_CMD_SAVE 24
#define TIPPSE_CMD_MOUSE 25
#define TIPPSE_CMD_SEARCH_PREV 26
#define TIPPSE_CMD_OPEN 27
#define TIPPSE_CMD_SPLIT 28
#define TIPPSE_CMD_SHOW_INVISIBLES 29
#define TIPPSE_CMD_BROWSER 30
#define TIPPSE_CMD_VIEW_SWITCH 31
#define TIPPSE_CMD_BOOKMARK 32
#define TIPPSE_CMD_WORDWRAP 33
#define TIPPSE_CMD_DOCUMENTSELECTION 34
#define TIPPSE_CMD_RETURN 35
#define TIPPSE_CMD_SELECT_ALL 36
#define TIPPSE_CMD_SELECT_UP 37
#define TIPPSE_CMD_SELECT_DOWN 38
#define TIPPSE_CMD_SELECT_RIGHT 39
#define TIPPSE_CMD_SELECT_LEFT 40
#define TIPPSE_CMD_SELECT_PAGEUP 41
#define TIPPSE_CMD_SELECT_PAGEDOWN 42
#define TIPPSE_CMD_SELECT_FIRST 43
#define TIPPSE_CMD_SELECT_LAST 44
#define TIPPSE_CMD_SELECT_HOME 45
#define TIPPSE_CMD_SELECT_END 46
#define TIPPSE_CMD_CLOSE 47
#define TIPPSE_CMD_SAVEALL 48
#define TIPPSE_CMD_UNSPLIT 49
#define TIPPSE_CMD_SHELL 50
#define TIPPSE_CMD_REPLACE 51
#define TIPPSE_CMD_REPLACE_NEXT 52
#define TIPPSE_CMD_REPLACE_PREV 53
#define TIPPSE_CMD_REPLACE_ALL 54
#define TIPPSE_CMD_GOTO 55
#define TIPPSE_CMD_RELOAD 56
#define TIPPSE_CMD_COMMANDS 57
#define TIPPSE_CMD_UPPER 58
#define TIPPSE_CMD_LOWER 59
#define TIPPSE_CMD_NFD_NFC 60
#define TIPPSE_CMD_NFC_NFD 61
#define TIPPSE_CMD_SEARCH_MODE_TEXT 62
#define TIPPSE_CMD_SEARCH_MODE_REGEX 63
#define TIPPSE_CMD_SEARCH_CASE_SENSITIVE 64
#define TIPPSE_CMD_SEARCH_CASE_IGNORE 65
#define TIPPSE_CMD_SEARCH_DIRECTORY 66
#define TIPPSE_CMD_CONSOLE 67
#define TIPPSE_CMD_CUTLINE 68
#define TIPPSE_CMD_BLOCK_UP 69
#define TIPPSE_CMD_BLOCK_DOWN 70
#define TIPPSE_CMD_MAX 71

#define TIPPSE_MOUSE_LBUTTON 1
#define TIPPSE_MOUSE_RBUTTON 2
#define TIPPSE_MOUSE_MBUTTON 4
#define TIPPSE_MOUSE_WHEEL_UP 8
#define TIPPSE_MOUSE_WHEEL_DOWN 16

struct editor {
  int close;                  // editor closing?

  const char* base_path;      // base file path of startup
  struct screen* screen;      // display manager
  struct list* documents;     // open documents

  struct document_file* tabs_doc;     // document: open documents
  struct document_file* browser_doc;  // document: list of file from current directory
  struct document_file* search_doc;   // document: current search text
  struct document_file* replace_doc;  // document: current replace text
  struct document_file* goto_doc;     // document: line/offset for position jump
  struct document_file* compiler_doc; // document: compiler output
  struct document_file* search_results_doc; // document: search result output
  struct document_file* filter_doc;   // document: panel filter
  struct document_file* commands_doc; // document: list of known commands
  struct document_file* console_doc;  // document: list last editor messages

  struct splitter* splitters;         // Tree of splitters
  struct splitter* panel;             // Extra panel for toolbox user input
  struct splitter* filter;            // Extra filter panel for toolbox user input filtering
  struct splitter* toolbox;           // Contains filter and panel
  struct splitter* focus;             // Current focused document
  struct splitter* document;          // Current selected document
  struct splitter* last_document;     // Last selected user document

  int search_regex;         // Search for regluar expression?
  int search_ignore_case;   // Ignore case during search?
  int64_t tick;             // Start tick
  int64_t tick_undo;        // Tick count for next undo chaining
  int64_t tick_incremental; // Tick count for next incremental document processing

  char* command_map[TIPPSE_CMD_MAX];
  int pipefd[2];                      // Process stdin/stdout pipes
  int console_index;                  // Index of console updates
  int console_status;                 // Last index displayed as status line
  int64_t console_timeout;            // Display end time of last index
  char* console_text;                 // Last console line text
  int console_color;                  // Last console line color
};

#include "misc.h"
#include "documentfile.h"
#include "screen.h"
#include "splitter.h"
#include "list.h"
#include "trie.h"
#include "search.h"
#include "config.h"
#include "encoding.h"

struct editor* editor_create(const char* base_path, struct screen* screen, int argc, const char** argv);
void editor_destroy(struct editor* base);
void editor_closed(struct editor* base);
void editor_draw(struct editor* base);
void editor_tick(struct editor* base);
void editor_keypress(struct editor* base, int key, codepoint_t cp, int button, int button_old, int x, int y);
void editor_intercept(struct editor* base, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y);

void editor_focus(struct editor* base, struct splitter* node, int disable);
void editor_split(struct editor* base, struct splitter* node);
struct splitter* editor_unsplit(struct editor* base, struct splitter* node);
int editor_open_selection(struct editor* base, struct splitter* node, struct splitter* destination);
void editor_open_document(struct editor* base, const char* name, struct splitter* node, struct splitter* destination);
void editor_reload_document(struct editor* base, struct document_file* file);
void editor_save_document(struct editor* base, struct document_file* file);
void editor_save_documents(struct editor* base);
void editor_close_document(struct editor* base, struct document_file* file);
void editor_panel_assign(struct editor* base, struct document_file* file);
void editor_view_browser(struct editor* base, const char* filename, struct stream* filter_stream, struct encoding* filter_encoding);
void editor_view_tabs(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding);
void editor_view_commands(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding);
int editor_update_panel_height(struct editor* base, struct splitter* panel, int max);
struct document_file* editor_empty_document(struct editor* base);
void editor_update_search_title(struct editor* base);

void editor_console_update(struct editor* base, const char* text, size_t length, int type);

void editor_search(struct editor* base);

void editor_command_map_create(struct editor* base);
void editor_command_map_destroy(struct editor* base);
void editor_command_map_read(struct editor* base, struct document_file* file);

void editor_filter_clear(struct editor* base);
#endif /* #ifndef TIPPSE_EDITOR_H */
