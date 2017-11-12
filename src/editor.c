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

const char* editor_commands[TIPPSE_CMD_MAX] = {
  "",
  "quit",
  "up",
  "down",
  "right",
  "left",
  "pageup",
  "pagedown",
  "first",
  "last",
  "home",
  "end",
  "backspace",
  "delete",
  "insert",
  "search",
  "searchnext",
  "undo",
  "redo",
  "cut",
  "copy",
  "paste",
  "tab",
  "untab",
  "save",
  "mouse",
  "searchprevious",
  "open",
  "split",
  "invisibles",
  "browser",
  "switch",
  "bookmark",
  "wordwrap",
  "documents",
  "return",
  "selectall",
  "selectup",
  "selectdown",
  "selectright",
  "selectleft",
  "selectpageup",
  "selectpagedown",
  "selectfirst",
  "selectlast",
  "selecthome",
  "selectend",
  "close",
  "saveall",
  "unsplit"
};

// Create editor
struct editor* editor_create(const char* base_path, struct screen* screen, int argc, const char** argv) {
  struct editor* base = (struct editor*)malloc(sizeof(struct editor));
  base->close = 0;
  base->base_path = base_path;
  base->screen = screen;

  base->commands = trie_create();
  for (size_t n = 0; n<TIPPSE_CMD_MAX; n++) {
    struct trie_node* parent = NULL;
    const char* command = editor_commands[n];
    while (*command) {
      parent = trie_append_codepoint(base->commands, parent, *command, 0);
      command++;
    }

    if (parent) {
      parent->type = (intptr_t)n;
    }
  }

  base->documents = list_create();

  base->tabs_doc = document_file_create(0);
  document_file_name(base->tabs_doc, "Open");
  base->tabs_doc->defaults.wrapping = 0;

  base->browser_doc = document_file_create(0);
  document_file_name(base->browser_doc, base->base_path);
  base->browser_doc->defaults.wrapping = 0;

  base->document_doc = document_file_create(1);
  document_file_name(base->document_doc, "Untitled");
  base->document = splitter_create(0, 0, NULL, NULL,  "");
  splitter_assign_document_file(base->document, base->document_doc);
  document_view_reset(base->document->view, base->document_doc);

  base->search_doc = document_file_create(0);
  document_file_name(base->search_doc, "Search");
  base->search_doc->binary = 0;

  base->panel = splitter_create(0, 0, NULL, NULL, "");
  splitter_assign_document_file(base->panel, base->search_doc);

  base->splitters = splitter_create(TIPPSE_SPLITTER_VERT|TIPPSE_SPLITTER_FIXED0, 5, base->panel, base->document, "");
  list_insert(base->documents, NULL, base->document_doc);
  list_insert(base->documents, NULL, base->tabs_doc);
  list_insert(base->documents, NULL, base->search_doc);
  list_insert(base->documents, NULL, base->browser_doc);

  base->panel->view->line_select = 0; // TODO: check why the panel form needs this

  for (int n = argc-1; n>=1; n--) {
    if (n==1) {
      document_file_load(base->document_doc, argv[n]);
      splitter_assign_document_file(base->document, base->document_doc);
    } else {
      struct document_file* document_add = document_file_create(1);
      document_file_load(document_add, argv[n]);
      list_insert(base->documents, NULL, document_add);
    }
  }

  base->focus = base->document;
  base->last_document = base->document;
  base->focus->active = 1;

  document_directory(base->browser_doc);
  document_file_manualchange(base->browser_doc);

  return base;
}

// Destroy editor
void editor_destroy(struct editor* base) {
  splitter_destroy(base->splitters);

  while (base->documents->first) {
    document_file_destroy((struct document_file*)base->documents->first->object);
    list_remove(base->documents, base->documents->first);
  }

  list_destroy(base->documents);
  trie_destroy(base->commands);

  free(base);
}

