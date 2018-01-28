// Tippse - Document - Interface for different view types and common meta document functions

#include "document.h"

// Search in document
int document_search(struct splitter* splitter, struct range_tree_node* search_text, struct encoding* search_encoding, struct range_tree_node* replace_text, struct encoding* replace_encoding, int reverse, int ignore_case, int regex, int all, int replace) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (!search_text || !file->buffer) {
    editor_console_update(file->editor, "No text to search for!", SIZE_T_MAX, CONSOLE_TYPE_NORMAL);
    return 0;
  }

  struct stream needle_stream;
  stream_from_page(&needle_stream, range_tree_first(search_text), 0);
  struct search* search;
  if (regex) {
    search = search_create_regex(ignore_case, reverse, needle_stream, search_encoding, file->encoding);
  } else {
    search = search_create_plain(ignore_case, reverse, needle_stream, search_encoding, file->encoding);
  }

  file_offset_t offset = view->offset;
  file_offset_t begin = 0;
  file_offset_t replacements = 0;
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
      struct range_tree_node* replacement = regex?search_replacement(search, replace_text, replace_encoding, file->buffer):replace_text;
      document_file_reduce(&begin, selection_low, selection_high-selection_low);
      document_file_delete_selection(splitter->file, splitter->view);

      file_offset_t start = view->offset;
      file_offset_t end = start;
      if (replacement) {
        if (begin!=view->offset) {
          document_file_expand(&begin, view->offset, replacement->length);
        }
        document_file_insert_buffer(splitter->file, view->offset, replacement);
        end = view->offset;

        if (regex) {
          range_tree_destroy(replacement, NULL);
        }
      }

      document_view_select_nothing(view, file);
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
        if (((!reverse && start>=offset) || (reverse && end<=offset)) && end-start<=left) {
          view->offset = reverse?start:end;

          document_view_select_nothing(view, file);
          document_view_select_range(view, start, end, TIPPSE_INSERTER_MARK|TIPPSE_INSERTER_NOFUSE);
          selection_low = start;
          selection_high = end;
          found = 1;
          break;
        }
      }
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

  if (replacements>0) {
    document_undo_chain(file, file->undos);
  }

  if (replace) {
    char status[1024];
    sprintf(&status[0], "%d replacements", (int)replacements);
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
void document_search_directory(const char* path, struct range_tree_node* search_text, struct encoding* search_encoding, struct range_tree_node* replace_text, struct encoding* replace_encoding, int ignore_case, int regex, int replace) {
  printf("Scanning %s...\n", path);
  struct list* entries = list_create(sizeof(char*));
  char* copy = strdup(path);
  list_insert(entries, entries->last, &copy);

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
      struct stream needle_stream;
      stream_from_page(&needle_stream, range_tree_first(search_text), 0);
      struct document_file* file = document_file_create(0, 0, NULL);
      struct file_cache* cache = file_cache_create(scan);
      struct stream stream;
      stream_from_file(&stream, cache, 0);
      document_file_detect_properties_stream(file, &stream);
      struct search* search;
      if (regex) {
        search = search_create_regex(ignore_case, 0, needle_stream, search_encoding, file->encoding);
      } else {
        search = search_create_plain(ignore_case, 0, needle_stream, search_encoding, file->encoding);
      }

      file_offset_t line = 1;
      struct stream newlines = stream;
      file_offset_t line_hit = line;
      struct stream line_start = newlines;
      while (!stream_end(&stream)) {
        stream_forward_oob(&stream, 0);
        int found = search_find(search, &stream, NULL);
        if (found) {
          file_offset_t hit = stream_offset_file(&search->hit_start);
          stream_forward_oob(&newlines, 0);
          while (stream_offset(&newlines)<=hit) {
            line_hit = line;
            line_start = newlines;
            while (!stream_end(&newlines) && stream_read_forward(&newlines)!='\n') {
            }
            line++;
          }
          printf("%s:%d: ", scan, (int)line_hit);

          int columns = 80;
          struct stream line_copy = line_start;
          stream_forward_oob(&line_copy, 0);
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
        }
      }

      search_destroy(search);
      file_cache_dereference(cache);
      document_file_destroy(file);
    }
    free(scan);
    list_remove(entries, insert);
  }
  list_destroy(entries);
  printf("... done\n");
}

// Read directory into document, sort by file name
void document_directory(struct document_file* file, struct stream* filter_stream, struct encoding* filter_encoding) {
  struct directory* directory = directory_create(file->filename);
  struct search* search = filter_stream?search_create_plain(1, 0, *filter_stream, filter_encoding, file->encoding):NULL;

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
  }

  directory_destroy(directory);

  if (search) {
    search_destroy(search);
  }

  char** sort1 = malloc(sizeof(char*)*files->count);
  char** sort2 = malloc(sizeof(char*)*files->count);
  struct list_node* name = files->first;
  for (size_t n = 0; n<files->count && name; n++) {
    sort1[n] = *(char**)list_object(name);
    name = name->next;
  }

  char** sort = merge_sort(sort1, sort2, files->count);

  file->buffer = range_tree_delete(file->buffer, 0, range_tree_length(file->buffer), 0, file);

  for (size_t n = 0; n<files->count; n++) {
    if (file->buffer) {
      file->buffer = range_tree_insert_split(file->buffer, range_tree_length(file->buffer), (uint8_t*)"\n", 1, 0);
    }

    file->buffer = range_tree_insert_split(file->buffer, range_tree_length(file->buffer), (uint8_t*)sort[n], strlen(sort[n]), 0);
    free(sort[n]);
  }

  free(sort2);
  free(sort1);

  while (files->first) {
    list_remove(files, files->first);
  }

  list_destroy(files);
}

// Select whole document
void document_select_all(struct splitter* splitter, int update_offset) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  file_offset_t end = range_tree_length(file->buffer);
  if (update_offset) {
    view->offset = end;
  }
  document_view_select_all(view, file);
}

// Throw away all selections
void document_select_nothing(struct splitter* splitter) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  document_view_select_nothing(view, file);
}
