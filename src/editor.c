// Tippse - Editor - Glue all the components together for a usable editor

#include "editor.h"

#include "config.h"
#include "document.h"
#include "document_hex.h"
#include "document_text.h"
#include "documentfile.h"
#include "documentundo.h"
#include "documentview.h"
#include "library/encoding.h"
#include "library/encoding/utf8.h"
#include "filetype/markdown.h"
#include "library/list.h"
#include "library/misc.h"
#include "screen.h"
#include "library/search.h"
#include "splitter.h"
#include "library/trie.h"
#include "library/file.h"

// Documentation
#include "../tmp/doc/index.h"
#include "../tmp/doc/regex.h"
#include "../tmp/LICENSE.h"

struct {
  const char* name;
  const char* filename;
  const uint8_t* data;
  size_t size;
} editor_documentations[] = {
  {"Help - Index", "index.md", file_index, sizeof(file_index)-1},
  {"Help - Regular expressions", "regex.md", file_regex, sizeof(file_regex)-1},
  {"Help - License", "../LICENSE.md", file_LICENSE, sizeof(file_LICENSE)-1},
  {NULL, NULL, NULL, 0},
};

static const char* editor_key_names[TIPPSE_KEY_MAX] = {
  "",
  "up",
  "down",
  "right",
  "left",
  "pageup",
  "pagedown",
  "backspace",
  "delete",
  "insert",
  "first",
  "last",
  "tab",
  "mouse",
  "return",
  "pastestart",
  "pastestop",
  "escape",
  "f1",
  "f2",
  "f3",
  "f4",
  "f5",
  "f6",
  "f7",
  "f8",
  "f9",
  "f10",
  "f11",
  "f12",
  "space"
};

static struct config_cache editor_commands[TIPPSE_CMD_MAX+1] = {
  {"", TIPPSE_CMD_CHARACTER, ""},
  {"quit", TIPPSE_CMD_QUIT, "Exit application"},
  {"up", TIPPSE_CMD_UP, "Move cursor up"},
  {"down", TIPPSE_CMD_DOWN, "Move cursor down"},
  {"right", TIPPSE_CMD_RIGHT, "Move cursor right"},
  {"left", TIPPSE_CMD_LEFT, "Move cursor left"},
  {"pageup", TIPPSE_CMD_PAGEUP, "Move cursor one page up"},
  {"pagedown", TIPPSE_CMD_PAGEDOWN, "Move cursor one page down"},
  {"first", TIPPSE_CMD_FIRST, "Move cursor to first position of line (toggle on indentation)"},
  {"last", TIPPSE_CMD_LAST, "Move cursor to last position of line"},
  {"home", TIPPSE_CMD_HOME, "Move cursor to first position of file"},
  {"end", TIPPSE_CMD_END, "Move cursor to last position of file"},
  {"backspace", TIPPSE_CMD_BACKSPACE, "Remove character in front of the cursor"},
  {"delete", TIPPSE_CMD_DELETE, "Remove character at cursor position"},
  {"insert", TIPPSE_CMD_INSERT, "Toggle between insert and override mode"},
  {"search", TIPPSE_CMD_SEARCH, "Open search panel"},
  {"searchnext", TIPPSE_CMD_SEARCH_NEXT, "Find next occurrence of text to search"},
  {"undo", TIPPSE_CMD_UNDO, "Undo last operation"},
  {"redo", TIPPSE_CMD_REDO, "Repeat previous undone operation"},
  {"cut", TIPPSE_CMD_CUT, "Copy to clipboard and delete selection"},
  {"copy", TIPPSE_CMD_COPY, "Copy to clipboard and keep selection"},
  {"paste", TIPPSE_CMD_PASTE, "Paste from clipboard"},
  {"tab", TIPPSE_CMD_TAB, "Raise indentation"},
  {"untab", TIPPSE_CMD_UNTAB, "Lower indentation"},
  {"save", TIPPSE_CMD_SAVE, "Save active document"},
  {"mouse", TIPPSE_CMD_MOUSE, "Unused"},
  {"searchprevious", TIPPSE_CMD_SEARCH_PREV, "Find previous occurrence of text to search"},
  {"open", TIPPSE_CMD_OPEN, "Open selected file"},
  {"split", TIPPSE_CMD_SPLIT, "Split into two views"},
  {"invisibles", TIPPSE_CMD_SHOW_INVISIBLES, "Toggle invisible character display"},
  {"browser", TIPPSE_CMD_BROWSER, "Open file browser panel"},
  {"switch", TIPPSE_CMD_VIEW_SWITCH, "Switch view between text editor and hex editor"},
  {"bookmark", TIPPSE_CMD_BOOKMARK, "Toggle line bookmark"},
  {"wordwrap", TIPPSE_CMD_WORDWRAP, "Toggle word wrapping"},
  {"documents", TIPPSE_CMD_DOCUMENTSELECTION, "Open open document panel"},
  {"return", TIPPSE_CMD_RETURN, "Insert line break"},
  {"selectall", TIPPSE_CMD_SELECT_ALL, "Select whole document"},
  {"selectup", TIPPSE_CMD_SELECT_UP, "Extend selection upwards"},
  {"selectdown", TIPPSE_CMD_SELECT_DOWN, "Extend selection downwards"},
  {"selectright", TIPPSE_CMD_SELECT_RIGHT, "Extend selection to the right"},
  {"selectleft", TIPPSE_CMD_SELECT_LEFT, "Extend selection to the left"},
  {"selectpageup", TIPPSE_CMD_SELECT_PAGEUP, "Extend selection a page up"},
  {"selectpagedown", TIPPSE_CMD_SELECT_PAGEDOWN, "Extend selection a page down"},
  {"selectfirst", TIPPSE_CMD_SELECT_FIRST, "Extend selection to first position on line"},
  {"selectlast", TIPPSE_CMD_SELECT_LAST, "Extend selection to last position on line"},
  {"selecthome", TIPPSE_CMD_SELECT_HOME, "Extend selection to first position of file"},
  {"selectend", TIPPSE_CMD_SELECT_END, "Extend selection to last position of file"},
  {"close", TIPPSE_CMD_CLOSE, "Close active document"},
  {"saveall", TIPPSE_CMD_SAVEALL, "Save all modified documents"},
  {"unsplit", TIPPSE_CMD_UNSPLIT, "Remove active view"},
  {"shell", TIPPSE_CMD_SHELL, "Show shell output or start shell command if already shown"},
  {"replace", TIPPSE_CMD_REPLACE, "Open replace panel"},
  {"replacenext", TIPPSE_CMD_REPLACE_NEXT, "Replace selection and find next occurrence of text to search"},
  {"replaceprevious", TIPPSE_CMD_REPLACE_PREV, "Replace selection and find previous occurrence of text to search"},
  {"replaceall", TIPPSE_CMD_REPLACE_ALL, "Replace all occurrences"},
  {"goto", TIPPSE_CMD_GOTO, "Open line number input panel"},
  {"reload", TIPPSE_CMD_RELOAD, "Reload active document"},
  {"commands", TIPPSE_CMD_COMMANDS, "Show all commands"},
  {"upper", TIPPSE_CMD_UPPER, "Convert to uppercase (document/selection)"},
  {"lower", TIPPSE_CMD_LOWER, "Convert to lowercase (document/selection)"},
  {"nfdnfc", TIPPSE_CMD_NFD_NFC, "Convert from nfd to nfc (document/selection)"},
  {"nfcnfd", TIPPSE_CMD_NFC_NFD, "Convert from nfc to nfd (document/selection)"},
  {"searchmodetext", TIPPSE_CMD_SEARCH_MODE_TEXT, "Search with uninterpreted plain text"},
  {"searchmoderegex", TIPPSE_CMD_SEARCH_MODE_REGEX, "Search with regular expression"},
  {"searchcasesensitive", TIPPSE_CMD_SEARCH_CASE_SENSITIVE, "Search case senstive"},
  {"searchcaseignore", TIPPSE_CMD_SEARCH_CASE_IGNORE, "Search while ignoring case"},
  {"searchdirectory", TIPPSE_CMD_SEARCH_DIRECTORY, "Search in current directory"},
  {"console", TIPPSE_CMD_CONSOLE, "Open editor console"},
  {"cutline", TIPPSE_CMD_CUTLINE, "Copy line to clipboard and delete"},
  {"blockup", TIPPSE_CMD_BLOCK_UP, "Move block up"},
  {"blockdown", TIPPSE_CMD_BLOCK_DOWN, "Move block down"},
  {"bookmarknext", TIPPSE_CMD_BOOKMARK_NEXT, "Goto next bookmark"},
  {"bookmarkprev", TIPPSE_CMD_BOOKMARK_PREV, "Goto previous bookmark"},
  {"wordnext", TIPPSE_CMD_WORD_NEXT, "Goto next word"},
  {"wordprev", TIPPSE_CMD_WORD_PREV, "Goto previous word"},
  {"selectwordnext", TIPPSE_CMD_SELECT_WORD_NEXT, "Extend selection to the next word"},
  {"selectwordprev", TIPPSE_CMD_SELECT_WORD_PREV, "Extend selection to the previous word"},
  {"escape", TIPPSE_CMD_ESCAPE, "Quit dialog or cancel operation"},
  {"selectinvert", TIPPSE_CMD_SELECT_INVERT, "Invert selection"},
  {"searchall", TIPPSE_CMD_SEARCH_ALL, "Search and select all matches"},
  {"saveas", TIPPSE_CMD_SAVEAS, "Save current document with another name"},
  {"new", TIPPSE_CMD_NEW, "Create new untitled document"},
  {"quitforce", TIPPSE_CMD_QUIT_FORCE, "Quit without any further questions"},
  {"quitsave", TIPPSE_CMD_QUIT_SAVE, "Save and quit"},
  {"saveforce", TIPPSE_CMD_SAVE_FORCE, "Save file contents regardless its current working state"},
  {"null", TIPPSE_CMD_NULL, "Nothing"},
  {"saveask", TIPPSE_CMD_SAVE_ASK, "Request user interaction if file was modified and is going to be saved"},
  {"closeforce", TIPPSE_CMD_CLOSE_FORCE, "Close document without asking for save options"},
  {"splitnext", TIPPSE_CMD_SPLIT_NEXT, "Select next split view"},
  {"splitprevious", TIPPSE_CMD_SPLIT_PREV, "Select previous split view"},
  {"shellkill", TIPPSE_CMD_SHELL_KILL, "Stop running shell command"},
  {"deletewordnext", TIPPSE_CMD_DELETE_WORD_NEXT, "Remove word or whitespace after current location"},
  {"deletewordprev", TIPPSE_CMD_DELETE_WORD_PREV, "Remove word or whitespace before current location"},
  {"selectline", TIPPSE_CMD_SELECT_LINE, "Extend selection to whole line"},
  {"saveskip", TIPPSE_CMD_SAVE_SKIP, "Skip file when saving documents"},
  {"documentback", TIPPSE_CMD_DOCUMENT_BACK, "Switch back to previous active document"},
  {"help", TIPPSE_CMD_HELP, "Show help"},
  {"splitgrabnext", TIPPSE_CMD_SPLIT_GRAB_NEXT, "Grab next splitter"},
  {"splitgrabprev", TIPPSE_CMD_SPLIT_GRAB_PREV, "Grab previous splitter"},
  {"splitgrabdecrease", TIPPSE_CMD_SPLIT_GRAB_DECREASE, "Split size decrease"},
  {"splitgrabincrease", TIPPSE_CMD_SPLIT_GRAB_INCREASE, "Split size increase"},
  {"splitgrabrotate", TIPPSE_CMD_SPLIT_GRAB_ROTATE, "Split rotation"},
  {"documentsticky", TIPPSE_CMD_DOCUMENT_STICKY, "Reopen document in current split only"},
  {"documentfloat", TIPPSE_CMD_DOCUMENT_FLOAT, "Reopen document in any split"},
  {"errornext", TIPPSE_CMD_ERROR_NEXT, "Search next error in shell command output"},
  {"errorprev", TIPPSE_CMD_ERROR_PREV, "Search prev error in shell command output"},
  {"bracketnext", TIPPSE_CMD_BRACKET_NEXT, "Search next bracket of current depth"},
  {"bracketprev", TIPPSE_CMD_BRACKET_PREV, "Search prev bracket of current depth"},
  {"autocomplete", TIPPSE_CMD_AUTOCOMPLETE, "Autocomplete as hinted"},
  {"space", TIPPSE_CMD_SPACE, "Insert word separation"},
  {NULL, 0, ""}
};