// Refresh editor components
void editor_draw(struct editor* base) {
  base->tabs_doc->buffer = range_tree_delete(base->tabs_doc->buffer, 0, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, TIPPSE_INSERTER_AUTO);
  struct list_node* doc = base->documents->first;
  while (doc) {
    struct document_file* file = (struct document_file*)doc->object;
    if (file!=base->browser_doc && file!=base->tabs_doc && file!=base->search_doc) {
      if (base->tabs_doc->buffer) {
        base->tabs_doc->buffer = range_tree_insert_split(base->tabs_doc->buffer, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, (uint8_t*)"\n", 1, TIPPSE_INSERTER_ESCAPE|TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      }

      base->tabs_doc->buffer = range_tree_insert_split(base->tabs_doc->buffer, base->tabs_doc->buffer?base->tabs_doc->buffer->length:0, (uint8_t*)file->filename, strlen(file->filename), TIPPSE_INSERTER_ESCAPE|TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
    }

    doc = doc->next;
  }

  document_file_manualchange(base->tabs_doc);

  if (base->focus==base->panel) {
    int height = (base->panel->file->buffer?base->panel->file->buffer->visuals.ys:0)+1;
    int max = (base->screen->height/4)+1;
    if (height>max) {
      height = max;
    }

    base->splitters->split = height;
  } else {
    base->splitters->split = 0;
  }

  screen_check(base->screen);
  splitter_draw_multiple(base->screen, base->splitters, 0);

  int foreground = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_STATUS];
  int background = base->focus->file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND];
  for (int x = 0; x<base->screen->width; x++) {
    int cp = 0x20;
    screen_setchar(base->screen, x, 0, 0, 0, base->screen->width, base->screen->height, &cp, 1, foreground, background);
  }

  screen_drawtext(base->screen, 0, 0, 0, 0, base->screen->width, base->screen->height, base->focus->name, (size_t)base->screen->width, foreground, background);
  struct encoding_stream stream;
  encoding_stream_from_plain(&stream, (uint8_t*)base->focus->status, SIZE_T_MAX);
  size_t length = encoding_utf8_strlen(NULL, &stream);
  screen_drawtext(base->screen, base->screen->width-(int)length, 0, 0, 0, base->screen->width, base->screen->height, base->focus->status, length, foreground, background);

  screen_draw(base->screen);
}

// Regular timer event (schedule processes)
void editor_tick(struct editor* base) {
  struct list_node* doc = base->documents->first;
  while (doc) {
    struct document_file* file = (struct document_file*)doc->object;
    document_undo_chain(file, file->undos);
    doc = doc->next;
  }

  splitter_draw_multiple(base->screen, base->splitters, 1);
}

// An input event was signalled ... translate it to a command if possible
void editor_keypress(struct editor* base, int key, int cp, int button, int button_old, int x, int y) {
  // Woops... we have a reencode here ... try to remove me (beside the performance loss the codepoints aren't recovered correctly)
  uint8_t utf8[8];
  size_t size = encoding_utf8_encode(NULL, cp, &utf8[0], 8);
  utf8[size] = 0;

  // But our lookup has to be an integer array before
  char key_lookup[1024];
  const char* key_name = editor_key_names[key&TIPPSE_KEY_MASK];
  sprintf(&key_lookup[0], "/keys/%s%s%s%s", (key&TIPPSE_KEY_MOD_SHIFT)?"shift+":"", (key&TIPPSE_KEY_MOD_CTRL)?"ctrl+":"", (key&TIPPSE_KEY_MOD_ALT)?"alt+":"", (cp!=0)?(const char*)&utf8[0]:key_name);

  int command = TIPPSE_KEY_CHARACTER;
  int* codepoints = config_find_ascii(base->focus->file->config, &key_lookup[0]);
  if (codepoints) {
    struct trie_node* parent = NULL;
    while (*codepoints) {
      parent = trie_find_codepoint(base->commands, parent, *codepoints);
      if (!parent) {
        break;
      }

      codepoints++;
    }

    if (parent && parent->type!=0) {
      command = (int)parent->type;
    }
  }

  if (command!=TIPPSE_KEY_CHARACTER || cp>=0x20) {
    editor_intercept(base, command, key, cp, button, button_old, x, y);
  }
}

