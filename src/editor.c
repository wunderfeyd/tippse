// Tippse - Editor - Glue all the components together for a usable editor

#include "editor.h"

const char* editor_key_names[TIPPSE_KEY_MAX] = {
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
  "f12"
};

struct config_cache editor_commands[TIPPSE_CMD_MAX+1] = {
  {"", TIPPSE_CMD_CHARACTER, ""},
  {"quit", TIPPSE_CMD_QUIT, "Exit application"},
  {"up", TIPPSE_CMD_UP, "Move cursor up"},
  {"down", TIPPSE_CMD_DOWN, "Move cursor down"},
  {"right", TIPPSE_CMD_RIGHT, "Move cursor right"},
  {"left", TIPPSE_CMD_LEFT, "Move cursor left"},
  {"pageup", TIPPSE_CMD_PAGEUP, "Move cursor one page up"},
  {"pagedown", TIPPSE_CMD_PAGEDOWN, "Move cursor one page down"},
  {"first", TIPPSE_CMD_FIRST, "Move cursor to first position of line (toggle on identation)"},
  {"last", TIPPSE_CMD_LAST, "Move cursor to last position of line"},
  {"home", TIPPSE_CMD_HOME, "Move cursor to first position of file"},
  {"end", TIPPSE_CMD_END, "Move cursor to last position of file"},
  {"backspace", TIPPSE_CMD_BACKSPACE, "Remove character in front of the cursor"},
  {"delete", TIPPSE_CMD_DELETE, "Remove character at cursor position"},
  {"insert", TIPPSE_CMD_INSERT, "Unused"},
  {"search", TIPPSE_CMD_SEARCH, "Open search panel"},
  {"searchnext", TIPPSE_CMD_SEARCH_NEXT, "Find next occurrence of text to search"},
  {"undo", TIPPSE_CMD_UNDO, "Undo last operation"},
  {"redo", TIPPSE_CMD_REDO, "Repeat previous undone operation"},
  {"cut", TIPPSE_CMD_CUT, "Copy to clipboard and delete selection"},
  {"copy", TIPPSE_CMD_COPY, "Copy to clipboard selection and keep selection alive"},
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
  {NULL, 0, ""}
};

// Create editor
struct editor* editor_create(const char* base_path, struct screen* screen, int argc, const char** argv) {
  struct editor* base = (struct editor*)malloc(sizeof(struct editor));
  base->close = 0;
  base->base_path = base_path;
  base->screen = screen;
  base->focus = NULL;
  base->tick = tick_count();
  base->tick_undo = -1;
  base->tick_incremental = -1;
  base->search_regex = 0;
  base->search_ignore_case = 1;
  base->console_index = 0;
  base->console_status = 0;
  base->console_timeout = 0;
  base->console_text = NULL;

  editor_command_map_create(base);

  base->documents = list_create(sizeof(struct document_file*));

  base->tabs_doc = document_file_create(0, 1, base);
  document_file_name(base->tabs_doc, "Open");
  base->tabs_doc->defaults.wrapping = 0;
  base->tabs_doc->line_select = 1;

  base->browser_doc = document_file_create(0, 1, base);
  document_file_name(base->browser_doc, base->base_path);
  base->browser_doc->defaults.wrapping = 0;
  base->browser_doc->line_select = 1;

  base->commands_doc = document_file_create(0, 1, base);
  document_file_name(base->commands_doc, base->base_path);
  base->commands_doc->defaults.wrapping = 0;
  base->commands_doc->line_select = 1;

  base->document = splitter_create(0, 0, NULL, NULL,  "");

  base->search_doc = document_file_create(0, 1, base);
  base->replace_doc = document_file_create(0, 1, base);
  editor_update_search_title(base);

  base->goto_doc = document_file_create(0, 1, base);
  document_file_name(base->goto_doc, "Goto");

  base->compiler_doc = document_file_create(0, 1, base);

