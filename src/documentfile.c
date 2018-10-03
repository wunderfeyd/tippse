// Tippse - Document - File operations and file setup

#include "documentfile.h"

#include "config.h"
#include "documentundo.h"
#include "documentview.h"
#include "encoding.h"
#include "encoding/utf8.h"
#include "encoding/utf16le.h"
#include "encoding/utf16be.h"
#include "encoding/cp850.h"
#include "encoding/ascii.h"
#include "editor.h"
#include "file.h"
#include "filecache.h"
#include "filetype.h"
#include "filetype/c.h"
#include "filetype/sql.h"
#include "filetype/text.h"
#include "filetype/lua.h"
#include "filetype/patch.h"
#include "filetype/php.h"
#include "filetype/xml.h"
#include "fragment.h"
#include "list.h"
#include "misc.h"
#include "rangetree.h"
#include "trie.h"
#include "unicode.h"

extern struct config_cache screen_color_codes[];
extern struct config_cache visual_color_codes[VISUAL_FLAG_COLOR_MAX+1];

// Tabstop types
struct config_cache document_file_tabstop[TIPPSE_TABSTOP_MAX+1] = {
  {"auto", TIPPSE_TABSTOP_AUTO, NULL},
  {"tab", TIPPSE_TABSTOP_TAB, NULL},
  {"space", TIPPSE_TABSTOP_SPACE, NULL},
  {NULL, 0, NULL}
};

// Newline types
struct config_cache document_file_newline[TIPPSE_NEWLINE_MAX+1] = {
  {"auto", TIPPSE_NEWLINE_AUTO, NULL},
  {"lf", TIPPSE_NEWLINE_LF, NULL},
  {"cr", TIPPSE_NEWLINE_CR, NULL},
  {"crlf", TIPPSE_NEWLINE_CRLF, NULL},
  {NULL, 0, NULL}
};

// TODO: this has to be covered by the settings subsystem in future
struct document_file_parser document_file_parsers[] = {
  {"c", file_type_c_create},
  {"sql", file_type_sql_create},
  {"lua", file_type_lua_create},
  {"php", file_type_php_create},
  {"xml", file_type_xml_create},
  {"patch", file_type_patch_create},
  {NULL,  NULL}
};

// Create file operations
struct document_file* document_file_create(int save, int config, struct editor* editor) {
  struct document_file* base = (struct document_file*)malloc(sizeof(struct document_file));
  base->editor = editor;
  base->buffer = NULL;
  base->bookmarks = NULL;
  base->cache = NULL;
  base->caches = list_create(sizeof(struct document_file_cache));
  base->binary = 0;
  base->draft = 0;
  base->undo = 1;
  base->undos = list_create(sizeof(struct document_undo));
  base->redos = list_create(sizeof(struct document_undo));
  base->filename = strdup("");
  base->views = list_create(sizeof(struct document_view*));
  base->save = save;
  base->save_skip = 0;
  base->line_select = 0;
  base->encoding = encoding_utf8_create();
  base->tabstop = TIPPSE_TABSTOP_AUTO;
  base->tabstop_width = 4;
  base->newline = TIPPSE_NEWLINE_AUTO;
  base->config = config?config_create():NULL;
  base->type = file_type_text_create(base->config, "");
  base->view = document_view_create();
#ifndef _WINDOWS
  base->pipefd[0] = -1;
  base->pipefd[1] = -1;
#endif
  base->autocomplete_offset = 0;
  base->autocomplete_last = NULL;
  base->autocomplete_build = NULL;
  base->autocomplete_rescan = 0;
  base->defaults.newline = TIPPSE_NEWLINE_AUTO;
  base->defaults.tabstop = TIPPSE_TABSTOP_AUTO;
  base->defaults.tabstop_width = 0;
  base->defaults.invisibles = 0;
  base->defaults.wrapping = 0;
  base->defaults.address_width = 6;
  base->defaults.line_width = 80;
  document_file_reset_views(base, 1);
  document_undo_mark_save_point(base);
  document_file_reload_config(base);
  return base;
}

