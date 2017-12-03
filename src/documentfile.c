// Tippse - Document - File operations and file setup

#include "documentfile.h"

extern struct config_cache screen_color_codes[];
extern struct config_cache visual_color_codes[VISUAL_FLAG_COLOR_MAX+1];

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
  {"diff", file_type_patch_create},
  {"patch", file_type_patch_create},
  {"js", file_type_js_create},
  {NULL,  NULL}
};

// Create file operations
struct document_file* document_file_create(int save, int config) {
  struct document_file* base = (struct document_file*)malloc(sizeof(struct document_file));
  base->buffer = NULL;
  base->bookmarks = NULL;
  base->cache = NULL;
  base->caches = list_create(sizeof(struct document_file_cache));
  base->binary = 0;
  base->undos = list_create(sizeof(struct document_undo));
  base->redos = list_create(sizeof(struct document_undo));
  base->filename = strdup("");
  base->views = list_create(sizeof(struct document_view*));
  base->save = save;
  base->line_select = 0;
  base->encoding = encoding_utf8_create();
  base->tabstop = TIPPSE_TABSTOP_AUTO;
  base->tabstop_width = 4;
  base->newline = TIPPSE_NEWLINE_AUTO;
  base->config = config?config_create():NULL;
  base->type = file_type_text_create(base->config);
  base->view = document_view_create();
  base->pipefd[0] = -1;
  base->pipefd[1] = -1;
  document_file_reset_views(base);
  document_undo_mark_save_point(base);
  document_file_reload_config(base);
  return base;
}

// Clear file operations
void document_file_clear(struct document_file* base) {
  if (base->cache) {
    document_file_dereference_cache(base, base->cache);
    file_cache_dereference(base->cache);
    base->cache = NULL;
  }

  if (base->buffer) {
    range_tree_destroy(base->buffer, base);
    base->buffer = NULL;
  }

  if (base->bookmarks) {
    range_tree_destroy(base->bookmarks, NULL);
    base->bookmarks = NULL;
  }

  if (base->config) {
    config_clear(base->config);
  }
}

// Destroy file operations
void document_file_destroy(struct document_file* base) {
  document_file_clear(base);
  document_file_close_pipe(base);
  document_undo_empty(base, base->undos);
  document_undo_empty(base, base->redos);
  list_destroy(base->undos);
  list_destroy(base->redos);
  list_destroy(base->views);
  list_destroy(base->caches);
  free(base->filename);
  (*base->type->destroy)(base->type);
  (*base->encoding->destroy)(base->encoding);
  document_view_destroy(base->view);
  if (base->config) {
    config_destroy(base->config);
  }

  free(base);
}

// Set up file name and select file type depending on the suffix
void document_file_name(struct document_file* base, const char* filename) {
  if (filename!=base->filename) {
    free(base->filename);
    base->filename = strdup(filename);
  }

  document_file_reload_config(base);

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

  for (size_t n = 0; document_file_types[n].extension; n++) {
    if (strcasecmp(document_file_types[n].extension, last)==0) {
      (*base->type->destroy)(base->type);
      base->type = (*document_file_types[n].constructor)(base->config);
      break;
    }
  }
}

// Change encoding
void document_file_encoding(struct document_file* base, struct encoding* encoding) {
  (*base->encoding->destroy)(base->encoding);
  base->encoding = encoding;
}

// Execute system command and push output into file contents
void document_file_pipe(struct document_file* base, const char* command) {
  range_tree_destroy(base->buffer, base);
  base->buffer = NULL;
  document_undo_empty(base, base->undos);
  document_undo_empty(base, base->redos);
  document_file_reset_views(base);

  document_file_close_pipe(base);

  pipe(base->pipefd);

  signal(SIGCHLD, SIG_IGN);
  base->pid = fork();
  if (base->pid==0) {
    dup2(base->pipefd[0], 0);
    dup2(base->pipefd[1], 1);
    dup2(base->pipefd[1], 2);
    close(base->pipefd[0]);
    close(base->pipefd[1]);
    char* argv[4];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = (char*)command;
    argv[3] = NULL;
    execv(argv[0], &argv[0]);
    exit(0);
  } else {
    close(base->pipefd[1]);

    int flags = fcntl(base->pipefd[0], F_GETFL, 0);
    fcntl(base->pipefd[0], F_SETFL, flags|O_NONBLOCK);

    document_undo_mark_save_point(base);
    document_file_name(base, command);
    document_file_detect_properties(base);
    base->bookmarks = range_tree_static(base->bookmarks, base->buffer?base->buffer->length:0, 0);
    document_file_reset_views(base);
  }
}

