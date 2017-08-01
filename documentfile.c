/* Tippse - Document - File operations and file setup */

#include "documentfile.h"

// TODO: this has to be covered by the settings subsystem in future
struct document_file_type document_file_types[] = {
  {"c", file_type_c_create},
  {"h", file_type_c_create},
  {"cpp", file_type_cpp_create},
  {"hpp", file_type_cpp_create},
  {"cc", file_type_cpp_create},
  {"cxx", file_type_cpp_create},
  {"sql", file_type_sql_create},
  {"lua", file_type_lua_create},
  {"php", file_type_php_create},
  {"xml", file_type_xml_create},
  {NULL,  NULL}
};

struct document_file* document_file_create(int save) {
  struct document_file* file = (struct document_file*)malloc(sizeof(struct document_file));
  file->buffer = NULL;
  file->undos = list_create();
  file->redos = list_create();
  file->filename = strdup("");
  file->views = list_create();
  file->modified = 0;
  file->save = save;
  file->type = file_type_text_create();
  file->encoding = encoding_utf8_create();
  file->tabstop = TIPPSE_TABSTOP_AUTO;
  file->tabstop_width = 4;
  file->newline = TIPPSE_NEWLINE_AUTO;
  return file;
}

void document_file_clear(struct document_file* file) {
  if (file->buffer) {
    range_tree_destroy(file->buffer);
    file->buffer = NULL;
  }
}

void document_file_destroy(struct document_file* file) {
  document_file_clear(file);
  document_undo_empty(file->undos);
  list_destroy(file->undos);
  document_undo_empty(file->redos);
  list_destroy(file->redos);
  list_destroy(file->views);
  free(file->filename);
  (*file->type->destroy)(file->type);
  (*file->encoding->destroy)(file->encoding);
  free(file);
}

// Set up file name and select file type depending on the suffix
void document_file_name(struct document_file* file, const char* filename) {
  if (filename!=file->filename) {
    free(file->filename);
    file->filename = strdup(filename);
  }

  const char* search = filename;
  const char* last = filename;
  while (*search) {
    if (*search=='.') {
      last = search+1;
    }

    search++;
  }

  if (last==filename) {
    last = search;
  }

  size_t n;
  for (n = 0; document_file_types[n].extension; n++) {
    if (strcasecmp(document_file_types[n].extension, last)==0) {
      (*file->type->destroy)(file->type);
      file->type = (*document_file_types[n].constructor)();
      break;
    }
  }
}

// Load file into memory if lower than a certain threshold, otherwise just use file offset references
void document_file_load(struct document_file* file, const char* filename) {
  document_file_clear(file);
  int f = open(filename, O_RDONLY);
  if (f!=-1) {
    struct file_cache* cache = file_cache_create(filename);

    file_offset_t length = (file_offset_t)lseek(f, 0, SEEK_END);
    lseek(f, 0, SEEK_SET);
    file_offset_t offset = 0;
    while (1) {
      file_offset_t block = 0;
      struct fragment* buffer = NULL;
      if (offset<TIPPSE_DOCUMENT_MEMORY_LOADMAX) {
        uint8_t* copy = (uint8_t*)malloc(TREE_BLOCK_LENGTH_MAX);
        block = (file_offset_t)read(f, copy, TREE_BLOCK_LENGTH_MAX);
        buffer = fragment_create_memory(copy, (size_t)block);
      } else {
        block = length-offset;
        if (block>TREE_BLOCK_LENGTH_MAX) {
          block = TREE_BLOCK_LENGTH_MAX;
        }

        buffer = fragment_create_file(cache, offset, (size_t)block);
      }

      if (block==0) {
        fragment_dereference(buffer);
        break;
      }

      file->buffer = range_tree_insert(file->buffer, file->type, offset, buffer, 0, buffer->length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER);
      offset += block;
    }

    file_cache_dereference(cache);
    close(f);
  }

  document_file_name(file, filename);
  file->modified = 0;

  document_file_detect_properties(file);
}