// Clear file operations
void document_file_clear(struct document_file* base, int all) {
  if (base->cache) {
    document_file_dereference_cache(base, base->cache);
    file_cache_dereference(base->cache);
    base->cache = NULL;
  }

  if (base->buffer) {
    range_tree_destroy(base->buffer, base);
    base->buffer = NULL;
  }

  if (all) {
    if (base->bookmarks) {
      range_tree_destroy(base->bookmarks, NULL);
      base->bookmarks = NULL;
    }

    if (base->config) {
      config_clear(base->config);
    }
  }
}

// Destroy file operations
void document_file_destroy(struct document_file* base) {
  document_file_clear(base, 1);
#ifndef _WINDOWS
  document_file_close_pipe(base);
#endif
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
  if (base->autocomplete_build) {
    trie_destroy(base->autocomplete_build);
  }
  if (base->autocomplete_last) {
    trie_destroy(base->autocomplete_last);
  }
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
  base->draft = 0;
}

// Set file draft status
void document_file_draft(struct document_file* base) {
  base->draft = 1;
}

// Retrieve file draft status
int document_file_drafted(struct document_file* base) {
  return base->draft;
}

// Change encoding
void document_file_encoding(struct document_file* base, struct encoding* encoding) {
  (*base->encoding->destroy)(base->encoding);
  base->encoding = encoding;
}

#ifndef _WINDOWS
// Create another process or thread and route the output into the file
void document_file_create_pipe(struct document_file* base) {
  range_tree_destroy(base->buffer, base);
  base->buffer = NULL;
  document_undo_empty(base, base->undos);
  document_undo_empty(base, base->redos);
  document_file_reset_views(base, 1);

  document_file_close_pipe(base);

  UNUSED(pipe(base->pipefd));

  signal(SIGCHLD, SIG_IGN);
  base->pid = fork();
  if (base->pid==0) {
    dup2(base->pipefd[0], 0);
    dup2(base->pipefd[1], 1);
    dup2(base->pipefd[1], 2);
    close(base->pipefd[0]);
    close(base->pipefd[1]);
  } else {
    close(base->pipefd[1]);

    int flags = fcntl(base->pipefd[0], F_GETFL, 0);
    fcntl(base->pipefd[0], F_SETFL, flags|O_NONBLOCK);

    document_undo_mark_save_point(base);
    document_file_detect_properties(base);
    base->bookmarks = range_tree_static(base->bookmarks, range_tree_length(base->buffer), 0);
    document_file_reset_views(base, 1);

    (*base->type->destroy)(base->type);
    base->type = file_type_c_create(base->config, "compiler_output");
  }
}

// Execute system command and push output into file contents
void document_file_pipe(struct document_file* base, const char* command) {
  document_file_name(base, command);
  document_file_create_pipe(base);
  if (base->pid==0) {
    char* argv[4];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = (char*)command;
    argv[3] = NULL;
    setpgid(0, 0);
    execv(argv[0], &argv[0]);
    exit(0);
  }
}