// Append incoming data from pipe
void document_file_fill_pipe(struct document_file* base, uint8_t* buffer, size_t length) {
  if (length>0) {
    uint8_t* copy = (uint8_t*)malloc(length);
    memcpy(copy, buffer, length);
    file_offset_t offset = base->buffer?base->buffer->length:0;
    struct fragment* fragment = fragment_create_memory(copy, length);
    base->buffer = range_tree_insert(base->buffer, offset, fragment, 0, length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, base);
    fragment_dereference(fragment, base);

    document_file_expand_all(base, offset, length);
  }
}

// Close incoming pipe
void document_file_close_pipe(struct document_file* base) {
  if (base->pipefd[0]==-1) {
    return;
  }

  close(base->pipefd[0]);

  base->pipefd[0] = -1;
  base->pipefd[1] = -1;
}

// Load file from file system, up to a certain threshold
void document_file_load(struct document_file* base, const char* filename) {
  document_file_clear(base);
  int f = open(filename, O_RDONLY);
  if (f!=-1) {
    base->cache = file_cache_create(filename);
    document_file_reference_cache(base, base->cache);

    file_offset_t length = (file_offset_t)lseek(f, 0, SEEK_END);
    lseek(f, 0, SEEK_SET);
    file_offset_t offset = 0;
    while (1) {
      file_offset_t block = 0;
      struct fragment* fragment = NULL;
      if (length<TIPPSE_DOCUMENT_MEMORY_LOADMAX) {
        uint8_t* copy = (uint8_t*)malloc(TREE_BLOCK_LENGTH_MAX);
        ssize_t in = read(f, copy, TREE_BLOCK_LENGTH_MAX);
        block = (file_offset_t)((in>=0)?in:0);
        fragment = fragment_create_memory(copy, (size_t)block);
      } else {
        block = length-offset;
        if (block>TREE_BLOCK_LENGTH_MAX) {
          block = TREE_BLOCK_LENGTH_MAX;
        }

        fragment = fragment_create_file(base->cache, offset, (size_t)block, base);
      }

      if (block==0) {
        fragment_dereference(fragment, base);
        break;
      }

      base->buffer = range_tree_insert(base->buffer, offset, fragment, 0, fragment->length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, base);
      fragment_dereference(fragment, base);
      offset += block;
    }

    close(f);
  }

  document_undo_mark_save_point(base);
  document_file_name(base, filename);
  document_file_detect_properties(base);
  base->bookmarks = range_tree_static(base->bookmarks, base->buffer?base->buffer->length:0, 0);
  document_file_reset_views(base);
}

// Load file from memory
void document_file_load_memory(struct document_file* base, const uint8_t* buffer, size_t length) {
  document_file_clear(base);
  file_offset_t offset = 0;
  while (length>0) {
    size_t max = (length>TREE_BLOCK_LENGTH_MAX)?TREE_BLOCK_LENGTH_MAX:length;
    uint8_t* copy = (uint8_t*)malloc(max);
    memcpy(copy, buffer, max);
    struct fragment* fragment = fragment_create_memory(copy, max);
    base->buffer = range_tree_insert(base->buffer, offset, fragment, 0, max, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, base);
    fragment_dereference(fragment, base);
    offset += max;
    length -= max;
    buffer += max;
  }

  document_undo_mark_save_point(base);
  document_file_name(base, "<memory>");
  document_file_detect_properties(base);
  base->bookmarks = range_tree_static(base->bookmarks, base->buffer?base->buffer->length:0, 0);
  document_file_reset_views(base);
}

// Save file directly to file system
int document_file_save_plain(struct document_file* base, const char* filename) {
  int f = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644);
  if (f==-1) {
    return 0;
  }

  if (base->buffer) {
    struct range_tree_node* buffer = range_tree_first(base->buffer);
    while (buffer) {
      fragment_cache(buffer->buffer);
      write(f, buffer->buffer->buffer+buffer->offset, buffer->length);
      buffer = range_tree_next(buffer);
    }
  }

  close(f);
  document_undo_mark_save_point(base);
  return 1;
}

// Save file, uses a temporary file if necessary
void document_file_save(struct document_file* base, const char* filename) {
  if (base->buffer && (base->buffer->inserter&TIPPSE_INSERTER_FILE)) {
    char* tmpname = combine_string(filename, ".save.tmp");
    if (document_file_save_plain(base, tmpname)) {
      if (rename(tmpname, filename)==0) {
        document_file_load(base, filename);
      }
    }

    free(tmpname);
  } else {
    if (document_file_save_plain(base, filename)) {
      document_undo_mark_save_point(base);
    }

    if (base->cache) {
      document_file_dereference_cache(base, base->cache);
      file_cache_dereference(base->cache);
      base->cache = file_cache_create(filename);
      document_file_reference_cache(base, base->cache);
    }
  }

  //document->keep_status = 1;
  //splitter_status(splitter, "Saved!", 1);
  //splitter_name(splitter, filename);
}