  base->search_results_doc = document_file_create(0, 1, base);

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

  for (int n = argc-1; n>=1; n--) {
    editor_open_document(base, argv[n], NULL, base->document);
  }

  if (!base->document->file) {
    struct document_file* empty = editor_empty_document(base);
    splitter_assign_document_file(base->document, empty);
    document_view_reset(base->document->view, empty);
  }

  editor_focus(base, base->document, 0);

  return base;
}

// Destroy editor
void editor_destroy(struct editor* base) {
  splitter_destroy(base->splitters);

  while (base->documents->first) {
    document_file_destroy(*(struct document_file**)list_object(base->documents->first));
    list_remove(base->documents, base->documents->first);
  }

  list_destroy(base->documents);
  editor_command_map_destroy(base);
  free(base->console_text);
  free(base);
}

// Check height of panel content
int editor_update_panel_height(struct editor* base, struct splitter* panel, int max) {
  (*panel->document->incremental_update)(panel->document, panel);

  int height = (int)(panel->file->buffer?panel->file->buffer->visuals.ys:0)+1;
  if (height>max) {
    height = max;
  }

  return height;
}

// Refresh editor components
void editor_draw(struct editor* base) {
  if (base->focus==base->panel || base->focus==base->filter) {
    int filter_height = (base->focus==base->filter)?(editor_update_panel_height(base, base->filter, (base->screen->height/4)+1)):0;
    int panel_height = (base->screen->height/2)+1-filter_height-(filter_height>0?1:0);
    if (base->panel->file!=base->browser_doc && base->panel->file!=base->tabs_doc && base->panel->file!=base->commands_doc) {
      panel_height = editor_update_panel_height(base, base->panel, (base->screen->height/2)+1-filter_height);;
    }

    base->toolbox->split = filter_height;
    base->splitters->split = filter_height+panel_height+(filter_height>0?1:0);
  } else {
    base->splitters->split = 0;
  }

  screen_check(base->screen);
  splitter_draw_multiple(base->splitters, base->screen, 0);

  int foreground = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_STATUS];
  int background = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
  for (int x = 0; x<base->screen->width; x++) {
    codepoint_t cp = 0x20;
    screen_setchar(base->screen, x, 0, 0, 0, base->screen->width, base->screen->height, &cp, 1, foreground, background);
  }

  int running = 0;
  struct list_node* docs = base->documents->first;
  while (docs && !running) {
    running |= ((*(struct document_file**)list_object(docs))->pipefd[0]!=-1)?1:0;
    docs = docs->next;
  }

  int x = 0;
  if (running) {
    codepoint_t cp = 'R';
    screen_setchar(base->screen, x, 0, 0, 0, base->screen->width, base->screen->height, &cp, 1, base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_TEXT], background);
    x++;
  }

  if (document_file_modified_cache(base->document->file)) {
    codepoint_t cp = 'M';
    screen_setchar(base->screen, x, 0, 0, 0, base->screen->width, base->screen->height, &cp, 1, base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_TEXT], background);
    x++;
  }

  screen_drawtext(base->screen, (x>0)?x+1:x, 0, 0, 0, base->screen->width, base->screen->height, base->focus->name, (size_t)base->screen->width, foreground, background);

  int64_t tick = tick_count();
  if (base->console_status!=base->console_index) {
    base->console_timeout = tick+1000000;
    base->console_status = base->console_index;
  }

  const char* status = base->focus->status;
  if (tick<base->console_timeout && base->console_text) {
    status = base->console_text;
    if (base->console_color==VISUAL_FLAG_COLOR_CONSOLENORMAL) {
      foreground = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
      background = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_STATUS];
    } else {
      foreground = base->focus->file->defaults.colors[base->console_color];
    }
  }

  struct stream stream;
  stream_from_plain(&stream, (uint8_t*)status, SIZE_T_MAX);
  size_t length = encoding_utf8_strlen(NULL, &stream);
  screen_drawtext(base->screen, base->screen->width-(int)length, 0, 0, 0, base->screen->width, base->screen->height, status, length, foreground, background);

  screen_draw(base->screen);
}

