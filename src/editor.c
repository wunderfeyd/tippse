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
  {"", TIPPSE_CMD_CHARACTER},
  {"quit", TIPPSE_CMD_QUIT},
  {"up", TIPPSE_CMD_UP},
  {"down", TIPPSE_CMD_DOWN},
  {"right", TIPPSE_CMD_RIGHT},
  {"left", TIPPSE_CMD_LEFT},
  {"pageup", TIPPSE_CMD_PAGEUP},
  {"pagedown", TIPPSE_CMD_PAGEDOWN},
  {"first", TIPPSE_CMD_FIRST},
  {"last", TIPPSE_CMD_LAST},
  {"home", TIPPSE_CMD_HOME},
  {"end", TIPPSE_CMD_END},
  {"backspace", TIPPSE_CMD_BACKSPACE},
  {"delete", TIPPSE_CMD_DELETE},
  {"insert", TIPPSE_CMD_INSERT},
  {"search", TIPPSE_CMD_SEARCH},
  {"searchnext", TIPPSE_CMD_SEARCH_NEXT},
  {"undo", TIPPSE_CMD_UNDO},
  {"redo", TIPPSE_CMD_REDO},
  {"cut", TIPPSE_CMD_CUT},
  {"copy", TIPPSE_CMD_COPY},
  {"paste", TIPPSE_CMD_PASTE},
  {"tab", TIPPSE_CMD_TAB},
  {"untab", TIPPSE_CMD_UNTAB},
  {"save", TIPPSE_CMD_SAVE},
  {"mouse", TIPPSE_CMD_MOUSE},
  {"searchprevious", TIPPSE_CMD_SEARCH_PREV},
  {"open", TIPPSE_CMD_OPEN},
  {"split", TIPPSE_CMD_SPLIT},
  {"invisibles", TIPPSE_CMD_SHOW_INVISIBLES},
  {"browser", TIPPSE_CMD_BROWSER},
  {"switch", TIPPSE_CMD_VIEW_SWITCH},
  {"bookmark", TIPPSE_CMD_BOOKMARK},
  {"wordwrap", TIPPSE_CMD_WORDWRAP},
  {"documents", TIPPSE_CMD_DOCUMENTSELECTION},
  {"return", TIPPSE_CMD_RETURN},
  {"selectall", TIPPSE_CMD_SELECT_ALL},
  {"selectup", TIPPSE_CMD_SELECT_UP},
  {"selectdown", TIPPSE_CMD_SELECT_DOWN},
  {"selectright", TIPPSE_CMD_SELECT_RIGHT},
  {"selectleft", TIPPSE_CMD_SELECT_LEFT},
  {"selectpageup", TIPPSE_CMD_SELECT_PAGEUP},
  {"selectpagedown", TIPPSE_CMD_SELECT_PAGEDOWN},
  {"selectfirst", TIPPSE_CMD_SELECT_FIRST},
  {"selectlast", TIPPSE_CMD_SELECT_LAST},
  {"selecthome", TIPPSE_CMD_SELECT_HOME},
  {"selectend", TIPPSE_CMD_SELECT_END},
  {"close", TIPPSE_CMD_CLOSE},
  {"saveall", TIPPSE_CMD_SAVEALL},
  {"unsplit", TIPPSE_CMD_UNSPLIT},
  {"compile", TIPPSE_CMD_COMPILE},
  {"replace", TIPPSE_CMD_REPLACE},
  {"replacenext", TIPPSE_CMD_REPLACE_NEXT},
  {"replaceprevious", TIPPSE_CMD_REPLACE_PREV},
  {"replaceall", TIPPSE_CMD_REPLACE_ALL},
  {"goto", TIPPSE_CMD_GOTO},
  {NULL, 0}
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

  base->documents = list_create(sizeof(struct document_file*));

  base->tabs_doc = document_file_create(0, 1);
  document_file_name(base->tabs_doc, "Open");
  base->tabs_doc->defaults.wrapping = 0;
  base->tabs_doc->line_select = 1;

  base->browser_doc = document_file_create(0, 1);
  document_file_name(base->browser_doc, base->base_path);
  base->browser_doc->defaults.wrapping = 0;
  base->browser_doc->line_select = 1;

  base->document_doc = document_file_create(1, 1);
  document_file_name(base->document_doc, "Untitled");
  base->document = splitter_create(0, 0, NULL, NULL,  "");
  splitter_assign_document_file(base->document, base->document_doc);
  document_view_reset(base->document->view, base->document_doc);

  base->search_doc = document_file_create(0, 1);
  document_file_name(base->search_doc, "Search");

  base->replace_doc = document_file_create(0, 1);
  document_file_name(base->replace_doc, "Replace");

  base->goto_doc = document_file_create(0, 1);
  document_file_name(base->goto_doc, "Goto");

  base->compiler_doc = document_file_create(0, 1);

  base->panel = splitter_create(0, 0, NULL, NULL, "");
  splitter_assign_document_file(base->panel, base->search_doc);

  base->splitters = splitter_create(TIPPSE_SPLITTER_VERT|TIPPSE_SPLITTER_FIXED0, 5, base->panel, base->document, "");
  list_insert(base->documents, NULL, &base->document_doc);
  list_insert(base->documents, NULL, &base->tabs_doc);
  list_insert(base->documents, NULL, &base->search_doc);
  list_insert(base->documents, NULL, &base->replace_doc);
  list_insert(base->documents, NULL, &base->goto_doc);
  list_insert(base->documents, NULL, &base->browser_doc);
  list_insert(base->documents, NULL, &base->compiler_doc);

  for (int n = argc-1; n>=1; n--) {
    if (n==1) {
      document_file_load(base->document_doc, argv[n]);
      splitter_assign_document_file(base->document, base->document_doc);
    } else {
      struct document_file* document_add = document_file_create(1, 1);
      document_file_load(document_add, argv[n]);
      list_insert(base->documents, NULL, &document_add);
    }
  }

  editor_focus(base, base->document, 0);

  document_directory(base->browser_doc);
  document_file_manualchange(base->browser_doc);

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

  free(base);
}

