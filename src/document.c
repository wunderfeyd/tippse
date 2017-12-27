// Tippse - Document - Interface for different view types and common meta document functions

#include "document.h"

// Search in document
int document_search(struct splitter* splitter, struct range_tree_node* search_text, struct range_tree_node* replace_text, int reverse, int ignore_case, int regex, int all, int replace) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (!search_text || !file->buffer) {
    ((struct document_text*)splitter->document_text)->keep_status = 1;
    splitter_status(splitter, "No text to search for!", 1);
    return 0;
  }

  struct encoding_stream needle_stream;
  encoding_stream_from_page(&needle_stream, range_tree_first(search_text), 0);
  struct search* search;
  if (regex) {
    search = search_create_regex(ignore_case, reverse, needle_stream, file->encoding, file->encoding);
  } else {
    search = search_create_plain(ignore_case, reverse, needle_stream, file->encoding, file->encoding);
  }

  file_offset_t offset = view->offset;
  file_offset_t begin = 0;
  file_offset_t replacements = 0;

  int first = 1;
  int found = 0;
  while (1) {
    found = 0;
    if (replace && view->selection_low!=FILE_OFFSET_T_MAX) {
      if (replacements==0) {
        document_undo_chain(file, file->undos);
      }

      replacements++;

      document_file_reduce(&begin, view->selection_low, view->selection_high-view->selection_low);
      document_file_delete_selection(splitter->file, splitter->view);
      document_file_expand(&begin, view->offset, replace_text->length);
      document_file_insert_buffer(splitter->file, view->offset, replace_text);

      file_offset_t start = view->offset;
      file_offset_t end = start+replace_text->length;
      view->selection_low = start;
      view->selection_high = end;
      view->selection_start = start;
      view->selection_end = end;
    }

    if (!file->buffer) {
      break;
    }

    if (!reverse) {
      if (view->selection_low!=FILE_OFFSET_T_MAX) {
        offset = view->selection_high;
      }
    } else {
      if (view->selection_low!=FILE_OFFSET_T_MAX) {
        offset = view->selection_low;
        if (offset==0) {
          offset = file->buffer->length-1;
        } else {
          offset--;
        }
      }
    }

    if (offset>=file->buffer->length) {
      offset = file->buffer->length-1;
    }

    if (first) {
      begin = offset;
    }

    while (1) {
      file_offset_t displacement;
      struct range_tree_node* buffer = range_tree_find_offset(file->buffer, offset, &displacement);
      struct encoding_stream text_stream;
      encoding_stream_from_page(&text_stream, buffer, displacement);

      file_offset_t left;
      if (!reverse) {
        if (offset<begin) {
          left = begin-offset;
        } else {
          left = file->buffer->length;
        }
      } else {
        if (offset>begin) {
          left = offset-begin;
        } else {
          left = file->buffer->length;
        }
      }

      while (1) {
        int hit = search_find(search, &text_stream, &left);
        if (!hit) {
          break;
        }
        file_offset_t start = range_tree_offset(search->hit_start.buffer)+search->hit_start.displacement;
        file_offset_t end = range_tree_offset(search->hit_end.buffer)+search->hit_end.displacement;
        if ((!reverse && start>=offset) || (reverse && end<=offset)) {
          view->offset = start;
          view->selection_low = start;
          view->selection_high = end;
          view->selection_start = start;
          view->selection_end = end;
          found = 1;
          break;
        }
      }

      if (found) {
        break;
      }

      if (!reverse) {
        if (offset==0) {
          break;
        }
        offset = 0;
      } else {
        if (offset==file->buffer->length-1) {
          break;
        }
        offset = file->buffer->length-1;
      }
    }

    first = 0;
    if (!found || !all) {
      break;
    }
  }

  search_destroy(search);

  if (replacements>0) {
    document_undo_chain(file, file->undos);
  }

  ((struct document_text*)splitter->document_text)->keep_status = 1;
  if (replace) {
    char status[1024];
    sprintf(&status[0], "%d replacements", (int)replacements);
    splitter_status(splitter, &status[0], 1);
    return 1;
  }

  if (found) {
    return 1;
  }

  splitter_status(splitter, "Not found!", 1);
  return 0;
}


// Read directory into document, sort by file name
void document_directory(struct document_file* file, struct encoding_stream* filter_stream, struct encoding* filter_encoding) {
  DIR* directory = opendir(file->filename);
  if (directory) {
    struct search* search = filter_stream?search_create_plain(1, 0, *filter_stream, filter_encoding, file->encoding):NULL;

    struct list* files = list_create(sizeof(char*));
    while (1) {
      struct dirent* entry = readdir(directory);
      if (!entry) {
        break;
      }

      struct encoding_stream text_stream;
      encoding_stream_from_plain(&text_stream, (uint8_t*)&entry->d_name[0], strlen(&entry->d_name[0]));
      if (!search || search_find(search, &text_stream, NULL)) {
        char* name = strdup(&entry->d_name[0]);
        list_insert(files, NULL, &name);
      }
    }

    closedir(directory);

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

    file->buffer = range_tree_delete(file->buffer, 0, file->buffer?file->buffer->length:0, 0, file);

    for (size_t n = 0; n<files->count; n++) {
      if (file->buffer) {
        file->buffer = range_tree_insert_split(file->buffer, file->buffer?file->buffer->length:0, (uint8_t*)"\n", 1, 0, NULL);
      }

      file->buffer = range_tree_insert_split(file->buffer, file->buffer?file->buffer->length:0, (uint8_t*)sort[n], strlen(sort[n]), 0, NULL);
      free(sort[n]);
    }

    free(sort2);
    free(sort1);

    while (files->first) {
      list_remove(files, files->first);
    }

    list_destroy(files);
  }
}
