/* Tippse - Document undo - Manages undo and redo lists */

#include "documentundo.h"

void document_undo_add(struct document_file* file, struct document_view* view, file_offset_t offset, file_offset_t length, int type) {
  if (length==0) {
    return;
  }

  struct document_undo* undo = (struct document_undo*)malloc(sizeof(struct document_undo));
  undo->offset = offset;
  undo->length = length;
  undo->type = type;
  undo->buffer = range_tree_copy(file->buffer, file->type, offset, length);
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

void document_undo_chain(struct document_file* file) {
  struct list_node* node = file->undos->first;
  if (!node) {
    return;
  }

  struct document_undo* prev = (struct document_undo*)node->object;
  if (prev->type!=TIPPSE_UNDO_TYPE_CHAIN) {
    struct document_undo* undo = (struct document_undo*)malloc(sizeof(struct document_undo));
    undo->type = TIPPSE_UNDO_TYPE_CHAIN;
    undo->buffer = NULL;

    list_insert(file->undos, NULL, undo);
  }
}

void document_undo_empty(struct list* list) {
  while (1) {
    struct list_node* node = list->first;
    if (!node) {
      break;
    }

    struct document_undo* undo = (struct document_undo*)node->object;
    if (undo->buffer) {
      range_tree_destroy(undo->buffer);
    }

    free(undo);
    list_remove(list, node);
  }
}

int document_undo_execute(struct document_file* file, struct document_view* view, struct list* from, struct list* to) {
  int chain = 0;
  struct list_node* node = from->first;
  if (!node) {
    return chain;
  }

  file->modified = 1;
  file_offset_t offset = 0;
  struct document_undo* undo = (struct document_undo*)node->object;
  if (undo->type==TIPPSE_UNDO_TYPE_INSERT) {
    file->buffer = range_tree_delete(file->buffer, file->type, undo->offset, undo->length, 0);
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

    undo->type = TIPPSE_UNDO_TYPE_DELETE;
    view->offset = offset;
    chain = 1;
  } else if (undo->type==TIPPSE_UNDO_TYPE_DELETE) {
    file->buffer = range_tree_paste(file->buffer, file->type, undo->buffer, undo->offset);
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

    undo->type = TIPPSE_UNDO_TYPE_INSERT;
    chain = 1;
  }

  if (chain==1) {
    view->offset = offset;

/*  view->selection_start = undo->selection_start;
    view->selection_end = undo->selection_end;
    view->selection_low = undo->selection_low;
    view->selection_high = undo->selection_high;
    view->scroll_x = undo->scroll_x;
    view->scroll_y = undo->scroll_y;*/
  }

  list_insert(to, NULL, undo);
  list_remove(from, node);

  return chain;
}

