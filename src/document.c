// Tippse - Document - Interface for different view types and common meta document functions

#include "document.h"

// Search in partial document
int document_compare(struct range_tree_node* left, file_offset_t displacement_left, struct range_tree_node* right_root, file_offset_t length) {
  struct range_tree_node* right = right_root;
  size_t displacement_right = 0;

  struct range_tree_node* reload = NULL;
  int found = 0;

  while (left && right) {
    if (displacement_left>=left->length || !left->buffer) {
      if (!reload) {
        reload = left;
      }

      left = range_tree_next(left);
      if (left && left->buffer) {
        fragment_cache(left->buffer);
      }

      displacement_left = 0;
      continue;
    }

    if (displacement_right>=right->length || !right->buffer) {
      right = range_tree_next(right);
      displacement_right = 0;
      continue;
    }

    uint8_t* text_left = left->buffer->buffer+left->offset+displacement_left;
    uint8_t* text_right = right->buffer->buffer+right->offset+displacement_right;
    file_offset_t max = length;
    if (left->length-displacement_left<max) {
      max = left->length-displacement_left;
    }

    if (right->length-displacement_right<max) {
      max = right->length-displacement_right;
    }

    if (strncmp((char*)text_left, (char*)text_right, max)!=0) {
      break;
    }

    displacement_left += max;
    displacement_right += max;
    length-= max;
    if (length==0) {
      found = 1;
      break;
    }
  }

  if (reload && reload->buffer) {
    fragment_cache(reload->buffer);
  }

  return found;
}

// Search in document
int document_search(struct splitter* splitter, struct range_tree_node* search_text, struct range_tree_node* replace_text, int forward, int all, int replace) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;

  if (!search_text || !file->buffer) {
    ((struct document_text*)splitter->document_text)->keep_status = 1;
    splitter_status(splitter, "No text to search for!", 1);
    return 0;
  }

  file_offset_t search_length = search_text->length;
  file_offset_t replace_length = replace_text?replace_text->length:0;

  if (search_length>file->buffer->length) {
    ((struct document_text*)splitter->document_text)->keep_status = 1;
    splitter_status(splitter, "Not found!", 1);
    return 0;
  }

  search_text = range_tree_first(search_text);

  file_offset_t offset = view->offset;

  int found = 0;
  file_offset_t replacements = 0;
  while (1) {
    found = 0;
    file_offset_t skip = 0;
    if (replace) {
      file_offset_t offset_low = view->selection_low;
      if (view->selection_low!=FILE_OFFSET_T_MAX) {
        if (replacements==0) {
          document_undo_chain(file, file->undos);
        }

        replacements++;

        document_file_delete_selection(splitter->file, splitter->view);
        document_file_insert_buffer(splitter->file, view->offset, replace_text);

        offset = (!forward)?offset_low:(offset_low+replace_length);
        skip = (!forward)?search_length:0;
      }
    }

    if (!file->buffer || search_length>file->buffer->length) {
      break;
    }

    if (forward) {
      if (view->selection_low!=FILE_OFFSET_T_MAX) {
        offset = view->selection_high;
      }
    } else {
      if (view->selection_low!=FILE_OFFSET_T_MAX) {
        offset = view->selection_low;
        skip = search_length;
      }
    }

    if (offset<skip) {
      offset = file->buffer->length-skip;
    } else {
      offset -= skip;
    }

    file_offset_t pos = offset;
    file_offset_t displacement;
    struct range_tree_node* buffer = range_tree_find_offset(file->buffer, offset, &displacement);
    fragment_cache(buffer->buffer);

    int wrap = 0;
    if (forward) {
      while (pos<offset || !wrap) {
        if (displacement>=buffer->length || !buffer->buffer) {
          buffer = range_tree_next(buffer);
          if (!buffer) {
            buffer = range_tree_first(file->buffer);
            pos = 0;
            wrap = 1;
          }

          if (buffer && buffer->buffer) {
            fragment_cache(buffer->buffer);
          }

          displacement = 0;
          continue;
        }

        if (document_compare(buffer, displacement, search_text, search_length)) {
          found = 1;
          break;
        }

        displacement++;
        pos++;
      }
    } else {
      while (pos>offset || !wrap) {
        if (displacement==FILE_OFFSET_T_MAX || !buffer->buffer) {
          buffer = range_tree_prev(buffer);
          if (!buffer) {
            buffer = range_tree_last(file->buffer);
            pos = file->buffer->length-1;
            wrap = 1;
          }

          if (buffer && buffer->buffer) {
            fragment_cache(buffer->buffer);
          }

          displacement = buffer->length-1;
          continue;
        }

        if (document_compare(buffer, displacement, search_text, search_length)) {
          found = 1;
          break;
        }

        displacement--;
        pos--;
      }
    }

    if (!found) {
      break;
    }

    view->offset = pos;
    view->selection_low = pos;
    view->selection_high = pos+search_length;
    view->selection_start = pos;
    view->selection_end = pos+search_length;
    if (!all) {
      break;
    }
  }

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
void document_directory(struct document_file* file) {
  DIR* directory = opendir(file->filename);
  if (directory) {
    struct list* files = list_create();
    while (1) {
      struct dirent* entry = readdir(directory);
      if (!entry) {
        break;
      }

      list_insert(files, NULL, strdup(&entry->d_name[0]));
    }

    closedir(directory);

    char** sort1 = malloc(sizeof(char*)*files->count);
    char** sort2 = malloc(sizeof(char*)*files->count);
    struct list_node* name = files->first;
    for (size_t n = 0; n<files->count && name; n++) {
      sort1[n] = (char*)name->object;
      name = name->next;
    }

    char** sort = merge_sort(sort1, sort2, files->count);

    file->buffer = range_tree_delete(file->buffer, 0, file->buffer?file->buffer->length:0, TIPPSE_INSERTER_AUTO);

    for (size_t n = 0; n<files->count; n++) {
      if (file->buffer) {
        file->buffer = range_tree_insert_split(file->buffer, file->buffer?file->buffer->length:0, (uint8_t*)"\n", 1, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      }

      file->buffer = range_tree_insert_split(file->buffer, file->buffer?file->buffer->length:0, (uint8_t*)sort[n], strlen(sort[n]), TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
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