// Detect file properties
void document_file_detect_properties(struct document_file* base) {
  if (!base->buffer) {
    return;
  }

  struct encoding_stream stream;
  encoding_stream_from_page(&stream, range_tree_first(base->buffer), 0);

  // Binary detection ... TODO: Recheck when UTF-16 as encoding is available
  file_offset_t offset = 0;

  int zeros = 0;
  while (offset<TIPPSE_DOCUMENT_MEMORY_LOADMAX && offset<base->buffer->length) {
    uint8_t byte = encoding_stream_peek(&stream, 0);
    encoding_stream_forward(&stream, 1);
    if (byte==0x00) {
      zeros++;
    }

    offset++;
  }

  if (zeros>=(int)(offset/512+1)) {
    base->binary = 1;
    document_file_encoding(base, encoding_ascii_create());
  } else {
    base->binary = 0;
  }

  encoding_stream_from_page(&stream, range_tree_first(base->buffer), 0);
  offset = 0;
  int newline_cr = 0;
  int newline_lf = 0;
  int newline_crlf = 0;
  int tabstop_space = 0;
  int tabstop_tab = 0;
  int tabstop = TIPPSE_TABSTOP_AUTO;
  int tabstop_width = 8;
  codepoint_t last = 0;
  int start = 1;
  int spaces = 0;
  while (offset<TIPPSE_DOCUMENT_MEMORY_LOADMAX && offset<base->buffer->length) {
    size_t length = 1;
    codepoint_t cp = (*base->encoding->decode)(base->encoding, &stream, &length);
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
    base->newline = TIPPSE_NEWLINE_CRLF;
  } else if (newline_cr>(newline_crlf+newline_lf)*3) {
    base->newline = TIPPSE_NEWLINE_CR;
  } else if (newline_lf>(newline_crlf+newline_cr)*3) {
    base->newline = TIPPSE_NEWLINE_LF;
  } else {
    base->newline = TIPPSE_NEWLINE_AUTO;
  }

  // Try to guess the tab style
  if (tabstop_tab>tabstop_space*3) {
    base->tabstop = TIPPSE_TABSTOP_TAB;
  } else if (tabstop_space>tabstop_tab*3) {
    base->tabstop = TIPPSE_TABSTOP_SPACE;
    base->tabstop_width = tabstop_width;
  } else {
    base->tabstop = TIPPSE_TABSTOP_AUTO;
  }
}

// Correct file offset by expansion offset and length
void document_file_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length) {
  if (*pos>=offset && *pos!=FILE_OFFSET_T_MAX) {
    *pos+=length;
  }
}

// Correct all file offsets by expansion offset and length
void document_file_expand_all(struct document_file* base, file_offset_t offset, file_offset_t length) {
  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);
    view->selection = range_tree_expand(view->selection, offset, length);
    base->bookmarks = range_tree_expand(base->bookmarks, offset, length);
    document_file_expand(&view->selection_end, offset, length);
    document_file_expand(&view->selection_start, offset, length);
    document_file_expand(&view->selection_low, offset, length);
    document_file_expand(&view->selection_high, offset, length);
    document_file_expand(&view->offset, offset, length);

    views = views->next;
  }
}

void document_file_insert(struct document_file* base, file_offset_t offset, const uint8_t* text, size_t length) {
  if (base->buffer && offset>base->buffer->length) {
    return;
  }

  file_offset_t old_length = base->buffer?base->buffer->length:0;
  base->buffer = range_tree_insert_split(base->buffer, offset, text, length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, NULL);
  length = (base->buffer?base->buffer->length:0)-old_length;
  if (length<=0) {
    return;
  }

  document_undo_add(base, NULL, offset, length, TIPPSE_UNDO_TYPE_INSERT);
  document_file_expand_all(base, offset, length);
  document_undo_empty(base, base->redos);
}

void document_file_insert_buffer(struct document_file* base, file_offset_t offset, struct range_tree_node* buffer) {
  if (!buffer) {
    return;
  }

  file_offset_t length = buffer->length;

  if (base->buffer && offset>base->buffer->length) {
    return;
  }

  base->buffer = range_tree_paste(base->buffer, buffer, offset, base);
  document_undo_add(base, NULL, offset, length, TIPPSE_UNDO_TYPE_INSERT);
  document_file_expand_all(base, offset, length);
  document_undo_empty(base, base->redos);
}

// Correct file offset by reduce offset and length
void document_file_reduce(file_offset_t* pos, file_offset_t offset, file_offset_t length) {
  if (*pos>=offset && *pos!=FILE_OFFSET_T_MAX) {
    if ((*pos-offset)>=length) {
      *pos-=length;
    } else {
      *pos = offset;
    }
  }
}