// Save file directly to file
int document_file_save_plain(struct document_file* file, const char* filename) {
  int f = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644);
  if (f==-1) {
    return 0;
  }

  if (file->buffer) {
    struct range_tree_node* buffer = range_tree_first(file->buffer);
    while (buffer) {
      fragment_cache(buffer->buffer);
      write(f, buffer->buffer->buffer+buffer->offset, buffer->length);
      buffer = range_tree_next(buffer);
    }
  }

  close(f);
  return 1;
}

// Check file type and save via temporary file if needed
void document_file_save(struct document_file* file, const char* filename) {
  if (file->buffer && (file->buffer->inserter&TIPPSE_INSERTER_FILE)) {
    char* tmpname = combine_string(filename, ".save.tmp");
    if (document_file_save_plain(file, tmpname)) {
      if (rename(tmpname, filename)==0) {
        document_file_load(file, filename);
      }
    }

    free(tmpname);
  } else {
    if (document_file_save_plain(file, filename)) {
      file->modified = 0;
    }
  }

  //document->keep_status = 1;
  //splitter_status(splitter, "Saved!", 1);
  //splitter_name(splitter, filename);
}

void document_file_detect_properties(struct document_file* file) {
  if (!file->buffer) {
    return;
  }

  struct encoding_stream stream;
  encoding_stream_from_page(&stream, range_tree_first(file->buffer), 0);
  file_offset_t offset = 0;

  int newline_cr = 0;
  int newline_lf = 0;
  int newline_crlf = 0;
  int tabstop_space = 0;
  int tabstop_tab = 0;
  int tabstop = TIPPSE_TABSTOP_AUTO;
  int tabstop_width = 8;
  int last = 0;
  int start = 1;
  int spaces = 0;
  while (offset<TIPPSE_DOCUMENT_MEMORY_LOADMAX) {
    size_t length = 1;
    int cp = (*file->encoding->decode)(file->encoding, &stream, &length);
    encoding_stream_forward(&stream, length);

    if (last==0 && cp==0) {
      break;
    }

    if (cp=='\t' && start) {
      if (tabstop==TIPPSE_TABSTOP_AUTO || tabstop==TIPPSE_TABSTOP_TAB) {
        tabstop = TIPPSE_TABSTOP_TAB;
      } else {
        tabstop = TIPPSE_TABSTOP_MAX;
      }
    } else if (cp==' ' && start) {
      if (tabstop==TIPPSE_TABSTOP_AUTO || tabstop==TIPPSE_TABSTOP_SPACE) {
        tabstop = TIPPSE_TABSTOP_SPACE;
      } else {
        tabstop = TIPPSE_TABSTOP_MAX;
      }

      spaces++;
    } else if (cp=='\r' || cp=='\n') {
      if (tabstop==TIPPSE_TABSTOP_TAB) {
        tabstop_tab++;
      } else if (tabstop==TIPPSE_TABSTOP_SPACE) {
        tabstop_space++;

        if (spaces!=0 && spaces<tabstop_width) {
          tabstop_width = spaces;
        }
      }

      start = 1;
      spaces = 0;
      tabstop = TIPPSE_TABSTOP_AUTO;
    } else {
      start = 0;
    }

    if (last=='\r') {
      if (cp=='\n') {
        newline_crlf++;
      } else {
        newline_cr++;
      }
    } else if (last!='\r' && cp=='\n') {
      newline_lf++;
    }

    last = cp;
    offset += length;
  }

  // Try to guess the style by using the most used type (base on some arbitrary factors) ;)
  if (newline_crlf>(newline_cr+newline_lf)*3) {
    file->newline = TIPPSE_NEWLINE_CRLF;
  } else if (newline_cr>(newline_crlf+newline_lf)*3) {
    file->newline = TIPPSE_NEWLINE_CR;
  } else if (newline_lf>(newline_crlf+newline_cr)*3) {
    file->newline = TIPPSE_NEWLINE_LF;
  } else {
    file->newline = TIPPSE_NEWLINE_AUTO;
  }

  // Try to guess the tab style
  if (tabstop_tab>tabstop_space*3) {
    file->tabstop = TIPPSE_TABSTOP_TAB;
  } else if (tabstop_space>tabstop_tab*3) {
    file->tabstop = TIPPSE_TABSTOP_SPACE;
    file->tabstop_width = tabstop_width;
  } else {
    file->tabstop = TIPPSE_TABSTOP_AUTO;
  }
}

