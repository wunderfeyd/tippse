/* Tippse - Document - Interface for different view types and common meta document functions */

#include "document.h"

int document_compare(struct range_tree_node* left, file_offset_t displacement_left, struct range_tree_node* right_root, file_offset_t length) {
  struct range_tree_node* right = right_root;
  size_t displacement_right = 0;

  while (left && right) {
    if (displacement_left>=left->length || !left->buffer) {
      left = range_tree_next(left);
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
      return 0;
    }

    displacement_left += max;
    displacement_right += max;
    length-= max;
    if (length==0) {
      return 1;
    }
  }

  return 0;
}

void document_search(struct splitter* splitter, struct range_tree_node* text, file_offset_t length, int forward) {
  struct document_file* file = splitter->file;
  struct document_view* view = splitter->view;
  if (!text || !file->buffer || file->buffer->length==0) {
    ((struct document_text*)splitter->document_text)->keep_status = 1;
    splitter_status(splitter, "No text to search for!", 1);
    return;
  }

  if (length>file->buffer->length) {
    ((struct document_text*)splitter->document_text)->keep_status = 1;
    splitter_status(splitter, "Not found!", 1);
    return;
  }

  file_offset_t offset = view->offset;
  if (forward) {
    if (view->selection_low!=~0) {
      offset = view->selection_high;
    }
  } else {
    offset--;
    if (view->selection_low!=~0) {
      offset = view->selection_low-1;
    }
    if (offset==~0) {
      offset = file->buffer->length-1;
    }
  }

  file_offset_t pos = offset;
  file_offset_t displacement;
  struct range_tree_node* buffer = range_tree_find_offset(file->buffer, offset, &displacement);
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

        displacement = 0;
        continue;
      }

      if (document_compare(buffer, displacement, text, length)) {
        view->offset = pos;
        view->selection_low = pos;
        view->selection_high = pos+length;
        view->selection_start = pos;
        view->selection_end = pos+length;
        return;
      }

      displacement++;
      pos++;
    }
  } else {
    while (pos>offset || !wrap) {
      if (displacement==~0 || !buffer->buffer) {
        buffer = range_tree_prev(buffer);
        if (!buffer) {
          buffer = range_tree_last(file->buffer);
          pos = file->buffer->length-1;
          wrap = 1;
        }

        displacement = buffer->length-1;
        continue;
      }

      if (document_compare(buffer, displacement, text, length)) {
        view->offset = pos;
        view->selection_low = pos;
        view->selection_high = pos+length;
        view->selection_start = pos;
        view->selection_end = pos+length;
        return;
      }

      displacement--;
      pos--;
    }
  }

  ((struct document_text*)splitter->document_text)->keep_status = 1;
  splitter_status(splitter, "Not found!", 1);
}

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
    size_t n;
    for (n = 0; n<files->count && name; n++) {
      sort1[n] = (char*)name->object;
      name = name->next;
    }

    char** sort = merge_sort(sort1, sort2, files->count);

    file->buffer = range_tree_delete(file->buffer, 0, file->buffer?file->buffer->length:0, TIPPSE_INSERTER_AUTO);

    for (n = 0; n<files->count; n++) {
      file->buffer = range_tree_insert_split(file->buffer, file->buffer?file->buffer->length:0, (uint8_t*)sort[n], strlen(sort[n]), TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      file->buffer = range_tree_insert_split(file->buffer, file->buffer?file->buffer->length:0, (uint8_t*)"\n", 1, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);

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