// After event translation we can intercept the core commands (for now)
void editor_intercept(struct editor* base, int command, int key, int cp, int button, int button_old, int x, int y) {
  if (command==TIPPSE_CMD_MOUSE) {
    struct splitter* select = splitter_by_coordinate(base->splitters, x, y);
    if (select && button!=0 && button_old==0) {
      base->focus->active = 0;
      base->focus = select;
      base->focus->active = 1;
      if (select->file->save) {
        base->document = select;
      }

      if (select!=base->panel) {
        base->last_document = select;
      }
    }
  }

  if (command==TIPPSE_CMD_DOCUMENTSELECTION) {
    splitter_assign_document_file(base->document, base->tabs_doc);
    base->document->view->line_select = 1;
  } else if (command==TIPPSE_CMD_BROWSER) {
    splitter_assign_document_file(base->document, base->browser_doc);
    base->document->view->line_select = 1;
  }

  if (command==TIPPSE_CMD_QUIT) {
    base->close = 1;
  } else if (command==TIPPSE_CMD_SHOW_INVISIBLES) {
    // TODO: handle this in the document itself?
    base->focus->view->show_invisibles ^= 1;
    (*base->focus->document->reset)(base->focus->document, base->focus);
  } else if (command==TIPPSE_CMD_WORDWRAP) {
    // TODO: handle this in the document itself?
    base->focus->view->wrapping ^= 1;
    (*base->focus->document->reset)(base->focus->document, base->focus);
  } else if (command==TIPPSE_CMD_SEARCH) {
    base->focus->active = 0;
    if (base->focus==base->document) {
      base->focus = base->panel;
    } else {
      base->focus = base->document;
    }

    base->focus->active = 1;
  } else if (command==TIPPSE_CMD_SEARCH_NEXT) {
    base->focus->active = 0;
    base->focus = base->document;
    base->focus->active = 1;
    if (base->search_doc->buffer) {
      document_search(base->last_document, range_tree_first(base->search_doc->buffer), base->search_doc->buffer->length, 1);
    }
  } else if (command==TIPPSE_CMD_SEARCH_PREV) {
    base->focus->active = 0;
    base->focus = base->document;
    base->focus->active = 1;
    if (base->search_doc->buffer) {
      document_search(base->last_document, range_tree_first(base->search_doc->buffer), base->search_doc->buffer->length, 0);
    }
  } else if (command==TIPPSE_CMD_VIEW_SWITCH) {
    if (base->focus->document==base->focus->document_hex) {
      base->focus->document = base->focus->document_text;
    } else {
      base->focus->document = base->focus->document_hex;
    }
  } else if (command==TIPPSE_CMD_CLOSE) {
    editor_close_document(base, base->focus->file);
  } else if (command==TIPPSE_CMD_OPEN || (base->focus->view->line_select && command==TIPPSE_CMD_RETURN)) {
    if (base->focus->view->selection_low!=base->focus->view->selection_high) {
      struct list_node* views = base->document->file->views->first;
      while (views) {
        struct document_view* view = (struct document_view*)views->object;
        if (view==base->document->view) {
          list_remove(base->document->file->views, views);
          break;
        }

        views = views->next;
      }

      char* name = (char*)range_tree_raw(base->focus->file->buffer, base->focus->view->selection_low, base->focus->view->selection_high);
      if (*name) {
        char* path_only = (base->focus->file==base->browser_doc)?strdup(base->focus->file->filename):strip_file_name(base->focus->file->filename);
        char* combined = combine_path_file(path_only, name);
        char* corrected = correct_path(combined);
        char* relative = relativate_path(base->base_path, corrected);

        struct document_file* new_document_doc = NULL;
        struct list_node* docs = base->documents->first;
        while (docs) {
          struct document_file* docs_document_doc = (struct document_file*)docs->object;
          if (strcmp(docs_document_doc->filename, relative)==0 && docs_document_doc!=base->focus->file) {
            new_document_doc = docs_document_doc;
            list_remove(base->documents, docs);
            break;
          }

          docs = docs->next;
        }

        if (!new_document_doc) {
          if (is_directory(relative)) {
            document_file_name(base->document->file, relative);
            document_directory(base->browser_doc);
            document_view_reset(base->document->view, base->browser_doc);
            base->document->view->line_select = 1;
          } else {
            new_document_doc = document_file_create(1);
            document_file_load(new_document_doc, relative);
          }
        }

        if (new_document_doc) {
          list_insert(base->documents, NULL, new_document_doc);
          splitter_assign_document_file(base->document, new_document_doc);
        }

        free(relative);
        free(combined);
        free(path_only);
        free(corrected);
      }

      free(name);
    }
  } else if (command==TIPPSE_CMD_SPLIT) {
    editor_split(base, base->document);
  } else if (command==TIPPSE_CMD_UNSPLIT) {
    struct splitter* document = editor_unsplit(base, base->document);
    if (base->focus==base->document) {
      base->focus->active = 0;
      base->focus = document;
      base->focus->active = 1;
    }

    base->document = document;
  } else if (command==TIPPSE_CMD_SAVE) {
    editor_save_document(base, base->document->file);
  } else if (command==TIPPSE_CMD_SAVEALL) {
    editor_save_documents(base);
  } else {
    (*base->focus->document->keypress)(base->focus->document, base->focus, command, key, cp, button, button_old, x-base->focus->x, y-base->focus->y);
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
    editor_save_document(base, (struct document_file*)docs->object);
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
    if ((struct document_file*)docs->object==file) {
      remove = docs;
    } else {
      if (((struct document_file*)docs->object)->save) {
        assign = (struct document_file*)docs->object;
      }
    }

    docs = docs->next;
  }

  if (remove) {
    if (!assign) {
      assign = document_file_create(1);
      document_file_name(assign, "Untitled");
      list_insert(base->documents, NULL, assign);
    }

    struct document_file* replace = (struct document_file*)remove->object;
    splitter_exchange_document_file(base->splitters, replace, assign);
    document_file_destroy(replace);
    list_remove(base->documents, remove);
  }
}