void document_file_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length) {
  if (*pos>=offset && *pos!=~0) {
    *pos+=length;
  }
}

void document_file_insert(struct document_file* file, file_offset_t offset, const uint8_t* text, size_t length) {
  if (file->buffer && offset>file->buffer->length) {
    return;
  }

  file_offset_t old_length = file->buffer?file->buffer->length:0;
  file->buffer = range_tree_insert_split(file->buffer, file->type, offset, text, length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, NULL);
  length = (file->buffer?file->buffer->length:0)-old_length;
  if (length<=0) {
    return;
  }

  document_undo_add(file, NULL, offset, length, TIPPSE_UNDO_TYPE_INSERT);

  struct list_node* views = file->views->first;
  while (views) {
    struct document_view* view = (struct document_view*)views->object;
    document_file_expand(&view->selection_end, offset, length);
    document_file_expand(&view->selection_start, offset, length);
    document_file_expand(&view->selection_low, offset, length);
    document_file_expand(&view->selection_high, offset, length);
    document_file_expand(&view->offset, offset, length);

    views = views->next;
  }

  document_undo_empty(file->redos);
  file->modified = 1;
}

void document_file_insert_buffer(struct document_file* file, file_offset_t offset, struct range_tree_node* buffer) {
  if (!buffer) {
    return;
  }

  file_offset_t length = buffer->length;

  if (file->buffer && offset>file->buffer->length) {
    return;
  }

  file->buffer = range_tree_paste(file->buffer, file->type, buffer, offset);
  document_undo_add(file, NULL, offset, length, TIPPSE_UNDO_TYPE_INSERT);

  struct list_node* views = file->views->first;
  while (views) {
    struct document_view* view = (struct document_view*)views->object;
    document_file_expand(&view->selection_end, offset, length);
    document_file_expand(&view->selection_start, offset, length);
    document_file_expand(&view->selection_low, offset, length);
    document_file_expand(&view->selection_high, offset, length);
    document_file_expand(&view->offset, offset, length);

    views = views->next;
  }

  document_undo_empty(file->redos);
  file->modified = 1;
}

void document_file_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length) {
  if (*pos>=offset && *pos!=~0) {
    if ((*pos-offset)>=length) {
      *pos-=length;
    } else {
      *pos = offset;
    }
  }
}

void document_file_delete(struct document_file* file, file_offset_t offset, file_offset_t length) {
  if (!file->buffer || offset>=file->buffer->length) {
    return;
  }

  document_undo_add(file, NULL, offset, length, TIPPSE_UNDO_TYPE_DELETE);

  file_offset_t old_length = file->buffer?file->buffer->length:0;
  file->buffer = range_tree_delete(file->buffer, file->type, offset, length, 0);
  length = old_length-(file->buffer?file->buffer->length:0);

  struct list_node* views = file->views->first;
  while (views) {
    struct document_view* view = (struct document_view*)views->object;
    document_file_reduce(&view->selection_end, offset, length);
    document_file_reduce(&view->selection_start, offset, length);
    document_file_reduce(&view->selection_low, offset, length);
    document_file_reduce(&view->selection_high, offset, length);
    document_file_reduce(&view->offset, offset, length);

    views = views->next;
  }

  document_undo_empty(file->redos);
  file->modified = 1;
}

int document_file_delete_selection(struct document_file* file, struct document_view* view) {
  if (!file->buffer) {
    return 0;
  }

  file_offset_t low = view->selection_low;
  if (low>file->buffer->length) {
    low = file->buffer->length;
  }

  file_offset_t high = view->selection_high;
  if (high>file->buffer->length) {
    high = file->buffer->length;
  }

  if (high-low==0) {
    return 0;
  }

  document_file_delete(file, low, high-low);
  view->offset = low;
  view->selection_low = ~0;
  view->selection_high = ~0;
  return 1;
}