// Regular timer event (schedule processes)
void editor_tick(struct editor* base) {
  int64_t tick = tick_count();
  if (tick>base->tick_undo) {
    base->tick_undo = tick+500000;
    struct list_node* doc = base->documents->first;
    while (doc) {
      struct document_file* file = *(struct document_file**)list_object(doc);
      document_undo_chain(file, file->undos);
      doc = doc->next;
    }
  }

  if (tick>base->tick_incremental) {
    base->tick_incremental = tick+100000;
    splitter_draw_multiple(base->splitters, base->screen, 1);
  }
}

// An input event was signalled ... translate it to a command if possible
void editor_keypress(struct editor* base, int key, codepoint_t cp, int button, int button_old, int x, int y) {
  base->tick_undo = tick_count()+500000;

  char key_lookup[1024];
  const char* key_name = editor_key_names[key&TIPPSE_KEY_MASK];
  sprintf(&key_lookup[0], "/keys/%s%s%s%s", (key&TIPPSE_KEY_MOD_SHIFT)?"shift+":"", (key&TIPPSE_KEY_MOD_CTRL)?"ctrl+":"", (key&TIPPSE_KEY_MOD_ALT)?"alt+":"", (cp!=0)?"":key_name);

  struct trie_node* parent = config_find_ascii(base->focus->file->config, &key_lookup[0]);
  if (cp!=0 && parent) {
    parent = config_advance_codepoints(base->focus->file->config, parent, &cp, 1);
  }

  if (!parent || !parent->end) {
    editor_intercept(base, TIPPSE_CMD_CHARACTER, NULL, key, cp, button, button_old, x, y);
    return;
  }

  // Copy and execute (copy is needed in case of file access commands altering the config structure)
  struct config_value* value = config_value_clone((struct config_value*)list_object(*(struct list_node**)trie_object(parent)));
  struct list_node* it = value->commands.first;
  while (it) {
    struct config_command* arguments = (struct config_command*)list_object(it);
    int command = (int)config_command_cache(arguments, &editor_commands[0]);
    if (command!=TIPPSE_CMD_CHARACTER) {
      editor_intercept(base, command, arguments, key, cp, button, button_old, x, y);
    }
    it = it->next;
  }
  config_value_destroy(value);
}

