/* Tippse - Document undo - Manages undo and redo lists */

#include "documentundo.h"

void document_undo_add(struct document_file* file, struct document_view* view, file_offset_t offset, file_offset_t length, int insert) {
  if (length==0) {
    return;
  }

  struct document_undo* undo = (struct document_undo*)malloc(sizeof(struct document_undo));
  undo->offset = offset;
  undo->length = length;
  undo->insert = insert;
  undo->buffer = range_tree_copy(file->buffer, offset, length);
  undo->cursor_delete = offset;
  undo->cursor_insert = offset+length;
/*  undo->selection_start = view->selection_start;
  undo->selection_end = view->selection_end;
  undo->selection_low = view->selection_low;
  undo->selection_high = view->selection_high;
  undo->scroll_x = view->scroll_x;
  undo->scroll_y = view->scroll_y;*/

  list_insert(file->undos, NULL, undo);
}


void document_undo_empty(struct list* list) {
  while (1) {
    struct list_node* node = list->first;
    if (!node) {
      break;
    }

    struct document_undo* undo = (struct document_undo*)node->object;
    range_tree_clear(undo->buffer);
    list_remove(list, node);
  }
}

void document_undo_execute(struct document_file* file, struct document_view* view, struct list* from, struct list* to) {
  struct list_node* node = from->first;
  if (!node) {
    return;
  }

  file_offset_t offset = 0;
  struct document_undo* undo = (struct document_undo*)node->object;
  if (undo->insert) {
    file->buffer = range_tree_delete(file->buffer, undo->offset, undo->length, 0);
    offset = undo->cursor_delete;

    struct list_node* views = file->views->first;
    while (views) {
      struct document_view* view = (struct document_view*)views->object;
      document_reduce(&view->selection_end, undo->offset, undo->length);
      document_reduce(&view->selection_start, undo->offset, undo->length);
      document_reduce(&view->selection_low, undo->offset, undo->length);
      document_reduce(&view->selection_high, undo->offset, undo->length);
      document_reduce(&view->offset, undo->offset, undo->length);
      
      views = views->next;
    }
  } else {
    file->buffer = range_tree_paste(file->buffer, undo->buffer, undo->offset);
    offset = undo->cursor_insert;

    struct list_node* views = file->views->first;
    while (views) {
      struct document_view* view = (struct document_view*)views->object;
      document_expand(&view->selection_end, undo->offset, undo->length);
      document_expand(&view->selection_start, undo->offset, undo->length);
      document_expand(&view->selection_low, undo->offset, undo->length);
      document_expand(&view->selection_high, undo->offset, undo->length);
      document_expand(&view->offset, undo->offset, undo->length);
      
      views = views->next;
    }
  }

  view->offset = offset;

/*  view->selection_start = undo->selection_start;
  view->selection_end = undo->selection_end;
  view->selection_low = undo->selection_low;
  view->selection_high = undo->selection_high;
  view->scroll_x = undo->scroll_x;
  view->scroll_y = undo->scroll_y;*/

  list_insert(to, NULL, undo);
  undo->insert = undo->insert^1;
  list_remove(from, node);
}