// Create editor
struct editor* editor_create(const char* base_path, struct screen* screen, int argc, const char** argv, editor_update_signal update_signal) {
  struct editor* base = (struct editor*)malloc(sizeof(struct editor));
  base->close = 0;
  base->update_signal = update_signal;
  base->tasks = list_create(sizeof(struct editor_task));
  base->task_focus = NULL;
  base->task_stop = NULL;
  base->menu = list_create(sizeof(struct editor_menu));
  base->base_path = base_path;
  base->screen = screen;
  base->focus = NULL;
  base->grab = NULL;
  base->state = NULL;
  base->tick = tick_count();
  base->tick_undo = -1;
  base->tick_incremental = -1;
  base->tick_message = 0;
  base->search_regex = 0;
  base->search_ignore_case = 1;
  base->console_index = 0;
  base->console_status = 0;
  base->console_timeout = 0;
  base->console_text = NULL;
  base->browser_type = TIPPSE_BROWSERTYPE_OPEN;
  base->browser_preset = NULL;
  base->document_draft_count = 0;
  base->indicator = 0;

  editor_command_map_create(base);

  base->documents = list_create(sizeof(struct document_file*));

  base->tabs_doc = document_file_create(0, 1, base);
  document_file_name(base->tabs_doc, "Open");
  base->tabs_doc->defaults.wrapping = 0;
  base->tabs_doc->line_select = 1;
  base->tabs_doc->undo = 0;

  base->browser_doc = document_file_create(0, 1, base);
  document_file_name(base->browser_doc, base->base_path);
  base->browser_doc->defaults.wrapping = 0;
  base->browser_doc->line_select = 1;
  base->browser_doc->undo = 0;

  base->commands_doc = document_file_create(0, 1, base);
  document_file_name(base->commands_doc, "Commands");
  base->commands_doc->defaults.wrapping = 0;
  base->commands_doc->line_select = 1;
  base->commands_doc->undo = 0;

  base->menu_doc = document_file_create(0, 1, base);
  document_file_name(base->menu_doc, "");
  base->menu_doc->defaults.wrapping = 0;
  base->menu_doc->line_select = 1;
  base->menu_doc->undo = 0;

  base->document = splitter_create(0, 0, NULL, NULL,  "");
  base->replace = NULL;

  base->search_doc = document_file_create(0, 1, base);
  base->replace_doc = document_file_create(0, 1, base);
  editor_update_search_title(base);

  base->goto_doc = document_file_create(0, 1, base);
  document_file_name(base->goto_doc, "Goto");

  base->compiler_doc = document_file_create(0, 1, base);
  base->compiler_doc->undo = 0;

  base->search_results_doc = document_file_create(0, 1, base);
  base->search_results_doc->undo = 0;

  base->help_doc = document_file_create(0, 1, base);
  base->help_doc->undo = 0;
  (*base->help_doc->type->destroy)(base->help_doc->type);
  base->help_doc->type = file_type_markdown_create(base->help_doc->config, "markdown");
  document_file_name(base->help_doc, "Help");

  base->filter_doc = document_file_create(0, 1, base);
  document_file_name(base->filter_doc, "Filter");

  base->console_doc = document_file_create(0, 1, base);
  document_file_name(base->console_doc, "Console");

  base->panel = splitter_create(0, 0, NULL, NULL, "");
  splitter_assign_document_file(base->panel, base->search_doc);

  base->filter = splitter_create(0, 0, NULL, NULL, "");
  splitter_assign_document_file(base->filter, base->filter_doc);

  base->toolbox = splitter_create(TIPPSE_SPLITTER_VERT|TIPPSE_SPLITTER_FIXED0, 5, base->filter, base->panel, "");
  base->splitters = splitter_create(TIPPSE_SPLITTER_VERT|TIPPSE_SPLITTER_FIXED0, 5, base->toolbox, base->document, "");
  list_insert(base->documents, NULL, &base->tabs_doc);
  list_insert(base->documents, NULL, &base->search_doc);
  list_insert(base->documents, NULL, &base->replace_doc);
  list_insert(base->documents, NULL, &base->goto_doc);
  list_insert(base->documents, NULL, &base->browser_doc);
  list_insert(base->documents, NULL, &base->commands_doc);
  list_insert(base->documents, NULL, &base->compiler_doc);
  list_insert(base->documents, NULL, &base->search_results_doc);
  list_insert(base->documents, NULL, &base->filter_doc);
  list_insert(base->documents, NULL, &base->console_doc);
  list_insert(base->documents, NULL, &base->menu_doc);
  list_insert(base->documents, NULL, &base->help_doc);

#ifndef _TESTSUITE
  const char* state = NULL;
  struct list* files = list_create(sizeof(const char*));
  for (int n = 1; n<argc;) {
    if (strcmp(argv[n], "--state")==0 || strcmp(argv[n], "-s")==0) {
      if (n+1<argc) {
        state = argv[n+1];
      }
      n += 2;
    } else {
      list_insert(files, NULL, &argv[n]);
      n++;
    }
  }

  if (state) {
    base->state = strdup(state);
    editor_state_load(base, base->state);
  }

  while (files->first) {
    editor_open_document(base, *((const char**)list_object(files->first)), NULL, base->document, TIPPSE_BROWSERTYPE_OPEN, NULL, NULL);
    list_remove(files, files->first);
  }

  list_destroy(files);
#else
  if (argc>3) {
    base->test_script_path = strdup(argv[1]);
    base->test_script = file_create(base->test_script_path, TIPPSE_FILE_READ);
    if (!base->test_script) {
      fprintf(stderr, "can't open test script (%s)\r\n", base->test_script_path);
      exit(1);
    }

    base->test_verify_path = strdup(argv[2]);
    base->test_output_path = strdup(argv[3]);
  } else {
    fprintf(stderr, "not enough test parameter\r\n");
    exit(1);
  }
#endif

  if (!base->document->file) {
    struct document_file* empty = editor_empty_document(base);
    splitter_assign_document_file(base->document, empty);
    document_view_reset(base->document->view, empty, 1);
  }

  editor_focus(base, base->document, 0);

  editor_keypress(base, 0, 0, 0, 0, 0, 0);
  return base;
}

// Destroy editor
void editor_destroy(struct editor* base) {
  if (base->state) {
    editor_state_save(base, base->state);
    free(base->state);
  }

  splitter_destroy(base->splitters);

  while (base->documents->first) {
    document_file_destroy(*(struct document_file**)list_object(base->documents->first));
    list_remove(base->documents, base->documents->first);
  }

  list_destroy(base->documents);
  editor_command_map_destroy(base);

  editor_menu_clear(base);
  list_destroy(base->menu);

  editor_task_clear(base);
  list_destroy(base->tasks);

  free(base->browser_preset);
  free(base->console_text);

#ifdef _TESTSUITE
  struct file* test_verify = file_create(base->test_verify_path, TIPPSE_FILE_READ);
  if (!test_verify) {
    fprintf(stderr, "can't open test verify (%s)\r\n", base->test_script_path);
    exit(1);
  }

  struct file* test_output = file_create(base->test_output_path, TIPPSE_FILE_READ);
  if (!test_output) {
    fprintf(stderr, "can't open test output (%s)\r\n", base->test_script_path);
    exit(1);
  }

  while (1) {
    uint8_t buffer_verify[4096];
    uint8_t buffer_output[4096];
    size_t size_verify = file_read(test_verify, &buffer_verify[0], 4096);
    size_t size_output = file_read(test_output, &buffer_output[0], 4096);
    if (size_verify!=size_output) {
      fprintf(stderr, "test verification failed due to different size (%s)\r\n", base->test_script_path);
      exit(1);
    }

    if (size_verify==0) {
      break;
    }

    if (memcmp(&buffer_verify[0], &buffer_output[0], size_verify)!=0) {
      fprintf(stderr, "test verification failed due to binary difference (%s)\r\n", base->test_script_path);
      exit(1);
    }
  }

  free(base->test_script_path);
  free(base->test_verify_path);
  free(base->test_output_path);

  file_destroy(base->test_script);
  file_destroy(test_verify);
  file_destroy(test_output);
#endif

  free(base);
}

// Check height of panel content
int editor_update_panel_height(struct editor* base, struct splitter* panel, int max) {
  (*panel->document->incremental_update)(panel->document, panel->view, panel->file);

  struct visual_info* visuals = panel->file->buffer.root?document_view_visual_create(panel->view, panel->file->buffer.root):NULL;
  int height = (int)(visuals?visuals->ys:0)+1;
  if (height>max) {
    height = max;
  }

  return height;
}

// Erase statusbar
void editor_draw_statusbar(struct editor* base, int foreground, int background) {
  for (int x = 0; x<base->screen->width; x++) {
    codepoint_t cp = 0x20;
    screen_setchar(base->screen, x, 0, 0, 0, base->screen->width, base->screen->height, &cp, 1, foreground, background);
  }
}

// Draw notification
void editor_draw_notify(struct editor* base, codepoint_t cp, int* x, int background, int flag) {
  if (flag) {
    screen_setchar(base->screen, *x, 0, 0, 0, base->screen->width, base->screen->height, &cp, 1, base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_TEXT], background);
    (*x)++;
  }
}

