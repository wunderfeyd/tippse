// Tippse - Document - Interface for different view types and common meta document functions

#include "document.h"

#include "clipboard.h"
#include "config.h"
#include "directory.h"
#include "document_text.h"
#include "documentfile.h"
#include "documentundo.h"
#include "documentview.h"
#include "editor.h"
#include "encoding.h"
#include "encoding/utf8.h"
#include "filetype.h"
#include "filecache.h"
#include "misc.h"
#include "rangetree.h"
#include "screen.h"
#include "search.h"
#include "trie.h"
#include "unicode.h"

// Search in document
int document_search(struct document_file* file, struct document_view* view, struct range_tree_node* search_text, struct encoding* search_encoding, struct range_tree_node* replace_text, struct encoding* replace_encoding, int reverse, int ignore_case, int regex, int all, int replace) {
  if (!search_text || !file->buffer) {
    editor_console_update(file->editor, "No text to search for!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
    return 0;
  }

  struct stream needle_stream;
  stream_from_page(&needle_stream, range_tree_first(search_text), 0);
  struct search* search;
  if (regex) {
    search = search_create_regex(ignore_case, reverse, &needle_stream, search_encoding, file->encoding);
  } else {
    search = search_create_plain(ignore_case, reverse, &needle_stream, search_encoding, file->encoding);
  }
  stream_destroy(&needle_stream);

  if (!replace && all) {
    document_view_select_nothing(view, file);
  }

  struct range_tree_node* replacement_transform = NULL;
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
      struct range_tree_node* replacement;
      if (regex) {
        replacement = search_replacement(search, replace_text, replace_encoding, file->buffer);
      } else {
        if (file->encoding==replace_encoding) {
          replacement = replace_text;
        } else {
          if (!replacement_transform) {
            replacement_transform = encoding_transform_page(replace_text, 0, FILE_OFFSET_T_MAX, replace_encoding, file->encoding);
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
          document_file_expand(&begin, view->offset, replacement->length);
        }
        document_file_insert_buffer(file, view->offset, replacement);
        end = view->offset;

        if (regex) {
          range_tree_destroy(replacement, NULL);
        }
      }

      document_view_select_range(view, start, end, TIPPSE_INSERTER_MARK|TIPPSE_INSERTER_NOFUSE);
      selection_low = start;
      selection_high = end;
    }

    if (!file->buffer) {
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
            offset = file->buffer->length;
          } else {
            offset--;
          }
        }
      }
    }

    if (offset>file->buffer->length) {
      offset = file->buffer->length;
    }

    if (first) {
      begin = offset;
    }

    file_offset_t displacement;
    struct range_tree_node* buffer = range_tree_find_offset(file->buffer, offset, &displacement);
    struct stream text_stream;
    stream_from_page(&text_stream, buffer, displacement);

    file_offset_t left;
    if (!reverse) {
      if (wrapped && offset>=begin) {
        left = 0;
      } else if (offset<begin) {
        left = begin-offset;
      } else {
        left = file->buffer->length-offset;
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
        int hit = search_find(search, &text_stream, &left);
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
          offset = file->buffer->length;
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
    range_tree_destroy(replacement_transform, NULL);
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
  return 0;
}

// Search in directory
void document_search_directory(const char* path, struct range_tree_node* search_text, struct encoding* search_encoding, struct range_tree_node* replace_text, struct encoding* replace_encoding, int ignore_case, int regex, int replace, const char* pattern_text, struct encoding* pattern_encoding, int binary) {
  printf("Scanning %s...\n", path);
  fflush(stdout);
  struct list* entries = list_create(sizeof(char*));
  char* copy = strdup(path);
  list_insert(entries, entries->last, &copy);

  int hits = 0;
  while (entries->first) {
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
      if (search_find(pattern, &filename_stream, NULL)) {
        struct document_file* file = document_file_create(0, 0, NULL);
        struct file_cache* cache = file_cache_create(scan);
        struct stream stream;
        stream_from_file(&stream, cache, 0);
        document_file_detect_properties_stream(file, &stream);
        if (!file->binary || binary) {
          struct stream needle_stream;
          stream_from_page(&needle_stream, range_tree_first(search_text), 0);
          struct search* search;
          if (regex) {
            search = search_create_regex(ignore_case, 0, &needle_stream, search_encoding, file->encoding);
          } else {
            search = search_create_plain(ignore_case, 0, &needle_stream, search_encoding, file->encoding);
          }
          stream_destroy(&needle_stream);

          file_offset_t line = 1;
          struct stream newlines;
          stream_clone(&newlines, &stream);
          file_offset_t line_hit = line;
          struct stream line_start;
          stream_clone(&line_start, &newlines);
          while (!stream_end(&stream)) {
            int found = search_find(search, &stream, NULL);
            if (!found) {
              break;
            }
            file_offset_t hit = stream_offset_file(&search->hit_start);
            while (stream_offset(&newlines)<=hit) {
              line_hit = line;
              stream_destroy(&line_start);
              stream_clone(&line_start, &newlines);
              while (!stream_end(&newlines) && stream_read_forward(&newlines)!='\n') {
              }
              line++;
            }
            printf("%s:%d: ", scan, (int)line_hit);
            hits++;

            int columns = 80;
            struct stream line_copy;
            stream_clone(&line_copy, &line_start);
            while (!stream_end(&line_copy) && columns>0) {
              columns--;
              uint8_t index = stream_read_forward(&line_copy);
              if (index=='\n') {
                break;
              }
              if (index>=0x20 || index=='\t') {
                printf("%c", index);
              }
            }
            printf("\n");
            fflush(stdout);
            stream_destroy(&line_copy);
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
  list_destroy(entries);
  printf("... done (%d hit(s) found)\n", hits);
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
/*  if (filter_stream) {
    struct encoding* utf8 = encoding_utf8_create();
    struct range_tree_node* root = encoding_transform_stream(*filter_stream, filter_encoding, utf8);
    encoding_utf8_destroy(utf8);
    char* subpath = (char*)range_tree_raw(root, 0, range_tree_length(root));
    range_tree_destroy(root, NULL);
    fprintf(stderr, "%s\r\n", subpath);
  }*/

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
    if (!search || search_find(search, &text_stream, NULL)) {
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
    document_file_insert(file, range_tree_length(file->buffer), (uint8_t*)predefined, strlen(predefined), TIPPSE_INSERTER_NOFUSE|document_directory_highlight(combined));
    free(combined);
  }

  for (size_t n = 0; n<files->count; n++) {
    char* combined = combine_path_file(file->filename, sort[n]);
    size_t length = strlen(sort[n]);

    struct stream text_stream;
    stream_from_plain(&text_stream, (uint8_t*)sort[n], length);
    if (!search || search_find(search, &text_stream, NULL)) {
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
  if (file->buffer) {
    document_file_insert_utf8(file, range_tree_length(file->buffer), "\n", 1, TIPPSE_INSERTER_NOFUSE);
  }

  inserter |= TIPPSE_INSERTER_NOFUSE;
  if (search) {
    size_t start = (size_t)stream_offset(&search->hit_start);
    size_t end = (size_t)stream_offset(&search->hit_end);
    document_file_insert(file, range_tree_length(file->buffer), (uint8_t*)&output[0], start, inserter);
    document_file_insert(file, range_tree_length(file->buffer), (uint8_t*)&output[start], end-start, TIPPSE_INSERTER_NOFUSE|TIPPSE_INSERTER_HIGHLIGHT|(VISUAL_FLAG_COLOR_STRING<<TIPPSE_INSERTER_HIGHLIGHT_COLOR_SHIFT));
    document_file_insert(file, range_tree_length(file->buffer), (uint8_t*)&output[end], length-end, inserter);
  } else {
    document_file_insert(file, range_tree_length(file->buffer), (uint8_t*)&output[0],  length, inserter);
  }
}

// Select whole document
void document_select_all(struct document_file* file, struct document_view* view, int update_offset) {
  file_offset_t end = range_tree_length(file->buffer);
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
  struct range_tree_node* copy = NULL;
  while (document_view_select_next(view, high, &low, &high)) {
    copy = range_tree_copy_insert(file->buffer, low, copy, range_tree_length(copy), high-low, NULL);
  }
  clipboard_set(copy, file->binary, file->encoding);
}

// Paste selection
void document_clipboard_paste(struct document_file* file, struct document_view* view) {
  struct encoding* encoding = NULL;
  struct range_tree_node* buffer = clipboard_get(&encoding);
  if (buffer) {
    struct range_tree_node* transform = encoding_transform_page(buffer, 0, FILE_OFFSET_T_MAX, encoding, file->encoding);
    document_file_insert_buffer(file, view->offset, transform);
    range_tree_destroy(transform, NULL);
  }
}

// Toggle bookmark
void document_bookmark_toggle_range(struct document_file* file, file_offset_t low, file_offset_t high) {
  int marked = range_tree_marked(file->bookmarks, low, high-low, TIPPSE_INSERTER_MARK);
  document_bookmark_range(file, low, high, marked);
}

// Add range to bookmark list
void document_bookmark_range(struct document_file* file, file_offset_t low, file_offset_t high, int marked) {
  file->bookmarks = range_tree_mark(file->bookmarks, low, high-low, marked?0:TIPPSE_INSERTER_MARK|TIPPSE_INSERTER_NOFUSE);
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

// Goto next bookmark
void document_bookmark_next(struct document_file* file, struct document_view* view) {
  file_offset_t offset = view->offset;
  int wrap = 1;
  while (1) {
    file_offset_t low;
    file_offset_t high;
    if (range_tree_marked_next(file->bookmarks, offset, &low, &high, wrap)) {
      view->offset = low;
      break;
    }
    if (!wrap) {
      editor_console_update(file->editor, "No bookmark found!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
      break;
    }
    wrap = 0;
    offset = 0;
  }
}

// Goto previous bookmark
void document_bookmark_prev(struct document_file* file, struct document_view* view) {
  file_offset_t offset = view->offset;
  int wrap = 1;
  while (1) {
    file_offset_t low;
    file_offset_t high;
    if (range_tree_marked_prev(file->bookmarks, offset, &low, &high, wrap)) {
      view->offset = low;
      break;
    }
    if (!wrap) {
      editor_console_update(file->editor, "No bookmark found!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
      break;
    }
    wrap = 0;
    offset = range_tree_length(file->buffer);
  }
}