// Refresh editor components
void editor_draw(struct editor* base) {
  base->tabs_doc->buffer = range_tree_delete(base->tabs_doc->buffer, 0, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, TIPPSE_INSERTER_AUTO, base->tabs_doc);
  struct list_node* doc = base->documents->first;
  while (doc) {
    struct document_file* file = *(struct document_file**)list_object(doc);
    if (file->save) {
      if (base->tabs_doc->buffer) {
        base->tabs_doc->buffer = range_tree_insert_split(base->tabs_doc->buffer, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, (uint8_t*)"\n", 1, TIPPSE_INSERTER_ESCAPE|TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      }

      base->tabs_doc->buffer = range_tree_insert_split(base->tabs_doc->buffer, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, (uint8_t*)file->filename, strlen(file->filename), TIPPSE_INSERTER_ESCAPE|TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
    }

    doc = doc->next;
  }

  document_file_manualchange(base->tabs_doc);

  if (base->focus==base->panel) {
    if (base->panel->document!=base->panel->document_hex) {
      document_text_incremental_update(base->panel->document, base->panel);
    }

    int height = (int)(base->panel->file->buffer?base->panel->file->buffer->visuals.ys:0)+1;
    int max = (base->screen->height/4)+1;
    if (height>max) {
      height = max;
    }

    base->splitters->split = height;
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
    x ++;
  }

  if (document_file_modified_cache(base->document->file)) {
    codepoint_t cp = 'M';
    screen_setchar(base->screen, x, 0, 0, 0, base->screen->width, base->screen->height, &cp, 1, base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_TEXT], background);
    x ++;
  }

  screen_drawtext(base->screen, (x>0)?x+1:x, 0, 0, 0, base->screen->width, base->screen->height, base->focus->name, (size_t)base->screen->width, foreground, background);
  struct encoding_stream stream;
  encoding_stream_from_plain(&stream, (uint8_t*)base->focus->status, SIZE_T_MAX);
  size_t length = encoding_utf8_strlen(NULL, &stream);
  screen_drawtext(base->screen, base->screen->width-(int)length, 0, 0, 0, base->screen->width, base->screen->height, base->focus->status, length, base->focus->status_inverted?background:foreground, base->focus->status_inverted?foreground:background);

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

  int command = (int)config_convert_int64_cache(parent, &editor_commands[0]);
  if (command!=TIPPSE_KEY_CHARACTER || cp>=0x20) {
    editor_intercept(base, command, key, cp, button, button_old, x, y);
  }
}

// After event translation we can intercept the core commands (for now)
void editor_intercept(struct editor* base, int command, int key, codepoint_t cp, int button, int button_old, int x, int y) {
  if (command==TIPPSE_CMD_MOUSE) {
    struct splitter* select = splitter_by_coordinate(base->splitters, x, y);
    if (select && button!=0 && button_old==0) {
      editor_focus(base, select, 1);
    }
  }

  if (command==TIPPSE_CMD_DOCUMENTSELECTION) {
    splitter_assign_document_file(base->document, base->tabs_doc);
  } else if (command==TIPPSE_CMD_BROWSER) {
    splitter_assign_document_file(base->document, base->browser_doc);
  }

  if (command==TIPPSE_CMD_QUIT) {
    base->close = 1;
  } else if (command==TIPPSE_CMD_SEARCH) {
    editor_panel_assign(base, base->search_doc);
  } else if (command==TIPPSE_CMD_SEARCH_NEXT) {
    editor_focus(base, base->document, 1);
    if (base->search_doc->buffer) {
      document_search(base->last_document, base->search_doc->buffer, NULL, 1, 0, 0);
    }
  } else if (command==TIPPSE_CMD_SEARCH_PREV) {
    editor_focus(base, base->document, 1);
    if (base->search_doc->buffer) {
      document_search(base->last_document, base->search_doc->buffer, NULL, 0, 0, 0);
    }
  } else if (command==TIPPSE_CMD_REPLACE) {
    editor_panel_assign(base, base->replace_doc);
  } else if (command==TIPPSE_CMD_REPLACE_NEXT) {
    editor_focus(base, base->document, 1);
    document_search(base->last_document, base->search_doc->buffer, base->replace_doc->buffer, 1, 0, 1);
  } else if (command==TIPPSE_CMD_REPLACE_PREV) {
    editor_focus(base, base->document, 1);
    document_search(base->last_document, base->search_doc->buffer, base->replace_doc->buffer, 0, 0, 1);
  } else if (command==TIPPSE_CMD_REPLACE_ALL) {
    editor_focus(base, base->document, 1);
    document_search(base->last_document, base->search_doc->buffer, base->replace_doc->buffer, 1, 1, 1);
  } else if (command==TIPPSE_CMD_GOTO) {
    editor_panel_assign(base, base->goto_doc);
  } else if (command==TIPPSE_CMD_VIEW_SWITCH) {
    if (base->focus->document==base->focus->document_hex) {
      base->focus->document = base->focus->document_text;
    } else {
      base->focus->document = base->focus->document_hex;
    }
  } else if (command==TIPPSE_CMD_CLOSE) {
    editor_close_document(base, base->focus->file);
  } else if (command==TIPPSE_CMD_RETURN && base->focus->file==base->goto_doc) {
    struct encoding_stream stream;
    encoding_stream_from_page(&stream, base->focus->file->buffer, 0);
    struct encoding_cache cache;
    encoding_cache_clear(&cache, base->focus->file->encoding, &stream);
    // TODO: this should be handled by the specialized document classes
    if (base->document->document==base->document->document_hex) {
      file_offset_t offset = (file_offset_t)decode_based_unsigned(&cache, 16);
      file_offset_t length = base->document->file->buffer?base->document->file->buffer->length:0;
      if (offset>length) {
        offset = length;
      }

      base->document->view->offset = offset;
      base->document->view->show_scrollbar = 1;
    } else {
      position_t line = (position_t)decode_based_unsigned(&cache, 10);
      if (line>0) {
        document_text_goto(base->document->document, base->document, line-1);
      }
    }

    editor_focus(base, base->document, 1);
  } else if (command==TIPPSE_CMD_OPEN || (command==TIPPSE_CMD_RETURN && base->focus->view->line_select)) {
    editor_open_selection(base, base->focus, base->document);
  } else if (command==TIPPSE_CMD_SPLIT) {
    editor_split(base, base->document);
  } else if (command==TIPPSE_CMD_UNSPLIT) {
    struct splitter* document = editor_unsplit(base, base->document);
    if (base->focus==base->document) {
      editor_focus(base, document, 0);
    }
  } else if (command==TIPPSE_CMD_SAVE) {
    editor_save_document(base, base->document->file);
  } else if (command==TIPPSE_CMD_SAVEALL) {
    editor_save_documents(base);
  } else if (command==TIPPSE_CMD_COMPILE) {
    if (base->document->file->pipefd[1]==-1 && base->document->file==base->compiler_doc) {
      document_file_pipe(base->document->file, "find / -iname \"*.*\"");
    } else {
      splitter_assign_document_file(base->document, base->compiler_doc);
    }
  } else {
    (*base->focus->document->keypress)(base->focus->document, base->focus, command, key, cp, button, button_old, x-base->focus->x, y-base->focus->y);
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

  if (node!=base->panel) {
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
void editor_open_selection(struct editor* base, struct splitter* node, struct splitter* destination) {
  if (node->view->selection_low!=node->view->selection_high) {
    struct list_node* views = destination->file->views->first;
    while (views) {
      struct document_view* view = *(struct document_view**)list_object(views);
      if (view==destination->view) {
        list_remove(destination->file->views, views);
        break;
      }

      views = views->next;
    }

    char* name = (char*)range_tree_raw(node->file->buffer, node->view->selection_low, node->view->selection_high);
    if (*name) {
      editor_open_document(base, name, node, destination);
    }

    free(name);
  }
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
        document_file_name(destination->file, relative);
        document_directory(base->browser_doc);
        document_file_manualchange(base->browser_doc);
        document_view_reset(destination->view, base->browser_doc);
        destination->view->line_select = 1;
      }
    } else {
      new_document_doc = document_file_create(1, 1);
      document_file_load(new_document_doc, relative);
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

// Save single modified document
void editor_save_document(struct editor* base, struct document_file* file) {
  if (document_undo_modified(file) && file->save) {
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
      assign = document_file_create(1, 1);
      document_file_name(assign, "Untitled");
      list_insert(base->documents, NULL, &assign);
    }

    struct document_file* replace = *(struct document_file**)list_object(remove);
    splitter_exchange_document_file(base->splitters, replace, assign);
    document_file_destroy(replace);
    list_remove(base->documents, remove);
  }
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
