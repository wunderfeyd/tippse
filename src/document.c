// Tippse - Document - Interface for different view types and common meta document functions

#include "document.h"

#include "clipboard.h"
#include "config.h"
#include "library/directory.h"
#include "document_text.h"
#include "documentfile.h"
#include "documentundo.h"
#include "documentview.h"
#include "editor.h"
#include "library/thread.h"
#include "library/encoding.h"
#include "library/encoding/utf8.h"
#include "filetype.h"
#include "library/filecache.h"
#include "library/misc.h"
#include "library/rangetree.h"
#include "screen.h"
#include "library/search.h"
#include "library/trie.h"
#include "library/unicode.h"

// Build search dependent on the type and parameters
struct search* document_search_build(struct document_file* file, struct range_tree* search_text, struct encoding* search_encoding, int reverse, int ignore_case, int regex) {
  struct stream needle_stream;
  stream_from_page(&needle_stream, range_tree_node_first(search_text->root), 0);
  struct search* search;
  if (regex) {
    search = search_create_regex(ignore_case, reverse, &needle_stream, search_encoding, file->encoding);
  } else {
    search = search_create_plain(ignore_case, reverse, &needle_stream, search_encoding, file->encoding);
  }
  stream_destroy(&needle_stream);
  return search;
}

// Search in document
int document_search(struct document_file* file, struct document_view* view, struct range_tree* search_text, struct encoding* search_encoding, struct range_tree* replace_text, struct encoding* replace_encoding, int reverse, int ignore_case, int regex, int all, int replace) {
  if (!search_text || !search_text->root || !file->buffer.root) {
    editor_console_update(file->editor, "No text to search for!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
    document_select_nothing(file, view);
    return 0;
  }

  struct search* search = document_search_build(file, search_text, search_encoding, reverse, ignore_case, regex);

  if (!replace && all) {
    document_view_select_nothing(view, file);
  }

  struct range_tree* replacement_transform = NULL;
  file_offset_t offset = view->offset;
  file_offset_t begin = 0;
  file_offset_t replacements = 0;
  file_offset_t matches = 0;
  file_offset_t first_low;
  file_offset_t first_high;
  file_offset_t selection_low;
  file_offset_t selection_high;
  document_view_select_next(view, 0, &selection_low, &selection_high);
  first_low = selection_low;
  first_high = selection_high;

  int first = 1;
  int found = 1;
  int wrapped = 0;
  while (1) {
    if (replace && selection_low!=FILE_OFFSET_T_MAX && found && !first) {
      if (replacements==0) {
        document_undo_chain(file, file->undos);
      }

      replacements++;
      struct range_tree* replacement;
      if (regex) {
        replacement = search_replacement(search, replace_text, replace_encoding, &file->buffer);
      } else {
        if (file->encoding==replace_encoding) {
          replacement = replace_text;
        } else {
          if (!replacement_transform) {
            replacement_transform = encoding_transform_page(replace_text->root, 0, FILE_OFFSET_T_MAX, replace_encoding, file->encoding);
          }

          replacement = replacement_transform;
        }
      }
      document_file_reduce(&begin, selection_low, selection_high-selection_low);
      document_select_delete(file, view);

      file_offset_t start = view->offset;
      file_offset_t end = start;
      if (replacement) {
        if (begin!=view->offset) {
          document_file_expand(&begin, view->offset, range_tree_length(replacement));
        }
        document_file_insert_buffer(file, view->offset, replacement->root);
        end = view->offset;

        if (regex) {
          range_tree_destroy(replacement);
        }
      }

      document_view_select_range(view, start, end, TIPPSE_INSERTER_MARK|TIPPSE_INSERTER_NOFUSE);
      selection_low = start;
      selection_high = end;
    }

    if (!file->buffer.root) {
      found = 0;
      break;
    }

    if (found) {
      if (!reverse) {
        if (selection_low!=FILE_OFFSET_T_MAX) {
          offset = (first && replace)?selection_low:selection_high;
        }
      } else {
        if (selection_low!=FILE_OFFSET_T_MAX) {
          offset = (first && replace)?selection_high:selection_low;
          if (offset==0) {
            offset = range_tree_length(&file->buffer);
          } else {
            offset--;
          }
        }
      }
    }

    if (offset>range_tree_length(&file->buffer)) {
      offset = range_tree_length(&file->buffer);
    }

    if (first) {
      begin = offset;
    }

    file_offset_t displacement;
    struct range_tree_node* buffer = range_tree_node_find_offset(file->buffer.root, offset, &displacement);
    struct stream text_stream;
    stream_from_page(&text_stream, buffer, displacement);

    file_offset_t left;
    if (!reverse) {
      if (wrapped && offset>=begin) {
        left = 0;
      } else if (offset<begin) {
        left = begin-offset;
      } else {
        left = range_tree_length(&file->buffer)-offset;
      }
    } else {
      if (wrapped && offset<=begin) {
        left = 0;
      } else if (offset>begin) {
        left = offset-begin+1;
      } else {
        left = offset+1;
      }
    }

    found = 0;
    if (left>0) {
      while (1) {
        int hit = search_find(search, &text_stream, &left, NULL);
        if (!hit) {
          break;
        }
        file_offset_t start = stream_offset_page(&search->hit_start);
        file_offset_t end = stream_offset_page(&search->hit_end);
        if ((!reverse && start>=offset) || (reverse && start<=offset)) {
          view->offset = reverse?start:end;
          offset = view->offset;

          if (replace || !all) {
            document_view_select_nothing(view, file);
          }
          document_view_select_range(view, start, end, TIPPSE_INSERTER_MARK|TIPPSE_INSERTER_NOFUSE);
          selection_low = start;
          selection_high = end;
          found = 1;
          break;
        }
      }
    }
    stream_destroy(&text_stream);

    if (found) {
      matches++;
    }

    if ((left==0 && wrapped) || (found && !all && (!replace || !first || first_low!=selection_low || first_high!=selection_high))) {
      break;
    }
    first = 0;

    if (!found) {
      if (wrapped) {
        break;
      } else {
        if (!reverse) {
          offset = 0;
        } else {
          offset = range_tree_length(&file->buffer);
        }

        wrapped = 1;
        if (begin==offset) {
          break;
        }
        continue;
      }
    }
  }

  search_destroy(search);

  if (replacement_transform) {
    range_tree_destroy(replacement_transform);
  }

  if (replacements>0) {
    document_undo_chain(file, file->undos);
  }

  if (replace) {
    char status[1024];
    sprintf(&status[0], "%d replacement(s)", (int)replacements);
    editor_console_update(file->editor, &status[0], SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
    return 1;
  }

  if (!replace && all) {
    char status[1024];
    sprintf(&status[0], "%d match(es)", (int)matches);
    editor_console_update(file->editor, &status[0], SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
    return 1;
  }

  if (found) {
    return 1;
  }

  editor_console_update(file->editor, "Not found!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
  document_select_nothing(file, view);
  return 0;
}

// Search in directory
void document_search_directory(struct thread* thread, struct document_file* pipe, const char* path, struct range_tree* search_text, struct encoding* search_encoding, struct range_tree* replace_text, struct encoding* replace_encoding, int ignore_case, int regex, int replace, const char* pattern_text, struct encoding* pattern_encoding, int binary) {
  size_t length = strlen(path)+1024;
  char* output = malloc(sizeof(char)*length);

  {
    size_t out = (size_t)sprintf(output, "Scanning %s...\n", path);
    document_file_fill_pipe(pipe, (uint8_t*)output, out);
  }

  struct list* entries = list_create(sizeof(char*));
  char* copy = strdup(path);
  list_insert(entries, entries->last, &copy);

  int hits = 0;
  int hits_lines = 0;
  while (entries->first && !thread->shutdown) {
    struct list_node* insert = entries->last;
    char* scan = *(char**)list_object(insert);
    if (is_directory(scan)) {
      struct directory* directory = directory_create(scan);
      while (1) {
        const char* filename = directory_next(directory);
        if (!filename) {
          break;
        }

        if (strcmp(filename, "..")!=0 && strcmp(filename, ".")!=0) {
          char* result = combine_path_file(scan, filename);
          list_insert(entries, insert, &result);
        }
      }
      directory_destroy(directory);
    } else {
      struct stream pattern_stream;
      stream_from_plain(&pattern_stream, (uint8_t*)pattern_text, strlen(pattern_text));
      struct stream filename_stream;
      stream_from_plain(&filename_stream, (uint8_t*)scan, strlen(scan));
      struct search* pattern = search_create_regex(0, 0, &pattern_stream, pattern_encoding, encoding_utf8_static());
      if (search_find(pattern, &filename_stream, NULL, &thread->shutdown)) {
        struct document_file* file = document_file_create(0, 0, NULL);
        struct file_cache* cache = file_cache_create(scan);
        struct stream stream;
        stream_from_file(&stream, cache, 0);
        document_file_detect_properties_stream(file, &stream);
        if (!file->binary || binary) {
          struct search* search = document_search_build(file, search_text, search_encoding, 0, ignore_case, regex);
          file_offset_t line_previous = 0;
          file_offset_t line = 1;
          struct stream newlines;
          stream_clone(&newlines, &stream);
          file_offset_t line_hit = line;
          struct stream line_start;
          stream_clone(&line_start, &newlines);
          while (!stream_end(&stream) && !thread->shutdown) {
            int found = search_find(search, &stream, NULL, &thread->shutdown);
            if (!found) {
              break;
            }
            file_offset_t hit_start = stream_offset_file(&search->hit_start);
            file_offset_t hit_end = stream_offset_file(&search->hit_end);
            while (stream_offset(&newlines)<=hit_start) {
              line_hit = line;
              stream_destroy(&line_start);
              stream_clone(&line_start, &newlines);
              while (!stream_end(&newlines) && stream_read_forward(&newlines)!='\n') {
              }
              line++;
            }
            hits++;

            if (line_hit!=line_previous) {
              hits_lines++;
              size_t out = (size_t)sprintf(output, "%s:%d: ", scan, (int)line_hit);
              int columns = 80;
              int max = 512;
              struct stream line_copy;
              stream_clone(&line_copy, &line_start);
              while (!stream_end(&line_copy) && columns>0 && max>0) {
                file_offset_t pos = stream_offset_file(&line_copy);
                if (pos==hit_start) {
                  output[out++] = '\b';
                }

                if (pos==hit_end) {
                  output[out++] = '\b';
                  if (columns<10) {
                    columns = 10;
                  }
                }

                if (pos>hit_end) {
                  columns--;
                }

                max--;

                uint8_t index = stream_read_forward(&line_copy);
                if (index=='\n') {
                  break;
                }
                if (index>=0x20 || index=='\t') {
                  output[out++] = (char)index;
                }
              }
              output[out++] = '\n';
              document_file_fill_pipe(pipe, (uint8_t*)output, out);
              stream_destroy(&line_copy);
              line_previous = line_hit;
            }
          }
          stream_destroy(&newlines);
          stream_destroy(&line_start);

          search_destroy(search);
        }

        stream_destroy(&stream);
        file_cache_dereference(cache);
        document_file_destroy(file);
      }

      search_destroy(pattern);
      stream_destroy(&pattern_stream);
      stream_destroy(&filename_stream);
    }
    free(scan);
    list_remove(entries, insert);
  }

  while (entries->first) {
    char* scan = *(char**)list_object(entries->first);
    free(scan);
    list_remove(entries, entries->first);
  }

  list_destroy(entries);

  {
    size_t out = (size_t)sprintf(output, "... %s (%d hit(s) in %d line(s) found)\n", thread->shutdown?"aborted":"done", hits, hits_lines);
    document_file_fill_pipe(pipe, (uint8_t*)output, out);
  }

  free(output);
}

// Check file properties and return highlight information
TIPPSE_INLINE int document_directory_highlight(const char* path) {
  if (is_directory(path)) {
    return TIPPSE_INSERTER_HIGHLIGHT|(VISUAL_FLAG_COLOR_DIRECTORY<<TIPPSE_INSERTER_HIGHLIGHT_COLOR_SHIFT);
  } else if (!is_file(path)) {
    return TIPPSE_INSERTER_HIGHLIGHT|(VISUAL_FLAG_COLOR_REMOVED<<TIPPSE_INSERTER_HIGHLIGHT_COLOR_SHIFT);
  }
  return 0;
}

// Read directory into document, sort by file name
void document_directory(struct document_file* file, struct stream* filter_stream, struct encoding* filter_encoding, const char* predefined) {
  char* dir_name = file->filename;
  struct directory* directory = directory_create(dir_name);
  struct search* search = filter_stream?search_create_plain(1, 0, filter_stream, filter_encoding, file->encoding):NULL;

  struct list* files = list_create(sizeof(char*));
  while (1) {
    const char* filename = directory_next(directory);
    if (!filename) {
      break;
    }

    struct stream text_stream;
    stream_from_plain(&text_stream, (uint8_t*)filename, strlen(filename));
    if (!search || search_find(search, &text_stream, NULL, NULL)) {
      char* name = strdup(filename);
      list_insert(files, NULL, &name);
    }
    stream_destroy(&text_stream);
  }

  directory_destroy(directory);

  char** sort1 = (char**)malloc(sizeof(char*)*files->count);
  char** sort2 = (char**)malloc(sizeof(char*)*files->count);
  struct list_node* name = files->first;
  for (size_t n = 0; n<files->count && name; n++) {
    sort1[n] = *(char**)list_object(name);
    name = name->next;
  }

  char** sort = (char**)merge_sort((void**)sort1, (void**)sort2, files->count, merge_sort_asciiz);

  document_file_empty(file);

  if (predefined && *predefined) {
    char* combined = combine_path_file(file->filename, predefined);
    document_file_insert(file, range_tree_length(&file->buffer), (uint8_t*)predefined, strlen(predefined), TIPPSE_INSERTER_NOFUSE|document_directory_highlight(combined));
    free(combined);
  }

  for (size_t n = 0; n<files->count; n++) {
    char* combined = combine_path_file(file->filename, sort[n]);
    size_t length = strlen(sort[n]);

    struct stream text_stream;
    stream_from_plain(&text_stream, (uint8_t*)sort[n], length);
    if (!search || search_find(search, &text_stream, NULL, NULL)) {
      document_insert_search(file, search, sort[n], length, document_directory_highlight(combined));
    }
    stream_destroy(&text_stream);
    free(combined);
    free(sort[n]);
  }

  free(sort2);
  free(sort1);

  while (files->first) {
    list_remove(files, files->first);
  }

  if (search) {
    search_destroy(search);
  }

  list_destroy(files);
}

// Document insert search string
void document_insert_search(struct document_file* file, struct search* search, const char* output, size_t length, int inserter) {
  if (file->buffer.root) {
    document_file_insert_utf8(file, range_tree_length(&file->buffer), "\n", 1, TIPPSE_INSERTER_NOFUSE);
  }

  inserter |= TIPPSE_INSERTER_NOFUSE;
  if (search) {
    size_t start = (size_t)stream_offset(&search->hit_start);
    size_t end = (size_t)stream_offset(&search->hit_end);
    document_file_insert(file, range_tree_length(&file->buffer), (uint8_t*)&output[0], start, inserter);
    document_file_insert(file, range_tree_length(&file->buffer), (uint8_t*)&output[start], end-start, TIPPSE_INSERTER_NOFUSE|TIPPSE_INSERTER_HIGHLIGHT|(VISUAL_FLAG_COLOR_STRING<<TIPPSE_INSERTER_HIGHLIGHT_COLOR_SHIFT));
    document_file_insert(file, range_tree_length(&file->buffer), (uint8_t*)&output[end], length-end, inserter);
  } else {
    document_file_insert(file, range_tree_length(&file->buffer), (uint8_t*)&output[0],  length, inserter);
  }
}

// Select whole document
void document_select_all(struct document_file* file, struct document_view* view, int update_offset) {
  file_offset_t end = range_tree_length(&file->buffer);
  if (update_offset) {
    view->offset = end;
  }
  document_view_select_all(view, file);
}

// Throw away all selections
void document_select_nothing(struct document_file* file, struct document_view* view) {
  document_view_select_nothing(view, file);
}

// Delete selection
int document_select_delete(struct document_file* file, struct document_view* view) {
  file_offset_t low;
  file_offset_t high;
  int removed = 0;
  while (document_view_select_next(view, 0, &low, &high)) {
    document_file_delete(file, low, high-low);
    removed = 1;
  }

  return removed;
}

// Copy selection
void document_clipboard_copy(struct document_file* file, struct document_view* view) {
  file_offset_t low;
  file_offset_t high = 0;
  struct range_tree* copy = range_tree_create(NULL, 0);
  while (document_view_select_next(view, high, &low, &high)) {
    range_tree_node_copy_insert(file->buffer.root, low, copy, range_tree_length(copy), high-low);
  }
  clipboard_set(copy, file->binary, file->encoding);
}

// Paste selection
void document_clipboard_paste(struct document_file* file, struct document_view* view) {
  struct encoding* encoding = NULL;
  int binary = 0;
  struct range_tree* buffer = clipboard_get(&encoding, &binary);
  if (buffer) {
    struct range_tree* transform = binary?buffer:encoding_transform_page(buffer->root, 0, FILE_OFFSET_T_MAX, encoding, file->encoding);
    if (transform) {
      document_file_insert_buffer(file, view->offset, transform->root);
      if (!binary) {
        range_tree_destroy(transform);
      }
    }
    range_tree_destroy(buffer);
  }

  encoding->destroy(encoding);
}

// Append data to clipboard
void document_clipboard_extend(struct document_file* file, struct document_view* view, file_offset_t from, file_offset_t to, int extend) {
  struct encoding* encoding = extend?NULL:file->encoding->create();
  int binary = 0;
  struct range_tree* buffer = extend?clipboard_get(&encoding, &binary):range_tree_create(NULL, 0);
  if (buffer) {
    struct range_tree* transform;
    if (binary || !buffer->root) {
      transform = buffer;
    } else {
      transform = encoding_transform_page(buffer->root, 0, FILE_OFFSET_T_MAX, encoding, file->encoding);
      range_tree_destroy(buffer);
    }
    range_tree_node_copy_insert(file->buffer.root, from, transform, range_tree_length(transform), to-from);
    clipboard_set(transform, file->binary, file->encoding);
  }

  encoding->destroy(encoding);
}

// Toggle bookmark
void document_bookmark_toggle_range(struct document_file* file, file_offset_t low, file_offset_t high) {
  int marked = range_tree_node_marked(file->bookmarks.root, low, high-low, TIPPSE_INSERTER_MARK);
  document_bookmark_range(file, low, high, marked);
}

// Add range to bookmark list
void document_bookmark_range(struct document_file* file, file_offset_t low, file_offset_t high, int marked) {
  range_tree_mark(&file->bookmarks, low, high-low, marked?0:TIPPSE_INSERTER_MARK|TIPPSE_INSERTER_NOFUSE);
}

// Bookmark selection
void document_bookmark_selection(struct document_file* file, struct document_view* view, int marked) {
  file_offset_t low;
  file_offset_t high = 0;
  while (document_view_select_next(view, high, &low, &high)) {
    document_bookmark_range(file, low, high, marked);
  }
}

// Bookmark selection (invert)
void document_bookmark_toggle_selection(struct document_file* file, struct document_view* view) {
  file_offset_t low;
  file_offset_t high = 0;
  while (document_view_select_next(view, high, &low, &high)) {
    document_bookmark_toggle_range(file, low, high);
  }
}

// Goto next/previous bookmark
void document_bookmark_find(struct document_file* file, struct document_view* view, int reverse) {
  file_offset_t offset = view->offset;
  int wrap = 1;
  while (1) {
    file_offset_t low;
    file_offset_t high;
    if (!reverse && range_tree_marked_next(&file->bookmarks, offset, &low, &high, wrap)) {
      view->offset = low;
      break;
    }
    if (reverse && range_tree_marked_prev(&file->bookmarks, offset, &low, &high, wrap)) {
      view->offset = low;
      break;
    }
    if (!wrap) {
      editor_console_update(file->editor, "No bookmark found!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
      break;
    }
    wrap = 0;
    offset = reverse?range_tree_length(&file->buffer):0;
  }
}

int document_keypress(struct document* base, struct document_view* view, struct document_file* file, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y, file_offset_t selection_low, file_offset_t selection_high, int* selection_keep, int* seek, file_offset_t file_size, file_offset_t* offset_old) {
  if (command==TIPPSE_CMD_UNDO) {
    document_undo_execute_chain(file, view, file->undos, file->redos, 0);
    *seek = 1;
  } else if (command==TIPPSE_CMD_REDO) {
    document_undo_execute_chain(file, view, file->redos, file->undos, 1);
    *seek = 1;
  } else if (command==TIPPSE_CMD_BOOKMARK_NEXT) {
    document_bookmark_find(file, view, 0);
    *seek = 1;
  } else if (command==TIPPSE_CMD_BOOKMARK_PREV) {
    document_bookmark_find(file, view, 1);
    *seek = 1;
  } else if (command==TIPPSE_CMD_COPY || command==TIPPSE_CMD_CUT) {
    document_undo_chain(file, file->undos);
    if (selection_low!=FILE_OFFSET_T_MAX) {
      document_clipboard_copy(file, view);
      if (command==TIPPSE_CMD_CUT) {
        document_select_delete(file, view);
      } else {
        *selection_keep = 1;
      }
    }
    document_undo_chain(file, file->undos);
    *seek = 1;
  } else if (command==TIPPSE_CMD_PASTE) {
    document_undo_chain(file, file->undos);
    document_select_delete(file, view);
    document_clipboard_paste(file, view);
    document_undo_chain(file, file->undos);
    *seek = 1;
  } else if (command==TIPPSE_CMD_SHOW_INVISIBLES) {
    view->show_invisibles ^= 1;
    (*base->reset)(base, view, file);
  } else if (command==TIPPSE_CMD_WORDWRAP) {
    view->wrapping ^= 1;
    (*base->reset)(base, view, file);
  } else if (command==TIPPSE_CMD_SELECT_ALL) {
    *offset_old = view->selection_start = 0;
    view->offset = view->selection_end = file_size;
  } else {
    return 0;
  }

  return 1;
}