// Refresh editor components
void editor_draw(struct editor* base) {
  screen_check(base->screen);

  if (base->focus==base->panel || base->focus==base->filter) {
    int filter_height = (base->focus==base->filter)?(editor_update_panel_height(base, base->filter, (base->screen->height/4)+1)):0;
    int panel_height = (base->screen->height/2)+1-filter_height-(filter_height>0?1:0);
    if (base->panel->file!=base->browser_doc && base->panel->file!=base->tabs_doc && base->panel->file!=base->commands_doc && base->panel->file!=base->menu_doc) {
      panel_height = editor_update_panel_height(base, base->panel, (base->screen->height/2)+1-filter_height);;
    }

    base->toolbox->split = filter_height;
    base->splitters->split = filter_height+panel_height+(filter_height>0?1:0);
  } else {
    base->splitters->split = 0;
  }

  splitter_draw_multiple(base->splitters, base->screen, 0);

  int foreground = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_STATUS];
  int background = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
  editor_draw_statusbar(base, foreground, background);

  int running = 0;
  struct list_node* docs = base->documents->first;
  while (docs && !running) {
    running |= ((*(struct document_file**)list_object(docs))->piped==TIPPSE_PIPE_ACTIVE)?1:0;
    docs = docs->next;
  }

  int x = 0;
  editor_draw_notify(base, 'R', &x, background, running);
  editor_draw_notify(base, 'M', &x, background, document_file_modified_cache(base->document->file));
  editor_draw_notify(base, 'N', &x, background, base->document->file->save_skip);
  editor_draw_notify(base, 'S', &x, background, base->document->file->splitter?1:0);
  editor_draw_notify(base, 'O', &x, background, (base->document->view && base->document->view->overwrite)?1:0);
  editor_draw_notify(base, '*', &x, background, base->document->file?document_undo_modified(base->document->file):0);

  screen_drawtext(base->screen, (x>0)?x+1:x, 0, 0, 0, base->screen->width, base->screen->height, base->focus->name, (size_t)base->screen->width, foreground, background);

  int64_t tick = tick_count();
  if (base->console_status!=base->console_index) {
    base->console_timeout = tick+1000000;
    base->console_status = base->console_index;
  }

  bool_t status_visible = (base->focus->file!=base->browser_doc && base->focus->file!=base->menu_doc && tick<base->status_timeout)?1:0;

  const char* status = base->focus->status;
  if (tick<base->console_timeout && base->console_text) {
    status_visible = 1;
    status = base->console_text;
    if (base->console_color==VISUAL_FLAG_COLOR_CONSOLENORMAL) {
      foreground = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
      background = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_STATUS];
    } else {
      foreground = base->focus->file->defaults.colors[base->console_color];
    }
  }

  if (status_visible) {
    struct stream stream;
    stream_from_plain(&stream, (uint8_t*)status, SIZE_T_MAX);
    size_t length = encoding_strlen_based(NULL, encoding_utf8_decode, &stream);
    screen_drawtext(base->screen, base->screen->width-(int)length, 0, 0, 0, base->screen->width, base->screen->height, status, length, foreground, background);
    stream_destroy(&stream);
  }

  screen_update(base->screen);
}

// Regular timer event (schedule processes)
void editor_tick(struct editor* base) {
  int64_t tick = tick_count();
  if (tick>base->tick_undo) {
    base->tick_undo = tick+tick_ms(500);
    struct list_node* doc = base->documents->first;
    while (doc) {
      struct document_file* file = *(struct document_file**)list_object(doc);
      document_undo_chain(file, file->undos);
      doc = doc->next;
    }
  }

  int redraw = (base->splitters->timeout && base->splitters->timeout<tick)?1:0;
  if (base->status_timeout && base->status_timeout<tick) {
    base->status_timeout = 0;
    redraw = 1;
  }

  if (base->console_timeout && base->console_timeout<tick) {
    base->console_timeout = 0;
    redraw = 1;
  }

  if (redraw || screen_resized(base->screen)) {
    editor_draw(base);
  }

  if (tick>base->tick_incremental) {
    base->tick_incremental = tick+tick_ms(100);
    base->tick_message = tick_count();
    splitter_draw_multiple(base->splitters, base->screen, 1);
  }
}

// An input event was signalled ... translate it to a command if possible
void editor_keypress(struct editor* base, int key, codepoint_t cp, int button, int button_old, int x, int y) {
  int64_t tick = tick_count();
  base->tick_message = tick;
  base->tick_undo = tick+500000;
  base->status_timeout = tick+1000000;

  char key_lookup[1024];
  const char* key_name = editor_key_names[key&TIPPSE_KEY_MASK];
  sprintf(&key_lookup[0], "/keys/%s%s%s%s", (key&TIPPSE_KEY_MOD_SHIFT)?"shift+":"", (key&TIPPSE_KEY_MOD_CTRL)?"ctrl+":"", (key&TIPPSE_KEY_MOD_ALT)?"alt+":"", (cp!=0)?"":key_name);

  struct trie_node* parent = config_find_ascii(base->focus->file->config, &key_lookup[0]);
  if (cp!=0 && parent) {
    parent = config_advance_codepoints(base->focus->file->config, parent, &cp, 1);
  }

  if (!parent || !parent->end) {
    if (cp>0x0) {
      editor_task_append(base, 3, TIPPSE_CMD_CHARACTER, NULL, key, cp, button, button_old, x, y, NULL);
    }
  } else {
    struct config_value* value = (struct config_value*)list_object(*(struct list_node**)trie_object(parent));
    struct list_node* it = value->commands.first;
    while (it) {
      struct config_command* arguments = (struct config_command*)list_object(it);
      int command = (int)config_command_cache(arguments, &editor_commands[0]);
      if (command!=TIPPSE_CMD_CHARACTER) {
        editor_task_append(base, 3, command, arguments, key, cp, button, button_old, x, y, NULL);
      }
      it = it->next;
    }
  }

  editor_task_dispatch(base);
}