// Append incoming data from pipe
void document_file_fill_pipe(struct document_file* base, uint8_t* buffer, size_t length) {
  if (length>0) {
    uint8_t* copy = (uint8_t*)malloc(length);
    memcpy(copy, buffer, length);
    file_offset_t offset = range_tree_length(base->buffer);
    struct fragment* fragment = fragment_create_memory(copy, length);
    base->buffer = range_tree_insert(base->buffer, offset, fragment, 0, length, 0, 0, base, NULL);
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

// Close incoming pipe
void document_file_kill_pipe(struct document_file* base) {
  if (base->pipefd[0]==-1) {
    return;
  }

  killpg(base->pid, SIGKILL);

  base->pipefd[0] = -1;
  base->pipefd[1] = -1;
}
#endif

// Load file from file system, up to a certain threshold
void document_file_load(struct document_file* base, const char* filename, int reload, int reset) {
  document_file_clear(base, !reload);
  int opened = 0;
  if (!is_directory(filename)) {
    struct file* f = file_create(filename, TIPPSE_FILE_READ);
    if (f) {
      opened = 1;
      base->cache = file_cache_create(filename);
      document_file_reference_cache(base, base->cache);

      file_offset_t length = file_seek(f, 0, TIPPSE_SEEK_END);
      file_seek(f, 0, TIPPSE_SEEK_START);
      file_offset_t offset = 0;
      while (1) {
        file_offset_t block = 0;
        struct fragment* fragment = NULL;
        if (length<TIPPSE_DOCUMENT_MEMORY_LOADMAX) {
          uint8_t* copy = (uint8_t*)malloc(TREE_BLOCK_LENGTH_MIN);
          block = (file_offset_t)file_read(f, copy, TREE_BLOCK_LENGTH_MIN);
          fragment = fragment_create_memory(copy, (size_t)block);
        } else {
          block = length-offset;
          if (block>TREE_BLOCK_LENGTH_MAX) {
            block = TREE_BLOCK_LENGTH_MAX;
          }

          fragment = fragment_create_file(base->cache, offset, block, base);
        }

        if (block==0) {
          fragment_dereference(fragment, base);
          break;
        }

        base->buffer = range_tree_insert(base->buffer, offset, fragment, 0, fragment->length, 0, 0, base, NULL);
        fragment_dereference(fragment, base);
        offset += block;
      }

      file_destroy(f);
    }
  }

  document_file_name(base, filename);
  if (!opened) {
    document_file_draft(base);
  }

  document_file_detect_properties(base);
  base->bookmarks = range_tree_resize(base->bookmarks, range_tree_length(base->buffer), 0);
  if (!reload) {
    document_undo_empty(base, base->undos);
    document_undo_empty(base, base->redos);
  }

  document_undo_mark_save_point(base);

  if (!reset) {
    document_file_reset_views(base, !reload);
  } else {
    document_file_change_views(base, !reload);
  }
}

// Load file from memory
void document_file_load_memory(struct document_file* base, const uint8_t* buffer, size_t length) {
  document_file_clear(base, 1);
  file_offset_t offset = 0;
  while (length>0) {
    size_t max = (length>TREE_BLOCK_LENGTH_MID)?TREE_BLOCK_LENGTH_MID:length;
    uint8_t* copy = (uint8_t*)malloc(max);
    memcpy(copy, buffer, max);
    struct fragment* fragment = fragment_create_memory(copy, max);
    base->buffer = range_tree_insert(base->buffer, offset, fragment, 0, max, 0, 0, base, NULL);
    fragment_dereference(fragment, base);
    offset += max;
    length -= max;
    buffer += max;
  }

  document_undo_empty(base, base->undos);
  document_undo_empty(base, base->redos);
  document_file_name(base, "<memory>");
  document_file_detect_properties(base);
  base->bookmarks = range_tree_static(base->bookmarks, range_tree_length(base->buffer), 0);
  document_file_reset_views(base, 1);
}

// Save file directly to file system
int document_file_save_plain(struct document_file* base, const char* filename) {
  struct file* f = file_create(filename, TIPPSE_FILE_READ|TIPPSE_FILE_WRITE|TIPPSE_FILE_CREATE|TIPPSE_FILE_TRUNCATE);
  if (!f) {
    return 0;
  }

  int success = 1;
  if (base->buffer) {
    struct stream stream;
    stream_from_page(&stream, range_tree_first(base->buffer), 0);
    while (!stream_end(&stream)) {
      size_t length = stream_cache_length(&stream)-stream_displacement(&stream);
      if (file_write(f, (void*)stream_buffer(&stream), length)!=length) {
        success = 0;
        break;
      }
      stream_next(&stream);
    }
    stream_destroy(&stream);
  }

  file_destroy(f);
  document_undo_mark_save_point(base);
  return success;
}

// Save file, uses a temporary file if necessary
int document_file_save(struct document_file* base, const char* filename) {
  int success = 0;
  if (base->buffer && (base->buffer->inserter&TIPPSE_INSERTER_FILE)) {
    char* tmpname = combine_string(filename, ".save.tmp");
    if (document_file_save_plain(base, tmpname)) {
      if (rename(tmpname, filename)==0) {
        document_file_load(base, filename, 1, 0);
        editor_console_update(base->editor, "Saved!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
        success = 1;
      } else {
        editor_console_update(base->editor, "Renaming failed!", SIZE_T_MAX, CONSOLE_TYPE_ERROR);
      }
    } else {
      editor_console_update(base->editor, "Writing failed!", SIZE_T_MAX, CONSOLE_TYPE_ERROR);
    }

    free(tmpname);
  } else {
    if (document_file_save_plain(base, filename)) {
      document_undo_mark_save_point(base);
      editor_console_update(base->editor, "Saved!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
      success = 1;
    } else {
      editor_console_update(base->editor, "Writing failed!", SIZE_T_MAX, CONSOLE_TYPE_ERROR);
    }

    if (base->cache) {
      document_file_dereference_cache(base, base->cache);
      file_cache_dereference(base->cache);
      base->cache = file_cache_create(filename);
      document_file_reference_cache(base, base->cache);
    }
  }

  return success;
}

// Toggle "save skip" flag
void document_file_save_skip(struct document_file* base) {
  base->save_skip ^= 1;
}

// Detect file properties
void document_file_detect_properties(struct document_file* base) {
  if (!base->buffer) {
    return;
  }

  struct stream stream;
  stream_from_page(&stream, range_tree_first(base->buffer), 0);
  document_file_detect_properties_stream(base, &stream);
  stream_destroy(&stream);
}

// Auto detect stream properties
void document_file_detect_properties_stream(struct document_file* base, struct stream* document_stream) {
  file_offset_t offset = 0;
  struct stream stream;
  stream_clone(&stream, document_stream);

  struct encodings {
    struct encoding* (*create)(void);
    file_offset_t factor;
  };

  struct encoding_stats {
    file_offset_t good;
    file_offset_t bad;
    file_offset_t chars;
    int binary;
  };

  struct encodings encodings[] = {{encoding_ascii_create, 1}, {encoding_cp850_create, 1}, {encoding_utf8_create, 2}, {encoding_utf16le_create, 2}, {encoding_utf16be_create, 2}};
  struct encoding_stats stats[sizeof(encodings)/sizeof(struct encodings)];

  base->binary = 0;

  file_offset_t max_chars = 0;
  for (size_t n = 0; n<sizeof(encodings)/sizeof(struct encodings); n++) {
    stats[n].good = 0;
    stats[n].bad = 0;
    stats[n].chars = 0;
    stats[n].binary = 0;

    struct encoding* encoding = (*(encodings[n].create))();
    stream = *document_stream;
    offset = 0;
    while (offset<TIPPSE_DOCUMENT_AUTO_LOADMAX && !stream_end(&stream)) {
      size_t length = 1;
      codepoint_t cp = (*encoding->decode)(encoding, &stream, &length);
      codepoint_t visual = (*encoding->visual)(encoding, cp);

      stats[n].chars++;
      if (cp==-1 || visual<0 || visual==0xfffd || visual==0xfffe || (!unicode_letter(visual) && !unicode_digit(visual) && !unicode_whitespace(visual) && cp!=0xfeff && cp!='\r' && cp!='\n')) {
        if (cp==0) {
          stats[n].binary = 1;
        }

        stats[n].bad++;
      } else {
        stats[n].good++;
      }
      offset += length;
    }
    if (stats[n].chars>max_chars) {
      max_chars = stats[n].chars;
    }
    encoding->destroy(encoding);
  }

  file_offset_t max_bad = 0;
  file_offset_t best = 0;
  file_offset_t bests = 1;
  for (size_t n = 0; n<sizeof(encodings)/sizeof(struct encodings); n++) {
    if (stats[n].chars==0) {
      continue;
    }
    file_offset_t score = (stats[n].good*max_chars*encodings[n].factor)/stats[n].chars;
    // fprintf(stderr, "%s %d: %d / %d\r\n", base->filename, (int)n, (int)score, stats[n].binary);
    if (score>max_bad) {
      max_bad = score;
      best = n;
      bests = 1;
      base->binary = stats[n].binary;
    } else if (score==max_bad) {
      bests++;
    }
  }

  if (base->binary) {
    document_file_encoding(base, encoding_ascii_create());
  } else if (bests>1 && best==0) { // Default to UTF-8 if ASCII was detected // hacky hack for testing
    document_file_encoding(base, encoding_utf8_create());
  } else {
    document_file_encoding(base, (*(encodings[best].create))());
  }

  stream = *document_stream;
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
  while (offset<TIPPSE_DOCUMENT_AUTO_LOADMAX && !stream_end(&stream)) {
    size_t length = 1;
    codepoint_t cp = (*base->encoding->decode)(base->encoding, &stream, &length);

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

  // But respect users decision
  if (base->defaults.newline!=TIPPSE_NEWLINE_AUTO) {
    base->newline = base->defaults.newline;
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

  // But respect users decision
  if (base->defaults.tabstop!=TIPPSE_TABSTOP_AUTO) {
    base->tabstop = base->defaults.tabstop;
  }

  if (base->defaults.tabstop_width!=0) {
    base->tabstop_width = base->defaults.tabstop_width;
  }

  stream_destroy(&stream);
}

// Correct file offset by expansion offset and length
void document_file_expand(file_offset_t* pos, file_offset_t offset, file_offset_t length) {
  if (*pos>=offset && *pos!=FILE_OFFSET_T_MAX) {
    *pos+=length;
  }
}

// Correct all file offsets by expansion offset and length
void document_file_expand_all(struct document_file* base, file_offset_t offset, file_offset_t length) {
  base->bookmarks = range_tree_expand(base->bookmarks, offset, length);

  document_file_expand(&base->autocomplete_offset, offset, length);

  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);
    view->selection = range_tree_expand(view->selection, offset, length);
    document_file_expand(&view->selection_end, offset, length);
    document_file_expand(&view->selection_start, offset, length);
    document_file_expand(&view->offset, offset, length);

    views = views->next;
  }
}

void document_file_insert(struct document_file* base, file_offset_t offset, const uint8_t* text, size_t length, int inserter) {
  if (base->buffer && offset>base->buffer->length) {
    return;
  }

  file_offset_t old_length = range_tree_length(base->buffer);
  base->buffer = range_tree_insert_split(base->buffer, offset, text, length, inserter);
  length = range_tree_length(base->buffer)-old_length;
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
  base->bookmarks = range_tree_reduce(base->bookmarks, offset, length);

  document_file_reduce(&base->autocomplete_offset, offset, length);

  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);
    view->selection = range_tree_reduce(view->selection, offset, length);
    document_file_reduce(&view->selection_end, offset, length);
    document_file_reduce(&view->selection_start, offset, length);
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

  file_offset_t old_length = range_tree_length(base->buffer);
  base->buffer = range_tree_delete(base->buffer, offset, length, 0, base);
  length = old_length-range_tree_length(base->buffer);

  document_file_reduce_all(base, offset, length);
  document_undo_empty(base, base->redos);
}

// Clear file contents
void document_file_empty(struct document_file* base) {
  document_file_delete(base, 0, range_tree_length(base->buffer));
}

// Move offsets from one range into another
void document_file_relocate(file_offset_t* pos, file_offset_t from, file_offset_t to, file_offset_t length) {
  if (*pos>=from && *pos<from+length) {
    *pos = (*pos-from)+to;
    if (to>from) {
      *pos -= length;
    }
  } else {
    if (to<from && *pos>=to && *pos<from) {
      *pos += length;
    }
    if (to>from && *pos>=from && *pos<to) {
      *pos -= length;
    }
  }
}

// Move block from one position to another, including all metainformation and viewports
void document_file_move(struct document_file* base, file_offset_t from, file_offset_t to, file_offset_t length) {
  if (length==0 || (to>=from && to<from+length)) {
    return;
  }

  file_offset_t corrected = to;
  if (corrected>=from) {
    corrected -= length;
  }

  struct range_tree_node* buffer_file = range_tree_copy(base->buffer, from, length);
  document_undo_add(base, NULL, from, length, TIPPSE_UNDO_TYPE_DELETE);
  base->buffer = range_tree_delete(base->buffer, from, length, 0, base);
  base->buffer = range_tree_paste(base->buffer, buffer_file, corrected, base);
  document_undo_add(base, NULL, corrected, length, TIPPSE_UNDO_TYPE_INSERT);
  range_tree_destroy(buffer_file, NULL);

  struct range_tree_node* buffer_bookmarks = range_tree_copy(base->bookmarks, from, length);
  base->bookmarks = range_tree_delete(base->bookmarks, from, length, 0, NULL);
  base->bookmarks = range_tree_paste(base->bookmarks, buffer_bookmarks, corrected, NULL);
  range_tree_destroy(buffer_bookmarks, NULL);

  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);

    struct range_tree_node* buffer_selection = range_tree_copy(view->selection, from, length);
    view->selection = range_tree_delete(view->selection, from, length, 0, NULL);
    view->selection = range_tree_paste(view->selection, buffer_selection, corrected, base);
    range_tree_destroy(buffer_selection, NULL);

    document_file_relocate(&view->selection_end, from, to, length);
    document_file_relocate(&view->selection_start, from, to, length);
    document_file_relocate(&view->offset, from, to, length);

    views = views->next;
  }

  document_undo_empty(base, base->redos);
}

// Change document data structure if not real file
void document_file_change_views(struct document_file* base, int defaults) {
  base->bookmarks = range_tree_resize(base->bookmarks, range_tree_length(base->buffer), 0);

  document_view_filechange(base->view, base, defaults);

  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);
    document_view_filechange(view, base, defaults);

    views = views->next;
  }
}

