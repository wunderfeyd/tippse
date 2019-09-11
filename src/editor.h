#ifndef TIPPSE_EDITOR_H
#define TIPPSE_EDITOR_H

#include <stdlib.h>
#include <string.h>
#include "types.h"

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
#define TIPPSE_CMD_BOOKMARK_NEXT 71
#define TIPPSE_CMD_BOOKMARK_PREV 72
#define TIPPSE_CMD_WORD_NEXT 73
#define TIPPSE_CMD_WORD_PREV 74
#define TIPPSE_CMD_SELECT_WORD_NEXT 75
#define TIPPSE_CMD_SELECT_WORD_PREV 76
#define TIPPSE_CMD_ESCAPE 77
#define TIPPSE_CMD_SELECT_INVERT 78
#define TIPPSE_CMD_SEARCH_ALL 79
#define TIPPSE_CMD_SAVEAS 80
#define TIPPSE_CMD_NEW 81
#define TIPPSE_CMD_QUIT_FORCE 82
#define TIPPSE_CMD_QUIT_SAVE 83
#define TIPPSE_CMD_SAVE_FORCE 84
#define TIPPSE_CMD_NULL 85
#define TIPPSE_CMD_SAVE_ASK 86
#define TIPPSE_CMD_CLOSE_FORCE 87
#define TIPPSE_CMD_SPLIT_NEXT 88
#define TIPPSE_CMD_SPLIT_PREV 89
#define TIPPSE_CMD_SHELL_KILL 90
#define TIPPSE_CMD_DELETE_WORD_NEXT 91
#define TIPPSE_CMD_DELETE_WORD_PREV 92
#define TIPPSE_CMD_SELECT_LINE 93
#define TIPPSE_CMD_SAVE_SKIP 94
#define TIPPSE_CMD_DOCUMENT_BACK 95
#define TIPPSE_CMD_HELP 96
#define TIPPSE_CMD_SPLIT_GRAB_NEXT 97
#define TIPPSE_CMD_SPLIT_GRAB_PREV 98
#define TIPPSE_CMD_SPLIT_GRAB_DECREASE 99
#define TIPPSE_CMD_SPLIT_GRAB_INCREASE 100
#define TIPPSE_CMD_SPLIT_GRAB_ROTATE 101
#define TIPPSE_CMD_DOCUMENT_STICKY 102
#define TIPPSE_CMD_DOCUMENT_FLOAT 103
#define TIPPSE_CMD_ERROR_NEXT 104
#define TIPPSE_CMD_ERROR_PREV 105
#define TIPPSE_CMD_BRACKET_NEXT 106
#define TIPPSE_CMD_BRACKET_PREV 107
#define TIPPSE_CMD_MAX 108

#define TIPPSE_MOUSE_LBUTTON 1
#define TIPPSE_MOUSE_RBUTTON 2
#define TIPPSE_MOUSE_MBUTTON 4
#define TIPPSE_MOUSE_WHEEL_UP 8
#define TIPPSE_MOUSE_WHEEL_DOWN 16

#define TIPPSE_BROWSERTYPE_OPEN 0
#define TIPPSE_BROWSERTYPE_SAVE 1

struct editor_task {
  int command;                        // command to execute
  struct config_command* arguments;   // arguments to the command
  int key;                            // key from event source
  codepoint_t cp;                     // codepoint from event source
  int button;                         // mouse button status
  int button_old;                     // mouse button status before command
  int x;                              // mouse x coordinate for command
  int y;                              // mouse y coordinate for command
  struct document_file* file;         // document selected for this task
};

struct editor_menu {
  char* title;
  struct editor_task task;
};

struct editor {
  int close;                          // editor closing?

  const char* base_path;              // base file path of startup
  struct screen* screen;              // display manager
  struct list* documents;             // open documents

  struct document_file* tabs_doc;     // document: open documents
  struct document_file* browser_doc;  // document: list of file from current directory
  struct document_file* search_doc;   // document: current search text
  struct document_file* replace_doc;  // document: current replace text
  struct document_file* goto_doc;     // document: line/offset for position jump
  struct document_file* help_doc;     // document: current help page
  struct document_file* compiler_doc; // document: compiler output
  struct document_file* search_results_doc; // document: search result output
  struct document_file* filter_doc;   // document: panel filter
  struct document_file* commands_doc; // document: list of known commands
  struct document_file* console_doc;  // document: list last editor messages
  struct document_file* menu_doc;     // document: menu with selections for current task

  struct splitter* splitters;         // Tree of splitters
  struct splitter* panel;             // Extra panel for toolbox user input
  struct splitter* filter;            // Extra filter panel for toolbox user input filtering
  struct splitter* toolbox;           // Contains filter and panel
  struct splitter* focus;             // Current focused document
  struct splitter* grab;              // Current grabbed splitter
  struct splitter* document;          // Current selected document
  struct splitter* replace;           // Splitter to replace with opening document