// After event translation we can intercept the core commands (for now)
void editor_intercept(struct editor* base, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y, struct document_file* file) {
  if (command==TIPPSE_CMD_MOUSE) {
    struct splitter* select = splitter_by_coordinate(base->splitters, x, y);
    if (select && button!=0 && button_old==0) {
      editor_focus(base, select, 1);
    }
  }

  if (base->task_focus && ((base->focus!=base->panel && base->focus!=base->filter) || base->panel->file!=base->task_focus)) {
    base->task_focus = NULL;
    editor_task_clear(base);
  }

  if (command==TIPPSE_CMD_DOCUMENTSELECTION) {
    editor_view_tabs(base, NULL, NULL);
  } else if (command==TIPPSE_CMD_DOCUMENT_BACK) {
    if (base->documents->count>1) {
      struct list_node* docs = base->documents->first;
      docs = docs->next;
      struct document_file* docs_document_doc = *(struct document_file**)list_object(docs);
      editor_open_document(base, docs_document_doc->filename, NULL, base->document, TIPPSE_BROWSERTYPE_OPEN, NULL, NULL);
    }
  } else if (command==TIPPSE_CMD_BROWSER || command==TIPPSE_CMD_SAVEAS) {
    struct document_file* browser_file = file?file:base->document->file;
    int unknown_location = (browser_file==base->compiler_doc || browser_file==base->search_results_doc || browser_file==base->help_doc);
    const char* path = unknown_location?".":browser_file->filename;
    char* filename = extract_file_name(path);
    char* directory = strip_file_name(path);
    char* corrected = correct_path(directory);
    char* relative = relativate_path(base->base_path, corrected);
    editor_view_browser(base, relative, NULL, NULL, (command==TIPPSE_CMD_BROWSER)?TIPPSE_BROWSERTYPE_OPEN:TIPPSE_BROWSERTYPE_SAVE, (command==TIPPSE_CMD_BROWSER)?"":filename, NULL, browser_file);
    free(relative);
    free(corrected);
    free(directory);
    free(filename);
    editor_task_stop(base);
    base->task_focus = base->browser_doc;
  } else if (command==TIPPSE_CMD_COMMANDS) {
    editor_view_commands(base, NULL, NULL);
  } else if (command==TIPPSE_CMD_QUIT) {
    if (editor_modified_documents(base)) {
      editor_task_stop(base);
      editor_menu_clear(base);
      editor_menu_title(base, "Quit?");
      editor_menu_append(base, "Yes, save documents", TIPPSE_CMD_QUIT_SAVE, NULL, 0, 0, 0, 0, 0, 0, NULL);
      editor_menu_append(base, "No, go back", TIPPSE_CMD_ESCAPE, NULL, 0, 0, 0, 0, 0, 0, NULL);
      editor_menu_append(base, "Force quit without saving documents", TIPPSE_CMD_QUIT_FORCE, NULL, 0, 0, 0, 0, 0, 0, NULL);
      editor_focus(base, base->document, 1);
      editor_view_menu(base, NULL, NULL);
      base->task_focus = base->menu_doc;
    } else {
      editor_task_append(base, 1, TIPPSE_CMD_QUIT_FORCE, NULL, 0, 0, 0, 0, 0, 0, NULL);
    }
  } else if (command==TIPPSE_CMD_QUIT_FORCE) {
    base->close = 1;
  } else if (command==TIPPSE_CMD_QUIT_SAVE) {
    editor_task_clear(base);
    editor_save_documents(base, TIPPSE_CMD_SAVE);
    editor_task_append(base, 0, TIPPSE_CMD_QUIT_FORCE, NULL, 0, 0, 0, 0, 0, 0, NULL);
  } else if (command==TIPPSE_CMD_SEARCH) {
    editor_search(base);
  } else if (command==TIPPSE_CMD_SEARCH_NEXT || (command==TIPPSE_CMD_RETURN && !base->search_regex && base->focus->file==base->search_doc)) {
    editor_focus(base, base->document, 1);
    document_search(base->document->file, base->document->view, &base->search_doc->buffer, base->search_doc->encoding, NULL, NULL, 0, base->search_ignore_case, base->search_regex, 0, 0);
  } else if (command==TIPPSE_CMD_SEARCH_PREV) {
    editor_focus(base, base->document, 1);
    document_search(base->document->file, base->document->view, &base->search_doc->buffer, base->search_doc->encoding, NULL, NULL, 1, base->search_ignore_case, base->search_regex, 0, 0);
  } else if (command==TIPPSE_CMD_SEARCH_ALL) {
    editor_focus(base, base->document, 1);
    document_search(base->document->file, base->document->view, &base->search_doc->buffer, base->search_doc->encoding, NULL, NULL, 0, base->search_ignore_case, base->search_regex, 1, 0);
  } else if (command==TIPPSE_CMD_SEARCH_DIRECTORY) {
    struct splitter* assign = editor_document_splitter(base, base->document, base->search_results_doc);
    if (assign->file==base->search_results_doc) {
      if (assign->file->piped!=TIPPSE_PIPE_ACTIVE) {
        struct document_file_pipe_operation* op = document_file_pipe_operation_create();
        op->operation = TIPPSE_PIPEOP_SEARCH;
        op->search.binary = (int)config_convert_int64(config_find_ascii(assign->file->config, "/searchfilebinary"));
        op->search.ignore_case = base->search_ignore_case;
        op->search.regex = base->search_regex;
        op->search.pattern_text = (char*)config_convert_encoding(config_find_ascii(assign->file->config, "/searchfilepattern"), encoding_utf8_static(), NULL);
        op->search.path = strdup(base->base_path);
        op->search.encoding = base->search_doc->encoding->create();
        op->search.buffer = range_tree_copy(&base->search_doc->buffer, 0, range_tree_length(&base->search_doc->buffer), NULL);

        document_file_create_pipe(assign->file, op);
      } else {
        document_file_shutdown_pipe(assign->file);
      }
    } else {
      splitter_assign_document_file(assign, base->search_results_doc);
    }
  } else if (command==TIPPSE_CMD_REPLACE) {
    editor_panel_assign(base, base->replace_doc);
    document_select_all(base->panel->file, base->panel->view, 1, 0);
  } else if (command==TIPPSE_CMD_REPLACE_NEXT) {
    editor_focus(base, base->document, 1);
    document_search(base->document->file, base->document->view, &base->search_doc->buffer, base->search_doc->encoding, &base->replace_doc->buffer, base->replace_doc->encoding, 0, base->search_ignore_case, base->search_regex, 0, 1);
  } else if (command==TIPPSE_CMD_REPLACE_PREV) {
    editor_focus(base, base->document, 1);
    document_search(base->document->file, base->document->view, &base->search_doc->buffer, base->search_doc->encoding, &base->replace_doc->buffer, base->replace_doc->encoding, 1, base->search_ignore_case, base->search_regex, 0, 1);
  } else if (command==TIPPSE_CMD_REPLACE_ALL || (command==TIPPSE_CMD_RETURN && !base->search_regex && base->focus->file==base->replace_doc)) {
    editor_focus(base, base->document, 1);
    document_search(base->document->file, base->document->view, &base->search_doc->buffer, base->search_doc->encoding, &base->replace_doc->buffer, base->replace_doc->encoding, 0, base->search_ignore_case, base->search_regex, 1, 1);
  } else if (command==TIPPSE_CMD_AUTOCOMPLETE) {
    if (base->focus->document==base->focus->document_text) {
      document_text_autocomplete(base->focus->document, base->focus->view, base->focus->file);
    }
  } else if (command==TIPPSE_CMD_GOTO) {
    editor_panel_assign(base, base->goto_doc);
    document_select_all(base->panel->file, base->panel->view, 1, 0);
  } else if (command==TIPPSE_CMD_CONSOLE) {
    editor_panel_assign(base, base->console_doc);
  } else if (command==TIPPSE_CMD_VIEW_SWITCH) {
    if (base->focus->document==base->focus->document_hex) {
      base->focus->document = base->focus->document_text;
      document_text_reset(base->focus->document, base->focus->view, base->focus->file);
    } else {
      base->focus->document = base->focus->document_hex;
      document_hex_reset(base->focus->document, base->focus->view, base->focus->file);
    }
  } else if (command==TIPPSE_CMD_ESCAPE) {
    editor_focus(base, base->document, 1);
    editor_task_clear(base);
  } else if (command==TIPPSE_CMD_CLOSE) {
    editor_ask_document_action(base, base->focus->file, 0, 1);
    editor_task_append(base, 0, TIPPSE_CMD_CLOSE_FORCE, NULL, 0, 0, 0, 0, 0, 0, base->focus->file);
  } else if (command==TIPPSE_CMD_CLOSE_FORCE) {
    editor_close_document(base, base->focus->file);
  } else if (command==TIPPSE_CMD_NEW) {
    struct document_file* empty = editor_empty_document(base);
    splitter_assign_document_file(editor_document_splitter(base, base->document, empty), empty);
    document_view_reset(base->document->view, empty, 1);
  } else if (command==TIPPSE_CMD_RETURN && base->focus->file==base->goto_doc) {
    struct stream stream;
    stream_from_page(&stream, base->focus->file->buffer.root, 0);
    struct unicode_sequencer sequencer;
    unicode_sequencer_clear(&sequencer, base->focus->file->encoding, &stream);
    // TODO: this should be handled by the specialized document classes
    if (base->document->document==base->document->document_hex) {
      file_offset_t offset = (file_offset_t)decode_based_unsigned(&sequencer, 16, SIZE_T_MAX);
      file_offset_t length = range_tree_length(&base->document->file->buffer);
      if (offset>length) {
        offset = length;
      }

      base->document->view->offset = offset;
      base->document->view->show_scrollbar = 1;
      document_select_nothing(base->document->file, base->document->view, 0);
    } else {
      size_t advanced = 0;
      position_t line = (position_t)decode_based_unsigned_offset(&sequencer, 10, &advanced, SIZE_T_MAX);
      advanced++;
      position_t column = (position_t)decode_based_unsigned_offset(&sequencer, 10, &advanced, SIZE_T_MAX);
      if (line>0) {
        document_text_goto(base->document->document, base->document->view, base->document->file, line-1, (column>0)?column-1:0);
        document_select_nothing(base->document->file, base->document->view, 0);
      }
    }

    stream_destroy(&stream);
    editor_focus(base, base->document, 1);
  } else if (command==TIPPSE_CMD_OPEN) {
    editor_open_selection(base, base->focus, base->document);
  } else if (command==TIPPSE_CMD_SPLIT_NEXT) {
    editor_focus_next(base, base->document, 0);
  } else if (command==TIPPSE_CMD_SPLIT_PREV) {
    editor_focus_next(base, base->document, 1);
  } else if (command==TIPPSE_CMD_SPLIT_GRAB_NEXT) {
    editor_grab_next(base, base->grab, 0);
  } else if (command==TIPPSE_CMD_SPLIT_GRAB_PREV) {
    editor_grab_next(base, base->grab, 1);
  } else if (command==TIPPSE_CMD_SPLIT_GRAB_DECREASE) {
    if (base->grab && base->grab->split>1) {
      base->grab->split--;
    }
  } else if (command==TIPPSE_CMD_SPLIT_GRAB_INCREASE) {
    if (base->grab && base->grab->split<99) {
      base->grab->split++;
    }
  } else if (command==TIPPSE_CMD_SPLIT_GRAB_ROTATE) {
    if (base->grab) {
      base->grab->type ^= TIPPSE_SPLITTER_HORZ;
    }
  } else if (command==TIPPSE_CMD_SPLIT_GRAB_PREV) {
    editor_grab_next(base, base->grab, 1);
  } else if (command==TIPPSE_CMD_SPLIT) {
    editor_split(base, base->document);
  } else if (command==TIPPSE_CMD_UNSPLIT) {
    struct splitter* document = editor_unsplit(base, base->document);
    if (base->focus==base->document) {
      editor_focus(base, document, 0);
    }
    base->document = document;
  } else if (command==TIPPSE_CMD_DOCUMENT_STICKY) {
    editor_document_sticky(base, base->document);
  } else if (command==TIPPSE_CMD_DOCUMENT_FLOAT) {
    editor_document_float(base, base->document);
  } else if (command==TIPPSE_CMD_RELOAD) {
    editor_reload_document(base, base->document->file);
  } else if (command==TIPPSE_CMD_SAVE) {
    editor_save_document(base, file?file:base->document->file, 0, 0, 0);
  } else if (command==TIPPSE_CMD_SAVE_FORCE) {
    editor_save_document(base, file?file:base->document->file, 1, 0, 0);
  } else if (command==TIPPSE_CMD_SAVE_ASK) {
    editor_save_document(base, file?file:base->document->file, 0, 1, 0);
  } else if (command==TIPPSE_CMD_SAVE_SKIP) {
    document_file_save_skip(file?file:base->document->file);
  } else if (command==TIPPSE_CMD_NULL) {
    editor_task_remove_stop(base);
  } else if (command==TIPPSE_CMD_SAVEALL) {
    editor_save_documents(base, TIPPSE_CMD_SAVE);
  } else if (command==TIPPSE_CMD_ERROR_NEXT) {
    editor_open_error(base, 0);
  } else if (command==TIPPSE_CMD_ERROR_PREV) {
    editor_open_error(base, 1);
  } else if (command==TIPPSE_CMD_SHELL) {
    struct splitter* assign = editor_document_splitter(base, base->document, base->compiler_doc);

    if (assign->file==base->compiler_doc) {
      if (assign->file->piped!=TIPPSE_PIPE_ACTIVE) {
        if (arguments && arguments->length>1) {
          struct trie_node* parent = config_find_ascii(base->focus->file->config, "/shell/");
          if (parent) {
            parent = config_advance_codepoints(base->focus->file->config, parent, arguments->arguments[1].codepoints, arguments->arguments[1].length);
          }

          if (parent && parent->end) {
            struct document_file_pipe_operation* op = document_file_pipe_operation_create();
            op->operation = TIPPSE_PIPEOP_EXECUTE;
            op->execute.shell = (char*)config_convert_encoding(parent, encoding_utf8_static(), NULL);
            document_file_create_pipe(assign->file, op);
          }
        }
      } else {
        document_file_shutdown_pipe(assign->file);
      }
    } else {
      splitter_assign_document_file(assign, base->compiler_doc);
    }
  } else if (command==TIPPSE_CMD_SHELL_KILL) {
    document_file_shutdown_pipe(base->document->file);
  } else if (command==TIPPSE_CMD_HELP) {
    editor_view_help(base, "index.md");
  } else if (command==TIPPSE_CMD_SEARCH_MODE_TEXT) {
    base->search_regex = 0;
    editor_update_search_title(base);
  } else if (command==TIPPSE_CMD_SEARCH_MODE_REGEX) {
    base->search_regex = 1;
    editor_update_search_title(base);
  } else if (command==TIPPSE_CMD_SEARCH_CASE_SENSITIVE) {
    base->search_ignore_case = 0;
    editor_update_search_title(base);
  } else if (command==TIPPSE_CMD_SEARCH_CASE_IGNORE) {
    base->search_ignore_case = 1;
    editor_update_search_title(base);
  } else if (command==TIPPSE_CMD_SELECT_INVERT) {
    document_view_select_invert(base->document->view, 1);
  } else {
    if (base->focus->file==base->tabs_doc || base->focus->file==base->browser_doc || base->focus->file==base->commands_doc || base->focus->file==base->menu_doc || base->focus->file==base->filter_doc) {
      int send_panel = 0;
      int send_filter = 0;
      if (command==TIPPSE_CMD_UP || command==TIPPSE_CMD_DOWN || command==TIPPSE_CMD_PAGEDOWN || command==TIPPSE_CMD_PAGEUP || command==TIPPSE_CMD_HOME || command==TIPPSE_CMD_END || command==TIPPSE_CMD_RETURN) {
        send_panel = 1;
      } else {
        send_filter = 1;
      }

      if (command==TIPPSE_CMD_MOUSE) {
        struct splitter* select = splitter_by_coordinate(base->splitters, x, y);
        send_filter = 0;
        send_panel = 0;
        if (select==base->filter) {
          send_filter = 1;
        } else {
          send_panel = 1;
        }
      }

      file_offset_t before = range_tree_length(&base->filter->file->buffer);
      if (send_panel) {
        (*base->panel->document->keypress)(base->panel->document, base->panel->view, base->panel->file, command, arguments, key, cp, button, button_old, x-base->panel->x, y-base->panel->y);
      }

      if (send_filter) {
        (*base->filter->document->keypress)(base->filter->document, base->filter->view, base->filter->file, command, arguments, key, cp, button, button_old, x-base->filter->x, y-base->filter->y);
      }

      int selected = 0;
      if (command==TIPPSE_CMD_RETURN) {
        selected = editor_open_selection(base, base->panel, base->document);
      }

      if (!selected) {
        file_offset_t now = range_tree_length(&base->filter_doc->buffer);
        if (before!=now) {
          struct stream* filter_stream = NULL;
          struct stream stream;
          if (base->filter_doc->buffer.root) {
            stream_from_page(&stream, range_tree_first(&base->filter_doc->buffer), 0);
            filter_stream = &stream;
          }
          if (base->panel->file==base->browser_doc) {
            char* raw = (char*)range_tree_raw(&base->filter_doc->buffer, 0, now);
            editor_view_browser(base, NULL, filter_stream, base->filter_doc->encoding, base->browser_type, NULL, raw, base->browser_file);
            free(raw);
          } else if (base->panel->file==base->tabs_doc) {
            editor_view_tabs(base, filter_stream, base->filter_doc->encoding);
          } else if (base->panel->file==base->commands_doc) {
            editor_view_commands(base, filter_stream, base->filter_doc->encoding);
          } else if (base->panel->file==base->menu_doc) {
            editor_view_menu(base, filter_stream, base->filter_doc->encoding);
          }
          if (filter_stream) {
            stream_destroy(filter_stream);
          }
        }

        editor_focus(base, now?base->filter:base->panel, 1);
      }
    } else {
      editor_grab(base, NULL, 1);
      (*base->focus->document->keypress)(base->focus->document, base->focus->view, base->focus->file, command, arguments, key, cp, button, button_old, x-base->focus->x, y-base->focus->y);
    }
  }
}