// After event translation we can intercept the core commands (for now)
void editor_intercept(struct editor* base, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y) {
  if (command==TIPPSE_CMD_MOUSE) {
    struct splitter* select = splitter_by_coordinate(base->splitters, x, y);
    if (select && button!=0 && button_old==0) {
      editor_focus(base, select, 1);
    }
  }

  if (command==TIPPSE_CMD_DOCUMENTSELECTION) {
    editor_view_tabs(base, NULL, NULL);
  } else if (command==TIPPSE_CMD_BROWSER) {
    editor_view_browser(base, NULL, NULL, NULL);
  } else if (command==TIPPSE_CMD_COMMANDS) {
    editor_view_commands(base, NULL, NULL);
  }

  if (command==TIPPSE_CMD_QUIT) {
    base->close = 1;
  } else if (command==TIPPSE_CMD_SEARCH) {
    editor_search(base);
  } else if (command==TIPPSE_CMD_SEARCH_NEXT) {
    editor_focus(base, base->document, 1);
    document_search(base->last_document, base->search_doc->buffer, base->search_doc->encoding, NULL, NULL, 0, base->search_ignore_case, base->search_regex, 0, 0);
  } else if (command==TIPPSE_CMD_SEARCH_PREV) {
    editor_focus(base, base->document, 1);
    document_search(base->last_document, base->search_doc->buffer, base->search_doc->encoding, NULL, NULL, 1, base->search_ignore_case, base->search_regex, 0, 0);
  } else if (command==TIPPSE_CMD_SEARCH_DIRECTORY) {
    if (base->document->file->pipefd[1]==-1 && base->document->file==base->search_results_doc) {
      document_file_create_pipe(base->document->file);
      if (base->document->file->pid==0) {
        document_search_directory(base->base_path, base->search_doc->buffer, base->search_doc->encoding, NULL, NULL, base->search_ignore_case, base->search_regex, 0);
        exit(0);
      }
    } else {
      splitter_assign_document_file(base->document, base->search_results_doc);
    }
  } else if (command==TIPPSE_CMD_REPLACE) {
    editor_panel_assign(base, base->replace_doc);
    document_select_all(base->panel, 1);
  } else if (command==TIPPSE_CMD_REPLACE_NEXT) {
    editor_focus(base, base->document, 1);
    document_search(base->last_document, base->search_doc->buffer, base->search_doc->encoding, base->replace_doc->buffer, base->replace_doc->encoding, 0, base->search_ignore_case, base->search_regex, 0, 1);
  } else if (command==TIPPSE_CMD_REPLACE_PREV) {
    editor_focus(base, base->document, 1);
    document_search(base->last_document, base->search_doc->buffer, base->search_doc->encoding, base->replace_doc->buffer, base->replace_doc->encoding, 1, base->search_ignore_case, base->search_regex, 0, 1);
  } else if (command==TIPPSE_CMD_REPLACE_ALL) {
    editor_focus(base, base->document, 1);
    document_search(base->last_document, base->search_doc->buffer, base->search_doc->encoding, base->replace_doc->buffer, base->replace_doc->encoding, 0, base->search_ignore_case, base->search_regex, 1, 1);
  } else if (command==TIPPSE_CMD_GOTO) {
    editor_panel_assign(base, base->goto_doc);
    document_select_all(base->panel, 1);
  } else if (command==TIPPSE_CMD_CONSOLE) {
    editor_panel_assign(base, base->console_doc);
  } else if (command==TIPPSE_CMD_VIEW_SWITCH) {
    if (base->focus->document==base->focus->document_hex) {
      base->focus->document = base->focus->document_text;
    } else {
      base->focus->document = base->focus->document_hex;
    }
  } else if (command==TIPPSE_CMD_CLOSE) {
    editor_close_document(base, base->focus->file);
  } else if (command==TIPPSE_CMD_RETURN && base->focus->file==base->goto_doc) {
    struct stream stream;
    stream_from_page(&stream, base->focus->file->buffer, 0);
    struct encoding_cache cache;
    encoding_cache_clear(&cache, base->focus->file->encoding, &stream);
    // TODO: this should be handled by the specialized document classes
    if (base->document->document==base->document->document_hex) {
      file_offset_t offset = (file_offset_t)decode_based_unsigned(&cache, 16, SIZE_T_MAX);
      file_offset_t length = base->document->file->buffer?base->document->file->buffer->length:0;
      if (offset>length) {
        offset = length;
      }

      base->document->view->offset = offset;
      base->document->view->show_scrollbar = 1;
    } else {
      position_t line = (position_t)decode_based_unsigned(&cache, 10, SIZE_T_MAX);
      if (line>0) {
        document_text_goto(base->document->document, base->document, line-1);
      }
    }

    editor_focus(base, base->document, 1);
  } else if (command==TIPPSE_CMD_OPEN) {
    editor_open_selection(base, base->focus, base->document);
  } else if (command==TIPPSE_CMD_SPLIT) {
    editor_split(base, base->document);
  } else if (command==TIPPSE_CMD_UNSPLIT) {
    struct splitter* document = editor_unsplit(base, base->document);
    if (base->focus==base->document) {
      editor_focus(base, document, 0);
    }
    base->document = document;
  } else if (command==TIPPSE_CMD_RELOAD) {
    editor_reload_document(base, base->document->file);
  } else if (command==TIPPSE_CMD_SAVE) {
    editor_save_document(base, base->document->file);
  } else if (command==TIPPSE_CMD_SAVEALL) {
    editor_save_documents(base);
  } else if (command==TIPPSE_CMD_SHELL) {
    if (base->document->file->pipefd[1]==-1 && base->document->file==base->compiler_doc) {
      if (arguments && arguments->length>1) {
        struct trie_node* parent = config_find_ascii(base->focus->file->config, "/shell/");
        if (parent) {
          parent = config_advance_codepoints(base->focus->file->config, parent, arguments->arguments[1].codepoints, arguments->arguments[1].length);
        }

        // TODO: Use current shell encoding (get it somewhere)
        struct encoding* utf8 = encoding_utf8_create();
        char* shell = (char*)config_convert_encoding(parent, utf8);
        document_file_pipe(base->document->file, shell);
        free(shell);
        encoding_utf8_destroy(utf8);
      }
    } else {
      splitter_assign_document_file(base->document, base->compiler_doc);
    }
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
  } else {
    if (base->focus->file==base->tabs_doc || base->focus->file==base->browser_doc || base->focus->file==base->commands_doc || base->focus->file==base->filter_doc) {
      file_offset_t before = base->filter->file->buffer?base->filter->file->buffer->length:0;
      if (command==TIPPSE_CMD_UP || command==TIPPSE_CMD_DOWN || command==TIPPSE_CMD_PAGEDOWN || command==TIPPSE_CMD_PAGEUP || command==TIPPSE_CMD_HOME || command==TIPPSE_CMD_END || command==TIPPSE_CMD_RETURN) {
        (*base->panel->document->keypress)(base->panel->document, base->panel, command, arguments, key, cp, button, button_old, x-base->document->x, y-base->focus->y);
      } else {
        (*base->filter->document->keypress)(base->filter->document, base->filter, command, arguments, key, cp, button, button_old, x-base->filter->x, y-base->panel->y);
      }

      int selected = 0;
      if (command==TIPPSE_CMD_RETURN) {
        selected = editor_open_selection(base, base->panel, base->document);
      }

      if (!selected) {
        file_offset_t now = base->filter_doc->buffer?base->filter_doc->buffer->length:0;
        if (before!=now) {
          struct stream* filter_stream = NULL;
          struct stream stream;
          if (base->filter_doc->buffer) {
            stream_from_page(&stream, range_tree_first(base->filter_doc->buffer), 0);
            filter_stream = &stream;
          }
          if (base->panel->file==base->browser_doc) {
            editor_view_browser(base, NULL, filter_stream, base->filter_doc->encoding);
          } else if (base->panel->file==base->tabs_doc) {
            editor_view_tabs(base, filter_stream, base->filter_doc->encoding);
          } else if (base->panel->file==base->commands_doc) {
            editor_view_commands(base, filter_stream, base->filter_doc->encoding);
          }
        }

        editor_focus(base, now?base->filter:base->panel, 1);
      }
    } else {
      (*base->focus->document->keypress)(base->focus->document, base->focus, command, arguments, key, cp, button, button_old, x-base->focus->x, y-base->focus->y);
    }
  }
}

