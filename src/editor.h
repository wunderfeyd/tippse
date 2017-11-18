#ifndef TIPPSE_EDITOR_H
#define TIPPSE_EDITOR_H

#include <stdlib.h>
#include <string.h>

struct screen;
struct list;
struct trie;
struct document_file;
struct splitter;

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
#define TIPPSE_CMD_COMPILE 50
#define TIPPSE_CMD_MAX 51

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
  struct document_file* document_doc; // document: inital empty document
  struct document_file* compiler_doc; // document: compiler output

  struct splitter* splitters;         // Tree of splitters
  struct splitter* panel;             // Extra panel for toolbox user input
  struct splitter* focus;             // Current focused document
  struct splitter* document;          // Current selected document
  struct splitter* last_document;     // Last selected user document

  int pipefd[2];                      // Process stdin/stdout pipes
};

#include "misc.h"
#include "documentfile.h"
#include "screen.h"
#include "splitter.h"
#include "list.h"
#include "trie.h"

struct editor* editor_create(const char* base_path, struct screen* screen, int argc, const char** argv);
void editor_destroy(struct editor* base);
void editor_closed(struct editor* base);
void editor_draw(struct editor* base);
void editor_tick(struct editor* base);
void editor_keypress(struct editor* base, int key, int cp, int button, int button_old, int x, int y);
void editor_intercept(struct editor* base, int command, int key, int cp, int button, int button_old, int x, int y);

void editor_focus(struct editor* base, struct splitter* node, int disable);
void editor_split(struct editor* base, struct splitter* node);
struct splitter* editor_unsplit(struct editor* base, struct splitter* node);
void editor_open_selection(struct editor* base, struct splitter* node, struct splitter* destination);
void editor_open_document(struct editor* base, const char* name, struct splitter* node, struct splitter* destination);
void editor_save_document(struct editor* base, struct document_file* file);
void editor_save_documents(struct editor* base);
void editor_close_document(struct editor* base, struct document_file* file);
#endif /* #ifndef TIPPSE_EDITOR_H */