// Set focus to other document
void editor_focus(struct editor* base, struct splitter* node, int disable) {
  if (base->focus==node) {
    return;
  }

  if (disable) {
    if (base->panel) {
      base->panel->save_position = 0;
    }

    if (base->focus) {
      base->focus->active = 0;
      base->focus->save_position = 0;
    }
  }

  base->focus = node;
  if (base->focus) {
    base->focus->active = 1;
    base->focus->save_position = 1;
    if (node!=base->panel && node!=base->filter) {
      base->document = node;
    }
  }

  if (base->panel) {
    base->panel->save_position = 1;
  }

  if (!editor_document_sticked(base, base->document)) {
    base->replace = base->document;
  }
}

// Set focus to next open document
void editor_focus_next(struct editor* base, struct splitter* node, int side) {
  while (1) {
    node = splitter_next(node, side);
    if (!node) {
      node = base->splitters;
    } else if (node->file && node!=base->panel && node!=base->filter) {
      editor_focus(base, node, 1);
      break;
    }
  }
}

// Set grab to other splitter
void editor_grab(struct editor* base, struct splitter* node, int disable) {
  if (base->grab==node) {
    return;
  }

  if (base->grab && disable) {
    base->grab->grab = 0;
  }

  base->grab = node;
  if (base->grab) {
    base->grab->grab = 1;
  }
}

// Set grab to next grabbed splitter
void editor_grab_next(struct editor* base, struct splitter* node, int side) {
  node = splitter_grab_next(base->splitters, node, side);
  if (node) {
    editor_grab(base, node, 1);
  }
}

// Split view
void editor_split(struct editor* base, struct splitter* node) {
  splitter_split(node);
}

// Combine splitted view
struct splitter* editor_unsplit(struct editor* base, struct splitter* node) {
  if (base->grab==node) {
    editor_grab(base, NULL, 1);
  }

  if (base->replace==node) {
    base->replace = NULL;
  }

  struct list_node* it = base->documents->first;
  while (it) {
    struct document_file* document = *(struct document_file**)list_object(it);
    if (document->splitter==node) {
      document->splitter = NULL;
    }
    it = it->next;
  }

  return splitter_unsplit(node, base->splitters);
}

// Make document sticky to splitter
void editor_document_sticky(struct editor* base, struct splitter* node) {
  node->file->splitter = node;
}

// Make document available in any splitter
void editor_document_float(struct editor* base, struct splitter* node) {
  node->file->splitter = NULL;
}

// Has splitter a sticky document?
int editor_document_sticked(struct editor* base, struct splitter* node) {
  struct list_node* it = base->documents->first;
  while (it) {
    struct document_file* document = *(struct document_file**)list_object(it);
    if (document->splitter==node) {
      return 1;
    }
    it = it->next;
  }

  return 0;
}

// Return best splitter for given document
struct splitter* editor_document_splitter(struct editor* base, struct splitter* node, struct document_file* file) {
  if (file->splitter) {
    return file->splitter;
  }

  if (editor_document_sticked(base, node) && base->replace) {
    return base->replace;
  }

  return node;
}

// Use selection from specified view to open a new document
int editor_open_selection(struct editor* base, struct splitter* node, struct splitter* destination) {
  int done = 0;

  if (document_view_select_active(node->view) && node->file!=base->compiler_doc) {
    file_offset_t selection_low;
    file_offset_t selection_high;
    document_view_select_next(node->view, 0, &selection_low, &selection_high);

    char* name = (char*)range_tree_raw(&node->file->buffer, selection_low, selection_high);
    if (*name) {
      editor_focus(base, destination, 1);
      done = 1;
      if (node->file==base->menu_doc) {
        struct list_node* it = base->menu->first;
        while (it) {
          struct editor_menu* entry = (struct editor_menu*)list_object(it);
          if (strcmp(entry->title, name)==0) {
            editor_focus(base, base->document, 1);
            editor_task_append(base, 2, entry->task.command, entry->task.arguments, entry->task.key, entry->task.cp, entry->task.button, entry->task.button_old, entry->task.x, entry->task.y, entry->task.file);
            break;
          }
          it = it->next;
        }
      } else if (node->file==base->commands_doc) {
        for (size_t n = 0; editor_commands[n].text; n++) {
          const char* compare1 = name;
          const char* compare2 = editor_commands[n].text;
          while (*compare1!=0 && *compare1!=' ' && *compare1==*compare2) {
            compare1++;
            compare2++;
          }

          if (*compare1==' ') {
            editor_intercept(base, (int)n, NULL, 0, 0, 0, 0, 0, 0, NULL);
            break;
          }
        }
      } else if (node->file==base->browser_doc) {
        done = editor_open_document(base, name, node, destination, base->browser_type, NULL, NULL);
      } else {
        done = editor_open_document(base, name, node, destination, TIPPSE_BROWSERTYPE_OPEN, NULL, NULL);
      }
    }

    free(name);
  } else {
    if (node->document==node->document_text) {
      file_offset_t offset = document_text_line_start_offset(node->document, node->view, node->file);
      file_offset_t displacement;
      struct range_tree_node* buffer = range_tree_node_find_offset(node->file->buffer.root, offset, &displacement);
      struct stream text_stream;
      stream_from_page(&text_stream, buffer, displacement);

      const char* filter = node->file==base->help_doc?"[^\\n\\r]*?\\[[^\\n\\r\\]]*?\\]\\(([^\\n\\r\\)]*?)\\)":"\\s*([^\\n\\r]*?)\\s*\\:(\\d+)\\:((\\d+)\\:)?";
      struct stream filter_stream;
      stream_from_plain(&filter_stream, (uint8_t*)filter, strlen(filter));

      struct search* search = search_create_regex(1, 0, &filter_stream, node->file->encoding, node->file->encoding);

      int found = search_find_check(search, &text_stream);
      char* name = found?(char*)range_tree_raw(&node->file->buffer, stream_offset(&search->group_hits[0].start), stream_offset(&search->group_hits[0].end)):NULL;

      position_t line = 0;
      position_t column = 0;
      if (node->file!=base->help_doc) {
        struct unicode_sequencer sequencer;
        unicode_sequencer_clear(&sequencer, base->focus->file->encoding, &search->group_hits[1].start);
        line = (position_t)decode_based_unsigned(&sequencer, 10, SIZE_T_MAX);
        unicode_sequencer_clear(&sequencer, base->focus->file->encoding, &search->group_hits[3].start);
        column = (position_t)decode_based_unsigned(&sequencer, 10, SIZE_T_MAX);
      }

      search_destroy(search);
      stream_destroy(&text_stream);
      stream_destroy(&filter_stream);

      if (name) {
        editor_focus(base, destination, 1);
        done = 1;

        if (node->file==base->help_doc) {
          editor_view_help(base, name);
        } else {
          struct splitter* output;
          editor_open_document(base, name, NULL, destination, TIPPSE_BROWSERTYPE_OPEN, &output, NULL);
          if (line>0) {
            document_text_goto(output->document, output->view, output->file, line-1, (column>0)?column-1:0);
            document_select_nothing(output->file, output->view, 0);
          }
        }

        free(name);
      }
    }
  }

  if (base->task_focus==node->file && done) {
    editor_task_remove_stop(base);
    base->task_focus = NULL;
  }

  return done;
}

// Open file into destination splitter
int editor_open_document(struct editor* base, const char* name, struct splitter* node, struct splitter* destination, int type, struct splitter** output, struct document_file** file) {
  int success = 1;
  char* path_only;
  if (node) {
    if (node->file==base->browser_doc) {
      path_only = strdup(node->file->filename);
    } else {
      path_only = strip_file_name(node->file->filename);
    }
  } else {
    path_only = strdup("");
  }

  char* combined = combine_path_file(path_only, name);
  char* corrected = correct_path(combined);
  char* relative = relativate_path(base->base_path, corrected);

  if (is_directory(relative)) {
    if (destination) {
      editor_view_browser(base, relative, NULL, NULL, type, NULL, NULL, base->browser_file);
      success = 0;
    }
  } else {
    struct document_file* new_document_doc = NULL;
    struct list_node* docs = base->documents->first;
    while (docs) {
      struct document_file* docs_document_doc = *(struct document_file**)list_object(docs);
      if (strcmp(docs_document_doc->filename, relative)==0 && (!node || docs_document_doc!=node->file)) {
        new_document_doc = docs_document_doc;
        break;
      }

      docs = docs->next;
    }

    if (type==TIPPSE_BROWSERTYPE_OPEN) {
      if (new_document_doc) {
        list_remove(base->documents, docs);
      } else {
        new_document_doc = document_file_create(1, 1, base);
        document_file_load(new_document_doc, relative, 0, 1);
      }

      list_insert(base->documents, NULL, &new_document_doc);
      if (destination) {
        destination = editor_document_splitter(base, destination, new_document_doc);
        splitter_assign_document_file(destination, new_document_doc);
        editor_focus(base, destination, 1);
      }
    } else {
      if (new_document_doc && new_document_doc!=base->browser_file) {
        editor_console_update(base, "File already opened!", SIZE_T_MAX, CONSOLE_TYPE_WARNING);
        success = 0;
      } else {
        document_file_name(base->browser_file, relative);
        success = editor_save_document(base, base->browser_file, 1, 0, 1);
      }
    }

    if (file) {
      *file = new_document_doc;
    }
  }

  free(relative);
  free(combined);
  free(path_only);
  free(corrected);

  if (output) {
    *output = destination;
  }

  return success;
}

// Reload single document
void editor_reload_document(struct editor* base, struct document_file* file) {
  if (!file->save) {
    return;
  }

  document_file_load(file, file->filename, 0, 0);
}