// Reset all active views to the document
void document_file_reset_views(struct document_file* base, int defaults) {
  document_view_reset(base->view, base, defaults);
  struct list_node* views = base->views->first;
  while (views) {
    struct document_view* view = *(struct document_view**)list_object(views);
    document_view_reset(view, base, defaults);

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

  base->defaults.newline = (int)config_convert_int64_cache(config_find_ascii(base->config, "/newline"), &document_file_newline[0]);
  base->defaults.tabstop = (int)config_convert_int64_cache(config_find_ascii(base->config, "/tabstop"), &document_file_tabstop[0]);
  base->defaults.tabstop_width = (int)config_convert_int64(config_find_ascii(base->config, "/tabstop_width"));
  base->defaults.wrapping = (int)config_convert_int64(config_find_ascii(base->config, "/wrapping"));
  base->defaults.invisibles = (int)config_convert_int64(config_find_ascii(base->config, "/invisibles"));
  base->defaults.address_width = (int)config_convert_int64(config_find_ascii(base->config, "/addresswidth"));
  base->defaults.line_width = (int)config_convert_int64(config_find_ascii(base->config, "/linewidth"));

  const char* search = base->filename;
  const char* last = base->filename;
  while (*search) {
    if (*search=='.') {
      last = search+1;
    }

    search++;
  }

  if (last==base->filename) {
    last = search;
  }

  struct trie_node* node = config_find_ascii(base->config, "/fileextensions/");
  if (node) {
    node = config_advance_ascii(base->config, node, last);
  }

  if (node && node->end) {
    struct encoding* encoding = encoding_utf8_create();
    char* file_type = (char*)config_convert_encoding(node, encoding);
    encoding_utf8_destroy(encoding);

    node = config_find_ascii(base->config, "/filetypes/");
    if (node) {
      node = config_advance_ascii(base->config, node, file_type);
    }

    if (node) {
      struct trie_node* node_file_type = node;

      node = config_advance_ascii(base->config, node_file_type, "/parser");

      if (node && node->end) {
        struct encoding* encoding = encoding_utf8_create();
        char* parser = (char*)config_convert_encoding(node, encoding);
        encoding_utf8_destroy(encoding);

        for (size_t n = 0; document_file_parsers[n].name; n++) {
          if (strcmp(document_file_parsers[n].name, parser)==0) {
            (*base->type->destroy)(base->type);
            base->type = (*document_file_parsers[n].constructor)(base->config, file_type);
            break;
          }
        }

        free(parser);
      }

      int newline = (int)config_convert_int64_cache(config_advance_ascii(base->config, node_file_type, "/newline"), &document_file_newline[0]);
      if (newline!=TIPPSE_NEWLINE_AUTO) {
        base->defaults.newline = newline;
      }

      int tabstop = (int)config_convert_int64_cache(config_advance_ascii(base->config, node_file_type, "/tabstop"), &document_file_tabstop[0]);
      if (tabstop!=TIPPSE_TABSTOP_AUTO) {
        base->defaults.tabstop = tabstop;
      }

      int tabstop_width = (int)config_convert_int64(config_advance_ascii(base->config, node_file_type, "/tabstop_width"));
      if (tabstop_width!=0) {
        base->defaults.tabstop_width = tabstop_width;
      }
    }

    free(file_type);
  }
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