// Correct all file offsets by reduce offset and length
void document_file_reduce_all(struct document_file* base, file_offset_t offset, file_offset_t length) {
  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);
    view->selection = range_tree_reduce(view->selection, offset, length);
    base->bookmarks = range_tree_reduce(base->bookmarks, offset, length);
    document_file_reduce(&view->selection_end, offset, length);
    document_file_reduce(&view->selection_start, offset, length);
    document_file_reduce(&view->selection_low, offset, length);
    document_file_reduce(&view->selection_high, offset, length);
    document_file_reduce(&view->offset, offset, length);

    views = views->next;
  }
}

// Delete file buffer
void document_file_delete(struct document_file* base, file_offset_t offset, file_offset_t length) {
  if (!base->buffer || offset>=base->buffer->length) {
    return;
  }

  document_undo_add(base, NULL, offset, length, TIPPSE_UNDO_TYPE_DELETE);

  file_offset_t old_length = base->buffer?base->buffer->length:0;
  base->buffer = range_tree_delete(base->buffer, offset, length, 0, base);
  length = old_length-(base->buffer?base->buffer->length:0);

  document_file_reduce_all(base, offset, length);
  document_undo_empty(base, base->redos);
}

// Delete file buffer selection
int document_file_delete_selection(struct document_file* base, struct document_view* view) {
  if (!base->buffer) {
    return 0;
  }

  file_offset_t low = view->selection_low;
  if (low>base->buffer->length) {
    low = base->buffer->length;
  }

  file_offset_t high = view->selection_high;
  if (high>base->buffer->length) {
    high = base->buffer->length;
  }

  if (high-low==0) {
    return 0;
  }

  document_file_delete(base, low, high-low);
  view->offset = low;
  view->selection_low = FILE_OFFSET_T_MAX;
  view->selection_high = FILE_OFFSET_T_MAX;
  return 1;
}

// Change document data structure if not real file
void document_file_manualchange(struct document_file* base) {
  base->bookmarks = range_tree_static(base->bookmarks, base->buffer?base->buffer->length:0, 0);

  document_view_filechange(base->view, base);

  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);
    document_view_filechange(view, base);

    views = views->next;
  }
}

// Reset all active views to the document
void document_file_reset_views(struct document_file* base) {
  document_view_reset(base->view, base);
  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);
    document_view_reset(view, base);

    views = views->next;
  }
}

// Reload file configuration
void document_file_reload_config(struct document_file* base) {
  if (!base->config) {
    return;
  }

  config_clear(base->config);

  if (*base->filename) {
    config_loadpaths(base->config, base->filename, !is_directory(base->filename));
  } else {
    config_loadpaths(base->config, ".", 0);
  }

  struct trie_node* color_base = config_find_ascii(base->config, "/colors/");
  if (color_base) {
    for (size_t n = 0; visual_color_codes[n].text; n++) {
      base->defaults.colors[visual_color_codes[n].value] = (int)config_convert_int64_cache(config_advance_ascii(base->config, color_base, visual_color_codes[n].text), &screen_color_codes[0]);
    }
  }

  base->defaults.wrapping = (int)config_convert_int64(config_find_ascii(base->config, "/wrapping"));
  base->defaults.invisibles = (int)config_convert_int64(config_find_ascii(base->config, "/invisibles"));
  base->defaults.continuous = (int)config_convert_int64(config_find_ascii(base->config, "/continuous"));
}

// Link a cache to the current document and count how often it is referenced
void document_file_reference_cache(struct document_file* base, struct file_cache* cache) {
  struct list_node* caches = base->caches->first;
  while (caches) {
    struct document_file_cache* node = (struct document_file_cache*)list_object(caches);
    if (node->cache==cache) {
      node->count++;
      return;
    }

    caches = caches->next;
  }

  struct document_file_cache* node = (struct document_file_cache*)list_object(list_insert_empty(base->caches, NULL));
  node->count = 1;
  node->cache = cache;
}

// Decrement reference counter and unlink cache if needed
void document_file_dereference_cache(struct document_file* base, struct file_cache* cache) {
  struct list_node* caches = base->caches->first;
  while (caches) {
    struct document_file_cache* node = (struct document_file_cache*)list_object(caches);
    if (node->cache==cache) {
      node->count--;
      if (node->count==0) {
        list_remove(base->caches, caches);
      }

      return;
    }

    caches = caches->next;
  }
}

// Check if one of the linked file caches was invalidated
int document_file_modified_cache(struct document_file* base) {
  int modified = 0;
  struct list_node* caches = base->caches->first;
  while (caches) {
    struct document_file_cache* node = (struct document_file_cache*)list_object(caches);
    modified |= file_cache_modified(node->cache);
    caches = caches->next;
  }

  return modified;
}