// Ask user for file action if needed
int editor_ask_document_action(struct editor* base, struct document_file* file, int force, int ask) {
  int modified_cache = document_file_modified_cache(file);
  int modified = document_undo_modified(file);
  int draft = document_file_drafted(file);

  if ((!modified && !modified_cache && !force) || (file->save_skip && !force)) {
    return 0;
  }

  file->save_skip = 0;

  if (modified_cache && !force) {
    editor_task_stop(base);
    editor_menu_clear(base);
    char* title = combine_string("File on disk modified, overwrite? - ", file->filename);
    editor_menu_title(base, title);
    free(title);
    editor_menu_append(base, "Yes, overwrite file", TIPPSE_CMD_SAVE_FORCE, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_menu_append(base, "No, go back", TIPPSE_CMD_ESCAPE, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_menu_append(base, "Skip file", TIPPSE_CMD_NULL, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_menu_append(base, "Never", TIPPSE_CMD_SAVE_SKIP, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_focus(base, base->document, 1);
    editor_view_menu(base, NULL, NULL);
    base->task_focus = base->menu_doc;
    return 0;
  }

  if (modified && !draft && !force && ask) {
    editor_task_stop(base);
    editor_menu_clear(base);
    char* title = combine_string("File modified, save? - ", file->filename);
    editor_menu_title(base, title);
    free(title);
    editor_menu_append(base, "Yes, save it", TIPPSE_CMD_SAVE_FORCE, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_menu_append(base, "No, go back", TIPPSE_CMD_ESCAPE, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_menu_append(base, "Skip file", TIPPSE_CMD_NULL, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_menu_append(base, "Never", TIPPSE_CMD_SAVE_SKIP, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_focus(base, base->document, 1);
    editor_view_menu(base, NULL, NULL);
    base->task_focus = base->menu_doc;
    return 0;
  }

  if (draft && !force) {
    editor_task_stop(base);
    editor_menu_clear(base);
    char* title = combine_string("File never saved, select path? - ", file->filename);
    editor_menu_title(base, title);
    free(title);
    editor_menu_append(base, "Yes, save it", TIPPSE_CMD_SAVEAS, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_menu_append(base, "No, go back", TIPPSE_CMD_ESCAPE, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_menu_append(base, "Skip file", TIPPSE_CMD_NULL, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_menu_append(base, "Never", TIPPSE_CMD_SAVE_SKIP, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_focus(base, base->document, 1);
    editor_view_menu(base, NULL, NULL);
    base->task_focus = base->menu_doc;
    return 0;
  }

  return 1;
}

// Save single modified document
int editor_save_document(struct editor* base, struct document_file* file, int force, int ask, int exist) {
  if (!editor_ask_document_action(base, file, force, ask)) {
    return 0;
  }

  if (exist && is_path(file->filename)) {
    editor_task_stop(base);
    editor_menu_clear(base);
    char* title = combine_string("File already exist... - ", file->filename);
    editor_menu_title(base, title);
    free(title);
    editor_menu_append(base, "Overwrite", TIPPSE_CMD_SAVE_FORCE, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_menu_append(base, "Save with different name", TIPPSE_CMD_SAVEAS, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_menu_append(base, "Go back", TIPPSE_CMD_ESCAPE, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_menu_append(base, "Skip file", TIPPSE_CMD_NULL, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_focus(base, base->document, 1);
    editor_view_menu(base, NULL, NULL);
    base->task_focus = base->menu_doc;
    return 0;
  }

  int saved = document_file_save(file, file->filename);
  if (saved) {
    document_file_detect_properties(file);
  } else {
    editor_task_stop(base);
    editor_menu_clear(base);
    char* title = combine_string("Unable to write file... - ", file->filename);
    editor_menu_title(base, title);
    free(title);
    editor_menu_append(base, "Try again", TIPPSE_CMD_SAVE, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_menu_append(base, "Save with different name", TIPPSE_CMD_SAVEAS, NULL, 0, 0, 0, 0, 0, 0, file);
    editor_menu_append(base, "Go back", TIPPSE_CMD_ESCAPE, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_menu_append(base, "Skip file", TIPPSE_CMD_NULL, NULL, 0, 0, 0, 0, 0, 0, NULL);
    editor_focus(base, base->document, 1);
    editor_view_menu(base, NULL, NULL);
    base->task_focus = base->menu_doc;
    return 0;
  }

  return 1;
}

// Save all modified documents
void editor_save_documents(struct editor* base, int command) {
  struct list_node* it = base->documents->first;
  while (it) {
    struct document_file* file = *(struct document_file**)list_object(it);
    if (file->save) {
      editor_task_append(base, 0, command, NULL, 0, 0, 0, 0, 0, 0, file);
    }
    it = it->next;
  }
}

// Check for modified documents
int editor_modified_documents(struct editor* base) {
  struct list_node* it = base->documents->first;
  while (it) {
    struct document_file* file = *(struct document_file**)list_object(it);
    if (file->save && !file->save_skip && (document_file_modified_cache(file) || document_undo_modified(file))) {
      return 1;
    }
    it = it->next;
  }
  return 0;
}

// Close single document and reassign splitters
void editor_close_document(struct editor* base, struct document_file* file) {
  if (!file->save) {
    return;
  }

  struct list_node* docs = base->documents->first;
  struct list_node* remove = NULL;
  struct document_file* assign = NULL;
  while (docs && (!remove || !assign)) {
    struct document_file* doc = *(struct document_file**)list_object(docs);
    if (doc==file) {
      remove = docs;
    } else if (doc->save) {
      assign = doc;
    }

    docs = docs->next;
  }

  if (remove) {
    if (!assign) {
      assign = editor_empty_document(base);
    }

    struct document_file* replace = *(struct document_file**)list_object(remove);
    editor_task_document_removed(base, replace);
    splitter_exchange_document_file(base->splitters, replace, assign);
    document_file_destroy(replace);
    list_remove(base->documents, remove);
  }
}

// Create empty document
struct document_file* editor_empty_document(struct editor* base) {
  struct document_file* file = document_file_create(1, 1, base);
  base->document_draft_count++;
  char title[1024];
  sprintf(&title[0], "Untitled%d", base->document_draft_count);
  document_file_name(file, &title[0]);
  document_file_draft(file);
  list_insert(base->documents, NULL, &file);
  return file;
}

// Assign new document to panel
void editor_panel_assign(struct editor* base, struct document_file* file) {
  if (base->focus==base->document || base->panel->file!=file) {
    splitter_assign_document_file(base->panel, file);
    editor_focus(base, base->panel, 1);
  } else {
    editor_focus(base, base->document, 1);
  }
}

// Update panel
void editor_view_update(struct editor* base, struct document_file* file) {
  document_file_change_views(file, 1);
  document_view_reset(base->panel->view, file, 1);
  base->panel->view->line_select = 1;

  (*base->panel->document->keypress)(base->panel->document, base->panel->view, base->panel->file, TIPPSE_CMD_RETURN, NULL, 0, 0, 0, 0, 0, 0);
}

// Update and change to browser view
void editor_view_browser(struct editor* base, const char* filename, struct stream* filter_stream, struct encoding* filter_encoding, int type, const char* preset, char* predefined, struct document_file* file) {
  if (preset) {
    free(base->browser_preset);
    base->browser_preset = strdup(preset);
  }

  editor_panel_assign(base, base->browser_doc);

  if (filename) {
    document_file_name(base->panel->file, filename);
  }

  if (!filter_stream) {
    editor_filter_clear(base, base->browser_doc->filename);
    if (base->browser_preset && *base->browser_preset) {
      document_file_insert(base->filter_doc, 0, (uint8_t*)base->browser_preset, strlen(base->browser_preset), 0);
      document_view_select_all(base->filter->view, base->filter->file, 0);
      editor_focus(base, base->filter, 1);
    }
  }

  base->browser_type = type;
  base->browser_file = file;
  document_directory(base->browser_doc, filter_stream, filter_encoding, (predefined && *predefined)?predefined:base->browser_preset);
  editor_view_update(base, base->browser_doc);
}

// Update and change to document view
void editor_view_tabs(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding) {
  if (!filter_stream) {
    editor_filter_clear(base, base->tabs_doc->filename);
  }

  editor_panel_assign(base, base->tabs_doc);

  struct search* search = filter_stream?search_create_plain(1, 0, filter_stream, filter_encoding, base->tabs_doc->encoding):NULL;

  document_file_empty(base->tabs_doc);
  struct list_node* doc = base->documents->first;
  while (doc) {
    struct document_file* file = *(struct document_file**)list_object(doc);
    size_t length = strlen(file->filename);
    struct stream text_stream;
    stream_from_plain(&text_stream, (uint8_t*)file->filename, length);
    if (file->save && (!search || search_find(search, &text_stream, NULL, NULL))) {
      document_insert_search(base->tabs_doc, search, file->filename, length, document_undo_modified(file)?TIPPSE_INSERTER_HIGHLIGHT|(VISUAL_FLAG_COLOR_MODIFIED<<TIPPSE_INSERTER_HIGHLIGHT_COLOR_SHIFT):0);
    }
    stream_destroy(&text_stream);

    doc = doc->next;
  }

  if (search) {
    search_destroy(search);
  }

  editor_view_update(base, base->tabs_doc);
}

// Update and change to commands view
void editor_view_commands(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding) {
  if (!filter_stream) {
    editor_command_map_read(base, base->document->file);
    editor_filter_clear(base, base->commands_doc->filename);
  }

  editor_panel_assign(base, base->commands_doc);

  struct search* search = filter_stream?search_create_plain(1, 0, filter_stream, filter_encoding, base->commands_doc->encoding):NULL;

  document_file_empty(base->commands_doc);
  for (size_t n = 1; editor_commands[n].text; n++) {
    char output[4096];
    // TODO: Encoding may destroy equal width ... build string in a different way
    sprintf(&output[0], "%-16s | %-16s | %s", editor_commands[n].text, base->command_map[n]?base->command_map[n]:"<none>", editor_commands[n].description);

    size_t length = strlen(&output[0]);
    struct stream text_stream;
    stream_from_plain(&text_stream, (uint8_t*)&output[0], length);
    if (!search || search_find(search, &text_stream, NULL, NULL)) {
      document_insert_search(base->commands_doc, search, &output[0], length, 0);
    }
    stream_destroy(&text_stream);
  }

  if (search) {
    search_destroy(search);
  }

  editor_view_update(base, base->commands_doc);
}

// Update and change to commands view
void editor_view_menu(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding) {
  if (!filter_stream) {
    editor_command_map_read(base, base->document->file);
    editor_filter_clear(base, base->menu_doc->filename);
  }

  editor_panel_assign(base, base->menu_doc);

  struct search* search = filter_stream?search_create_plain(1, 0, filter_stream, filter_encoding, base->menu_doc->encoding):NULL;

  document_file_empty(base->menu_doc);
  struct list_node* it = base->menu->first;
  while (it) {
    struct editor_menu* entry = (struct editor_menu*)list_object(it);

    size_t length = strlen(entry->title);
    struct stream text_stream;
    stream_from_plain(&text_stream, (uint8_t*)entry->title, length);
    if (!search || search_find(search, &text_stream, NULL, NULL)) {
      document_insert_search(base->menu_doc, search, entry->title, length, 0);
    }
    stream_destroy(&text_stream);

    it = it->next;
  }

  if (search) {
    search_destroy(search);
  }

  editor_view_update(base, base->menu_doc);
}

// Setup command map array
void editor_command_map_create(struct editor* base) {
  for (size_t n = 0; n<TIPPSE_CMD_MAX; n++) {
    base->command_map[n] = NULL;
  }
}

// Free strings of command map
void editor_command_map_destroy(struct editor* base) {
  for (size_t n = 0; n<TIPPSE_CMD_MAX; n++) {
    free(base->command_map[n]);
  }
}

// Build command map
void editor_command_map_read(struct editor* base, struct document_file* file) {
  editor_command_map_destroy(base);
  editor_command_map_create(base);
  struct trie_node* key_base = config_find_ascii(file->config, "/keys/");
  struct trie_node* parents[1024];
  codepoint_t chars[1024];
  size_t pos = 0;
  parents[pos] = key_base;
  chars[pos] = 0;
  while (1) {
    codepoint_t cp = 0;
    struct trie_node* parent = trie_find_codepoint_min(file->config->keywords, parents[pos], chars[pos], &cp);
    if (!parent) {
      if (pos==0) {
        break;
      }

      pos--;
    } else {
      chars[pos] = cp;
      pos++;

      if (parent->end) {
        char encoded[4096];
        char* build = &encoded[0];
        for (size_t copy = 0; copy<pos; copy++) {
          build += (*base->commands_doc->encoding->encode)(NULL, chars[copy], (uint8_t*)build, SIZE_T_MAX);
        }
        *build = 0;
        int command = (int)config_convert_int64_cache(parent, &editor_commands[0]);
        if (command>=0 && command<TIPPSE_CMD_MAX) {
          if (!base->command_map[command]) {
            base->command_map[command] = strdup(&encoded[0]);
          }
        }
      }

      parents[pos] = parent;
      chars[pos] = 0;
    }
  }
}

// Update the title of the search document accordingly to the current search flags
void editor_update_search_title(struct editor* base) {
  char title[1024];
  sprintf(&title[0], "Search [%s %s]", base->search_regex?"RegEx":"Text", base->search_ignore_case?"Ignore":"Sensitive");
  document_file_name(base->search_doc, &title[0]);

  sprintf(&title[0], "Replace [%s]", base->search_regex?"RegEx":"Text");
  document_file_name(base->replace_doc, &title[0]);
}

// Append line to console
void editor_console_update(struct editor* base, const char* text, size_t length, int type) {
  if (!base) {
    return;
  }

  if (length==SIZE_T_MAX) {
    length = strlen(text);
  }

  free(base->console_text);
  base->console_text = strndup(text, length);
  base->console_color = VISUAL_FLAG_COLOR_CONSOLENORMAL+type;

  document_file_insert(base->console_doc, range_tree_length(&base->console_doc->buffer), (uint8_t*)text, length, TIPPSE_INSERTER_HIGHLIGHT|(base->console_color<<TIPPSE_INSERTER_HIGHLIGHT_COLOR_SHIFT));
  document_file_insert(base->console_doc, range_tree_length(&base->console_doc->buffer), (uint8_t*)"\n", 1, 0);
  base->console_index++;
}

// Update search dialog
void editor_search(struct editor* base) {
  editor_panel_assign(base, base->search_doc);
  if (base->document->view->selection_reset && document_view_select_active(base->document->view) && base->document->view->update_search) {
    file_offset_t selection_low;
    file_offset_t selection_high;
    document_view_select_next(base->document->view, 0, &selection_low, &selection_high);

    // TODO: update encoding in search panel? or transform to encoding?
    document_file_delete(base->search_doc, 0, range_tree_length(&base->search_doc->buffer));
    struct range_tree* buffer = encoding_transform_page(base->document->file->buffer.root, selection_low, selection_high-selection_low, base->document->file->encoding, base->panel->file->encoding);
    document_file_insert_buffer(base->search_doc, 0, buffer->root);
    range_tree_destroy(buffer);
  }
  document_select_all(base->panel->file, base->panel->view, 1, 0);
  base->document->view->selection_reset = 0;
}

// Empty filter text
void editor_filter_clear(struct editor* base, const char* text) {
  document_file_empty(base->filter_doc);
  char* title = combine_string("Filter - ", text);
  document_file_name(base->filter_doc, title);
  free(title);
}

// Destroy task contents
void editor_task_destroy_inplace(struct editor_task* base) {
  if (base->arguments) {
    config_command_destroy(base->arguments);
  }
}

// Append task
struct editor_task* editor_task_append(struct editor* base, int front, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y, struct document_file* file) {
  struct list_node* prev = base->tasks->last;
  if (front==1) {
    if (base->task_stop) {
      prev = base->task_stop->prev;
    }
  } else if (front==2) {
    prev = base->tasks->first;
  } else if (front==3) {
    prev = NULL;
  }

  struct editor_task* task = (struct editor_task*)list_object(list_insert_empty(base->tasks, prev));
  task->command = command;
  task->arguments = arguments?config_command_clone(arguments):NULL;
  task->key = key;
  task->cp = cp;
  task->button = button;
  task->button_old = button_old;
  task->x = x;
  task->y = y;
  task->file = file;

  return task;
}

// Document was removed from index... also remove all tasks assigned to it
void editor_task_document_removed(struct editor* base, struct document_file* file) {
  struct list_node* it = base->tasks->first;
  while (it) {
    struct list_node* next = it->next;
    struct editor_task* task = (struct editor_task*)list_object(it);
    if (task->file==file) {
      if (it==base->task_active) {
        base->task_active = NULL;
      }
      if (it==base->task_stop) {
        base->task_stop = NULL;
      }
      editor_task_destroy_inplace(task);
      list_remove(base->tasks, it);
    }
    it = next;
  }
}

// Remove all tasks
void editor_task_clear(struct editor* base) {
  while (base->tasks->first) {
    struct editor_task* task = (struct editor_task*)list_object(base->tasks->first);
    editor_task_destroy_inplace(task);
    list_remove(base->tasks, base->tasks->first);
  }

  base->task_active = NULL;
  base->task_stop = NULL;
}

// Place "stop" at current task
void editor_task_stop(struct editor* base) {
  if (base->task_stop!=base->tasks->first) {
    editor_task_remove_stop(base);
  }

  base->task_stop = base->tasks->first;
}

// Search for first stop task and remove it
void editor_task_remove_stop(struct editor* base) {
  if (base->task_stop) {
    struct editor_task* task = (struct editor_task*)list_object(base->task_stop);
    if (base->task_stop==base->task_active) {
      base->task_active = NULL;
    }
    editor_task_destroy_inplace(task);
    list_remove(base->tasks, base->task_stop);
    base->task_stop = NULL;
  }
}

// Dispatch tasks
void editor_task_dispatch(struct editor* base) {
  while (base->tasks->first) {
    if (base->tasks->first==base->task_stop) {
      break;
    }

    struct editor_task* task = (struct editor_task*)list_object(base->tasks->first);

    base->task_active = base->tasks->first;
    editor_intercept(base, task->command, task->arguments, task->key, task->cp, task->button, task->button_old, task->x, task->y, task->file);
    if (base->task_active && base->task_active!=base->task_stop) {
      editor_task_destroy_inplace(task);
      list_remove(base->tasks, base->task_active);
    }
  }
}

// Change title for active menu
void editor_menu_title(struct editor* base, const char* title) {
  document_file_name(base->menu_doc, title);
}

// Remove menu contents
void editor_menu_clear(struct editor* base) {
  while (base->menu->first) {
    struct editor_menu* entry = (struct editor_menu*)list_object(base->menu->first);
    free(entry->title);
    editor_task_destroy_inplace(&entry->task);
    list_remove(base->menu, base->menu->first);
  }
}

// Add menu entry
void editor_menu_append(struct editor* base, const char* title, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y, struct document_file* file) {
  struct editor_menu* entry = (struct editor_menu*)list_object(list_insert_empty(base->menu, base->menu->last));
  entry->title = strdup(title);
  entry->task.command = command;
  entry->task.arguments = arguments?config_command_clone(arguments):NULL;
  entry->task.key = key;
  entry->task.cp = cp;
  entry->task.button = button;
  entry->task.button_old = button_old;
  entry->task.x = x;
  entry->task.y = y;
  entry->task.file = file;
}

void editor_process_message(struct editor* base, const char* message, file_offset_t position, file_offset_t length) {
  int64_t tick = tick_count();
  if (tick>base->tick_message+1000000 && base->focus && base->screen) {
    base->tick_message = tick;

    int foreground = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
    int background = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_SELECTION];
    editor_draw_statusbar(base, foreground, background);

    char indicator = 0;
    base->indicator++;
    base->indicator&=3;
    if (base->indicator==0) {
      indicator = '|';
    } if (base->indicator==1) {
      indicator = '/';
    } if (base->indicator==2) {
      indicator = '-';
    } if (base->indicator==3) {
      indicator = '\\';
    }

    if (length>(1<<24)) {
      length >>= 12;
      position >>= 12;
    }

    if (length==0) {
      length = 1;
    }

    int permille = (position*(file_offset_t)1000)/length;

    char text[1024];
    size_t size = (size_t)snprintf(&text[0], 1023, "%s %d.%01d%% %c", message, permille/10, permille%10, indicator);

    screen_drawtext(base->screen, 0, 0, 0, 0, base->screen->width, base->screen->height, &text[0], size, foreground, background);
    screen_update(base->screen);
  }
}

// Load help by name from memory
void editor_view_help(struct editor* base, const char* name) {
  size_t index = 0;
  for (size_t n = 0; editor_documentations[n].name; n++) {
    if (strcmp(name, editor_documentations[n].filename)==0) {
      index = n;
      break;
    }
  }

  document_file_load_memory(base->help_doc, (const uint8_t*)editor_documentations[index].data, editor_documentations[index].size, editor_documentations[index].name);
  splitter_assign_document_file(editor_document_splitter(base, base->document, base->help_doc), base->help_doc);
}

// Try to grab next error message
void editor_open_error(struct editor* base, int reverse) {
  if (base->compiler_doc->views->count==0) {
    return;
  }

  struct document_view* compiler_view = *(struct document_view**)list_object(base->compiler_doc->views->first);

  size_t pattern_text_length;
  char* pattern_text = (char*)config_convert_encoding(config_find_ascii(base->compiler_doc->config, "/errorpattern"), encoding_utf8_static(), &pattern_text_length);
  struct range_tree* root = range_tree_create(NULL, 0);
  range_tree_insert_split(root, 0, (const uint8_t*)pattern_text, pattern_text_length, 0);

  int found = document_search(base->compiler_doc, compiler_view, root, base->compiler_doc->encoding, NULL, NULL, reverse, 1, 1, 0, 0);

  range_tree_destroy(root);
  free(pattern_text);

  // TODO: the following part is opening the shell and emitting a open selection command... it would be nice to open the target without this, but current splitter focused structure doesn't allow to work on backgrounded documents at the moment... change me soon
  if (found) {
    struct splitter* assign = editor_document_splitter(base, base->document, base->compiler_doc);
    if (assign) {
      splitter_assign_document_file(assign, base->compiler_doc);
      editor_focus(base, assign, 1);
      editor_task_append(base, 1, TIPPSE_CMD_OPEN, NULL, 0, 0, 0, 0, 0, 0, NULL);
    }
  }
}

// Append keyword and string value to state configuration
void editor_state_save_keyword_string(struct editor* base, struct config* config, const char* path, const char* keyword, const char* value) {
  size_t keyword_encoded_length;
  char* combined = combine_string(path, keyword);
  codepoint_t* keyword_encoded = (codepoint_t*)encoding_transform_plain((uint8_t*)combined, strlen(combined), encoding_utf8_static(), encoding_native_static(), &keyword_encoded_length);
  free(combined);
  size_t value_encoded_length;
  codepoint_t* value_encoded = (codepoint_t*)encoding_transform_plain((uint8_t*)value, strlen(value), encoding_utf8_static(), encoding_native_static(), &value_encoded_length);
  config_update(config, keyword_encoded, keyword_encoded_length/sizeof(codepoint_t), value_encoded, value_encoded_length/sizeof(codepoint_t));
  free(value_encoded);
  free(keyword_encoded);
}

// Load keyword value as string from state configuration
char* editor_state_load_keyword_string(struct editor* base, struct config* config, const char* path, const char* keyword) {
  char* combined = combine_string(path, keyword);
  char* value = (char*)config_convert_encoding(config_find_ascii(config, combined), encoding_utf8_static(), NULL);
  free(combined);
  return value;
}

// Append keyword and int value to state configuration
void editor_state_save_keyword_int64(struct editor* base, struct config* config, const char* path, const char* keyword, int64_t value) {
  char converted[1024];
  sprintf(&converted[0], "%lld", (long long int)value);
  editor_state_save_keyword_string(base, config, path, keyword, &converted[0]);
}

// Load keyword value as int from state configuration
int64_t editor_state_load_keyword_int64(struct editor* base, struct config* config, const char* path, const char* keyword) {
  char* combined = combine_string(path, keyword);
  int64_t value = config_convert_int64(config_find_ascii(config, combined));
  free(combined);
  return value;
}

// Append view parameters to state configuration
void editor_state_save_view(struct editor* base, struct config* config, const char* path, struct document_view* view) {
  editor_state_save_keyword_int64(base, config, path, "/view/offset", (int64_t)view->offset);
  editor_state_save_keyword_int64(base, config, path, "/view/cursor_x", (int64_t)view->cursor_x);
  editor_state_save_keyword_int64(base, config, path, "/view/cursor_y", (int64_t)view->cursor_y);
  editor_state_save_keyword_int64(base, config, path, "/view/scroll_x", (int64_t)view->scroll_x);
  editor_state_save_keyword_int64(base, config, path, "/view/scroll_y", (int64_t)view->scroll_y);
  editor_state_save_keyword_int64(base, config, path, "/view/invisibles", (int64_t)view->show_invisibles);
  editor_state_save_keyword_int64(base, config, path, "/view/wrapping", (int64_t)view->wrapping);
  editor_state_save_keyword_int64(base, config, path, "/view/line_select", (int64_t)view->line_select);
}

// Load view parameters from state configuration
void editor_state_load_view(struct editor* base, struct config* config, const char* path, struct document_view* view) {
  view->offset = (file_offset_t)editor_state_load_keyword_int64(base, config, path, "/view/offset");
  view->cursor_x = (position_t)editor_state_load_keyword_int64(base, config, path, "/view/cursor_x");
  view->cursor_y = (position_t)editor_state_load_keyword_int64(base, config, path, "/view/cursor_y");
  view->scroll_x = (position_t)editor_state_load_keyword_int64(base, config, path, "/view/scroll_x");
  view->scroll_y = (position_t)editor_state_load_keyword_int64(base, config, path, "/view/scroll_y");
  view->show_invisibles = (int)editor_state_load_keyword_int64(base, config, path, "/view/invisibles");
  view->wrapping = (int)editor_state_load_keyword_int64(base, config, path, "/view/wrapping");
  view->line_select = (int)editor_state_load_keyword_int64(base, config, path, "/view/line_select");
}

// Append splitter parameters to state configuration
void editor_state_save_splitter(struct editor* base, struct config* config, const char* path, struct splitter* splitter) {
  if (splitter->side[0] && splitter->side[1]) {
    char* combined0 = combine_string(path, "/0");
    char* combined1 = combine_string(path, "/1");
    editor_state_save_splitter(base, config, combined0, splitter->side[0]);
    editor_state_save_splitter(base, config, combined1, splitter->side[1]);
    free(combined1);
    free(combined0);
    editor_state_save_keyword_int64(base, config, path, "/type", (int64_t)splitter->type);
    editor_state_save_keyword_int64(base, config, path, "/split", (int64_t)splitter->split);
  } else {
    editor_state_save_keyword_int64(base, config, path, "/focus", (splitter==base->document)?1:0);
    editor_state_save_keyword_string(base, config, path, "/name", splitter->name);
    if (splitter->file->save && !splitter->file->draft) {
      editor_state_save_keyword_string(base, config, path, "/file", splitter->file->filename);
    }

    editor_state_save_keyword_string(base, config, path, "/style", (splitter->document==splitter->document_hex)?"hex":"text");
    editor_state_save_view(base, config, path, splitter->view);
  }
}

// Load splitter parameters to state configuration
struct splitter* editor_state_load_splitter(struct editor* base, struct config* config, const char* path, struct splitter** focus) {
  char* style = editor_state_load_keyword_string(base, config, path, "/style");
  char* name = editor_state_load_keyword_string(base, config, path, "/name");
  int type = editor_state_load_keyword_int64(base, config, path, "/type");
  int split = editor_state_load_keyword_int64(base, config, path, "/split");

  struct splitter* splitter;
  if (type) {
    char* combined0 = combine_string(path, "/0");
    char* combined1 = combine_string(path, "/1");
    struct splitter* split0 = editor_state_load_splitter(base, config, combined0, focus);
    struct splitter* split1 = editor_state_load_splitter(base, config, combined1, focus);
    splitter = splitter_create(type, split, split0, split1, name);
    free(combined1);
    free(combined0);
  } else {
    splitter = splitter_create(type, split, NULL, NULL, name);
    if (editor_state_load_keyword_int64(base, config, path, "/focus")) {
      *focus = splitter;
    }

    char* file = editor_state_load_keyword_string(base, config, path, "/file");
    if (!*file) {
      struct document_file* empty = editor_empty_document(base);
      splitter_assign_document_file(splitter, empty);
      document_view_reset(splitter->view, empty, 1);
    } else {
      editor_open_document(base, file, NULL, splitter, TIPPSE_BROWSERTYPE_OPEN, NULL, NULL);
    }

    editor_state_load_view(base, config, path, splitter->view);
    if (strcmp(style, "hex")==0) {
      splitter->document = splitter->document_hex;
    } else {
      splitter->document = splitter->document_text;
      splitter->view->seek_x = splitter->view->cursor_x;
      splitter->view->seek_y = splitter->view->cursor_y;
    }

    free(file);
  }

  free(name);
  free(style);
  return splitter;
}

// Store desktop to state file
void editor_state_save(struct editor* base, const char* filename) {
  struct config* config = config_create();
  int count = 0;
  struct list_node* it = base->documents->last;
  while (it) {
    struct document_file* file = *(struct document_file**)list_object(it);
    if (file->save && !file->draft) {
      char path[1024];
      sprintf(&path[0], "/documents/%d", (int)count);
      editor_state_save_keyword_string(base, config, &path[0], "/file", file->filename);
      editor_state_save_keyword_int64(base, config, &path[0], "/inactive", file->view_inactive);
      // TODO: save document splitter stickiness
      if (file->view_inactive) {
        struct document_view* view = *(struct document_view**)list_object(file->views->first);
        editor_state_save_view(base, config, &path[0], view);
      }

      count++;
    }
    it = it->prev;
  }

  editor_state_save_splitter(base, config, "/splitter", base->splitters->side[1]);
  config_save(config, filename);
  config_destroy(config);
}

// Restore desktop from state file
void editor_state_load(struct editor* base, const char* filename) {
  struct config* config = config_create();
  config_load(config, filename);

  base->focus = NULL;
  splitter_destroy(base->splitters->side[1]);
  base->splitters->side[1] = NULL;
  struct splitter* focus = NULL;
  struct splitter* root = editor_state_load_splitter(base, config, "/splitter", &focus);
  base->splitters->side[1] = root;
  root->parent = base->splitters;

  if (!focus) {
    if (root->file) {
      focus = root;
    } else {
      focus = base->filter;
    }
  }

  int count = 0;
  while (1) {
    char path[1024];
    sprintf(&path[0], "/documents/%d", (int)count);
    char* filepath = editor_state_load_keyword_string(base, config, &path[0], "/file");
    if (!*filepath) {
      free(filepath);
      break;
    }

    struct document_file* file = NULL;
    editor_open_document(base, filepath, NULL, NULL, TIPPSE_BROWSERTYPE_OPEN, NULL, &file);
    int inactive = (int)editor_state_load_keyword_int64(base, config, &path[0], "/inactive");
    if (inactive && file && (file->views->count<1 || file->view_inactive)) {
      if (!file->view_inactive) {
        struct document_view* view = document_view_create();
        list_insert(file->views, NULL, &view);
        file->view_inactive = 1;
      }

      struct document_view* view = *((struct document_view**)list_object(file->views->first));
      document_view_reset(view, file, 1);
      editor_state_load_view(base, config, path, view);
    }

    count++;
    free(filepath);
  }

  base->document = focus;
  editor_focus(base, base->document, 1);

  config_destroy(config);
}

#ifdef _TESTSUITE
void editor_test_read(struct editor* base) {
  size_t count = 0;
  char buffer[1024*10+1];
  size_t pos = 0;
  char* params[10+1];

  params[count] = &buffer[pos];

  while (1) {
    if (pos>=1024*10) {
      fprintf(stderr, "test script line too long (%s)\r\n", base->test_script_path);
      exit(1);
    }

    if (count>=10) {
      fprintf(stderr, "test script too much parameters in line (%s)\r\n", base->test_script_path);
      exit(1);
    }

    char c = '\n';
    size_t in = file_read(base->test_script, &c, 1);

    if (c=='\r') {
    } else if (c==',' || c=='\n') {
      if (in==0 && pos==0) {
        fprintf(stderr, "test script end reached but editor is alive (%s)\r\n", base->test_script_path);
        exit(1);
      }

      buffer[pos++] = 0;
      count++;
      params[count] = &buffer[pos];

      if (c=='\n') {
        break;
      }
    } else {
      if (&buffer[pos]!=params[count] || (c!=' ' && c!='\t')) {
        buffer[pos++] = c;
      }
    }
  }

  if (count>=1) {
    if (strcmp(params[0], "key")==0 && count>=7) {
      editor_task_append(base, 3, TIPPSE_CMD_CHARACTER, NULL, (int)strtol(params[1], NULL, 0), (codepoint_t)strtol(params[2], NULL, 0), (int)strtol(params[3], NULL, 0), (int)strtol(params[4], NULL, 0), (int)strtol(params[5], NULL, 0), (int)strtol(params[6], NULL, 0), NULL);
    } else if (strcmp(params[0], "str")==0 && count>=3) {
      while (*params[2]) {
        editor_task_append(base, 3, TIPPSE_CMD_CHARACTER, NULL, (int)strtol(params[1], NULL, 0), (codepoint_t)*params[2], 0, 0, 0, 0, NULL);
        editor_task_dispatch(base);
        params[2]++;
      }
    } else if (strcmp(params[0], "cmd")==0 && count>=2) {
      for (int check = 0; editor_commands[check].text; check++) {
        if (strcmp(params[1], editor_commands[check].text)==0) {
          editor_task_append(base, 3, editor_commands[check].value, NULL, 0, 0, 0, 0, 0, 0, NULL);
        }
      }
    }
  }

  editor_task_dispatch(base);
}

#endif