// Set focus to other document
void editor_focus(struct editor* base, struct splitter* node, int disable) {
  if (base->focus==node) {
    return;
  }

  if (base->focus && disable) {
    base->focus->active = 0;
  }

  base->focus = node;
  base->focus->active = 1;
  if (node->file->save) {
    base->document = node;
  }

  if (node!=base->panel && node!=base->filter) {
    base->last_document = node;
  }
}

// Split view
void editor_split(struct editor* base, struct splitter* node) {
  struct splitter* parent = node->parent;
  struct splitter* split = splitter_create(0, 0, NULL, NULL, "Document");

  splitter_assign_document_file(split, node->file);

  struct splitter* splitter = splitter_create(TIPPSE_SPLITTER_HORZ, 50, node, split, "");
  if (parent->side[0]==node) {
    parent->side[0] = splitter;
  } else {
    parent->side[1] = splitter;
  }
  splitter->parent = parent;
}

// Combine splitted view
struct splitter* editor_unsplit(struct editor* base, struct splitter* node) {
  if (node->side[0] || node->side[1]) {
    return node;
  }

  struct splitter* parent = node->parent;
  if (base->splitters==parent) {
    return node;
  }

  struct splitter* parentup = parent->parent;
  if (!parentup) {
    return node;
  }

  struct splitter* other = (node!=parent->side[0])?parent->side[0]:parent->side[1];