  int search_regex;                   // Search for regluar expression?
  int search_ignore_case;             // Ignore case during search?
  int64_t tick;                       // Start tick
  int64_t tick_message;               // Tick count for process messages
  int64_t tick_undo;                  // Tick count for next undo chaining
  int64_t tick_incremental;           // Tick count for next incremental document processing

  int indicator;                      // Process indicator

  char* command_map[TIPPSE_CMD_MAX];
  int pipefd[2];                      // Process stdin/stdout pipes
  int console_index;                  // Index of console updates
  int console_status;                 // Last index displayed as status line
  int64_t console_timeout;            // Display end time of last index
  char* console_text;                 // Last console line text
  int console_color;                  // Last console line color

  int browser_type;                   // Type of current file browser
  char* browser_preset;               // Text for filter preset
  struct document_file* browser_file; // Current file assigned to browser

  int document_draft_count;           // Counter for new documents

  struct list* tasks;                 // open tasks
  struct list_node* task_active;      // current active task
  struct list_node* task_stop;        // task to stop at
  struct document_file* task_focus;   // delete task list if file is nor focused

  struct list* menu;                  // menu selections
};

struct editor* editor_create(const char* base_path, struct screen* screen, int argc, const char** argv);
void editor_destroy(struct editor* base);
void editor_closed(struct editor* base);
void editor_draw(struct editor* base);
void editor_tick(struct editor* base);
void editor_keypress(struct editor* base, int key, codepoint_t cp, int button, int button_old, int x, int y);
void editor_intercept(struct editor* base, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y, struct document_file* file);

void editor_focus(struct editor* base, struct splitter* node, int disable);
void editor_focus_next(struct editor* base, struct splitter* node, int side);

void editor_grab(struct editor* base, struct splitter* node, int disable);
void editor_grab_next(struct editor* base, struct splitter* node, int side);

void editor_split(struct editor* base, struct splitter* node);
struct splitter* editor_unsplit(struct editor* base, struct splitter* node);

void editor_document_sticky(struct editor* base, struct splitter* node);
void editor_document_float(struct editor* base, struct splitter* node);
int editor_document_sticked(struct editor* base, struct splitter* node);
struct splitter* editor_document_splitter(struct editor* base, struct splitter* node, struct document_file* file);

int editor_open_selection(struct editor* base, struct splitter* node, struct splitter* destination);
int editor_open_document(struct editor* base, const char* name, struct splitter* node, struct splitter* destination, int type, struct splitter** output);
void editor_reload_document(struct editor* base, struct document_file* file);
int editor_ask_document_action(struct editor* base, struct document_file* file, int force, int ask);
void editor_save_document(struct editor* base, struct document_file* file, int force, int ask);
void editor_save_documents(struct editor* base, int command);
int editor_modified_documents(struct editor* base);
void editor_close_document(struct editor* base, struct document_file* file);
void editor_panel_assign(struct editor* base, struct document_file* file);
void editor_view_browser(struct editor* base, const char* filename, struct stream* filter_stream, struct encoding* filter_encoding, int type, const char* preset, char* predefined, struct document_file* file);
void editor_view_tabs(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding);
void editor_view_commands(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding);
void editor_view_menu(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding);
void editor_view_help(struct editor* base, const char* name);
int editor_update_panel_height(struct editor* base, struct splitter* panel, int max);
struct document_file* editor_empty_document(struct editor* base);
void editor_update_search_title(struct editor* base);

void editor_console_update(struct editor* base, const char* text, size_t length, int type);

void editor_search(struct editor* base);

void editor_command_map_create(struct editor* base);
void editor_command_map_destroy(struct editor* base);
void editor_command_map_read(struct editor* base, struct document_file* file);

void editor_filter_clear(struct editor* base);

struct editor_task* editor_task_append(struct editor* base, int front, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y, struct document_file* file);
void editor_task_destroy_inplace(struct editor_task* base);
void editor_task_document_removed(struct editor* base, struct document_file* file);
void editor_task_clear(struct editor* base);
void editor_task_stop(struct editor* base);
void editor_task_remove_stop(struct editor* base);

void editor_menu_title(struct editor* base, const char* title);
void editor_menu_clear(struct editor* base);
void editor_menu_append(struct editor* base, const char* title, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y, struct document_file* file);

void editor_process_message(struct editor* base, const char* message, file_offset_t position, file_offset_t length);

void editor_open_error(struct editor* base, int reverse);
#endif /* #ifndef TIPPSE_EDITOR_H */