  if (parentup->side[0]==parent) {
    parentup->side[0] = other;
  } else {
    parentup->side[1] = other;
  }

  other->parent = parentup;

  parent->side[0] = NULL;
  parent->side[1] = NULL;
  splitter_destroy(parent);
  splitter_destroy(node);
  return other;
}

// Use selection from specified view to open a new document
int editor_open_selection(struct editor* base, struct splitter* node, struct splitter* destination) {
  int done = 0;

  if (node->view->selection_low!=node->view->selection_high) {
    char* name = (char*)range_tree_raw(node->file->buffer, node->view->selection_low, node->view->selection_high);
    if (*name) {
      editor_focus(base, destination, 1);
      done = 1;
      if (base->panel->file!=base->commands_doc) {
        editor_open_document(base, name, node, destination);
      } else {
        for (size_t n = 0; editor_commands[n].text; n++) {
          const char* compare1 = name;
          const char* compare2 = editor_commands[n].text;
          while (*compare1!=0 && *compare1!=' ' && *compare1==*compare2) {
            compare1++;
            compare2++;
          }

          if (*compare1==' ') {
            editor_intercept(base, (int)n, NULL, 0, 0, 0, 0, 0, 0);
            break;
          }
        }
      }
    }

    free(name);
  } else {
    if (node->document==node->document_text) {
      file_offset_t offset = document_text_line_start_offset(node->document, node);
      file_offset_t displacement;
      struct range_tree_node* buffer = range_tree_find_offset(node->file->buffer, offset, &displacement);
      struct stream text_stream;
      stream_from_page(&text_stream, buffer, displacement);

      const char* filter = "\\s*([^\\n\\r]*?)\\s*\\:\\s*(\\d+)\\s*\\:";
      struct stream filter_stream;
      stream_from_plain(&filter_stream, (uint8_t*)filter, strlen(filter));

      struct search* search = search_create_regex(1, 0, filter_stream, node->file->encoding, node->file->encoding);
      if (search_find_check(search, &text_stream)) {
        char* name = (char*)range_tree_raw(node->file->buffer, stream_offset(&search->group_hits[0].start), stream_offset(&search->group_hits[0].end));
        if (name) {
          editor_focus(base, destination, 1);
          done = 1;
          editor_open_document(base, name, NULL, destination);
          struct encoding_cache cache;
          encoding_cache_clear(&cache, base->focus->file->encoding, &search->group_hits[1].start);
          position_t line = (position_t)decode_based_unsigned(&cache, 10, SIZE_T_MAX);
          if (line>0) {
            document_text_goto(node->document, node, line-1);
          }
        }
        free(name);
      }
      search_destroy(search);
    }
  }

  return done;
}

// Open file into destination splitter
void editor_open_document(struct editor* base, const char* name, struct splitter* node, struct splitter* destination) {
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

  struct document_file* new_document_doc = NULL;
  struct list_node* docs = base->documents->first;
  while (docs) {
    struct document_file* docs_document_doc = *(struct document_file**)list_object(docs);
    if (strcmp(docs_document_doc->filename, relative)==0 && (!node || docs_document_doc!=node->file)) {
      new_document_doc = docs_document_doc;
      list_remove(base->documents, docs);
      break;
    }

    docs = docs->next;
  }

  if (!new_document_doc) {
    if (is_directory(relative)) {
      if (destination) {
        editor_view_browser(base, relative, NULL, NULL);
      }
    } else {
      new_document_doc = document_file_create(1, 1, base);
      document_file_load(new_document_doc, relative, 0, 1);
    }
  }

  if (new_document_doc) {
    list_insert(base->documents, NULL, &new_document_doc);
    if (destination) {
      splitter_assign_document_file(destination, new_document_doc);
    }
  }

  free(relative);
  free(combined);
  free(path_only);
  free(corrected);
}

// Reload single document
void editor_reload_document(struct editor* base, struct document_file* file) {
  if (!file->save) {
    return;
  }

  document_file_load(file, file->filename, 0, 0);
}

// Save single modified document
void editor_save_document(struct editor* base, struct document_file* file) {
  if ((document_undo_modified(file) || document_file_modified_cache(file)) && file->save) {
    document_file_save(file, file->filename);
    document_file_detect_properties(file);
  }
}

// Save all modified documents
void editor_save_documents(struct editor* base) {
  struct list_node* docs = base->documents->first;
  while (docs) {
    editor_save_document(base, *(struct document_file**)list_object(docs));
    docs = docs->next;
  }
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
    } else {
      if (doc->save) {
        assign = doc;
      }
    }

    docs = docs->next;
  }

  if (remove) {
    if (!assign) {
      assign = editor_empty_document(base);
    }

    struct document_file* replace = *(struct document_file**)list_object(remove);
    splitter_exchange_document_file(base->splitters, replace, assign);
    document_file_destroy(replace);
    list_remove(base->documents, remove);
  }
}

// Create empty document
struct document_file* editor_empty_document(struct editor* base) {
  struct document_file* file = document_file_create(1, 1, base);
  document_file_name(file, "Untitled");
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

// Update and change to browser view
void editor_view_browser(struct editor* base, const char* filename, struct stream* filter_stream, struct encoding* filter_encoding) {
  if (!filter_stream) {
    editor_filter_clear(base);
  }

  editor_panel_assign(base, base->browser_doc);

  if (filename!=NULL) {
    document_file_name(base->panel->file, filename);
  }

  document_directory(base->browser_doc, filter_stream, filter_encoding);
  document_file_change_views(base->browser_doc);
  document_view_reset(base->panel->view, base->browser_doc);
  base->panel->view->line_select = 1;

  (*base->panel->document->keypress)(base->panel->document, base->panel, TIPPSE_CMD_RETURN, NULL, 0, 0, 0, 0, 0, 0);
}

// Update and change to document view
void editor_view_tabs(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding) {
  if (!filter_stream) {
    editor_filter_clear(base);
  }

  editor_panel_assign(base, base->tabs_doc);

  struct search* search = filter_stream?search_create_plain(1, 0, *filter_stream, filter_encoding, base->tabs_doc->encoding):NULL;

  base->tabs_doc->buffer = range_tree_delete(base->tabs_doc->buffer, 0, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, 0, base->tabs_doc);
  struct list_node* doc = base->documents->first;
  while (doc) {
    struct document_file* file = *(struct document_file**)list_object(doc);
    struct stream text_stream;
    stream_from_plain(&text_stream, (uint8_t*)file->filename, strlen(file->filename));
    if (file->save && (!search || search_find(search, &text_stream, NULL))) {
      if (base->tabs_doc->buffer) {
        base->tabs_doc->buffer = range_tree_insert_split(base->tabs_doc->buffer, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, (uint8_t*)"\n", 1, 0);
      }

      base->tabs_doc->buffer = range_tree_insert_split(base->tabs_doc->buffer, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, (uint8_t*)file->filename, strlen(file->filename), 0);
    }

    doc = doc->next;
  }

  if (search) {
    search_destroy(search);
  }

  document_file_change_views(base->tabs_doc);
  document_view_reset(base->panel->view, base->tabs_doc);
  base->panel->view->line_select = 1;

  (*base->panel->document->keypress)(base->panel->document, base->panel, TIPPSE_CMD_RETURN, NULL, 0, 0, 0, 0, 0, 0);
}

// Update and change to commands view
void editor_view_commands(struct editor* base, struct stream* filter_stream, struct encoding* filter_encoding) {
  if (!filter_stream) {
    editor_command_map_read(base, base->document->file);
    editor_filter_clear(base);
  }

  editor_panel_assign(base, base->commands_doc);

  struct search* search = filter_stream?search_create_plain(1, 0, *filter_stream, filter_encoding, base->tabs_doc->encoding):NULL;

  base->commands_doc->buffer = range_tree_delete(base->commands_doc->buffer, 0, base->commands_doc->buffer?base->commands_doc->buffer->length:0, 0, base->commands_doc);
  for (size_t n = 1; editor_commands[n].text; n++) {
    char output[4096];
    // TODO: Encoding may destroy equal width ... build string in a different way
    sprintf(&output[0], "%-16s | %-16s | %s", editor_commands[n].text, base->command_map[n]?base->command_map[n]:"<none>", editor_commands[n].description);

    struct stream text_stream;
    stream_from_plain(&text_stream, (uint8_t*)&output[0], strlen(&output[0]));
    if (!search || search_find(search, &text_stream, NULL)) {
      if (base->commands_doc->buffer) {
        base->commands_doc->buffer = range_tree_insert_split(base->commands_doc->buffer, base->commands_doc->buffer?base->commands_doc->buffer->length:0, (uint8_t*)"\n", 1, 0);
      }

      base->commands_doc->buffer = range_tree_insert_split(base->commands_doc->buffer, base->commands_doc->buffer?base->commands_doc->buffer->length:0, (uint8_t*)&output[0], strlen(&output[0]), 0);
    }
  }

  if (search) {
    search_destroy(search);
  }

  document_file_change_views(base->commands_doc);
  document_view_reset(base->panel->view, base->commands_doc);
  base->panel->view->line_select = 1;

  (*base->panel->document->keypress)(base->panel->document, base->panel, TIPPSE_CMD_RETURN, NULL, 0, 0, 0, 0, 0, 0);
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

  base->console_doc->buffer = range_tree_insert_split(base->console_doc->buffer, base->console_doc->buffer?base->console_doc->buffer->length:0, (uint8_t*)text, length, TIPPSE_INSERTER_HIGHLIGHT|(base->console_color<<TIPPSE_INSERTER_HIGHLIGHT_COLOR_SHIFT));
  base->console_doc->buffer = range_tree_insert_split(base->console_doc->buffer, base->console_doc->buffer?base->console_doc->buffer->length:0, (uint8_t*)"\n", 1, 0);
  base->console_index++;
}

// Update search dialog
void editor_search(struct editor* base) {
  editor_panel_assign(base, base->search_doc);
  if (base->document->view->selection_reset && base->document->view->selection_low!=FILE_OFFSET_T_MAX) {
    // TODO: update encoding in search panel? or transform to encoding?
    document_file_delete(base->search_doc, 0, base->search_doc->buffer?base->search_doc->buffer->length:0);
    struct range_tree_node* buffer = range_tree_copy(base->document->file->buffer, base->document->view->selection_low, base->document->view->selection_high-base->document->view->selection_low);
    document_file_insert_buffer(base->search_doc, 0, buffer);
    range_tree_destroy(buffer, NULL);
  }
  document_select_all(base->panel, 1);
  base->document->view->selection_reset = 0;
}

// Empty filter text
void editor_filter_clear(struct editor* base) {
  if (base->filter_doc->buffer) {
    document_file_delete(base->filter_doc, 0, base->filter_doc->buffer->length);
  }
}
