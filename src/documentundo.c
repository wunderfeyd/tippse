// Tippse - Document undo - Manages undo and redo lists

#include "documentundo.h"

// Add an undo step
void document_undo_add(struct document_file* file, struct document_view* view, file_offset_t offset, file_offset_t length, int type) {
  if (length==0) {
    return;
  }

  struct document_undo* undo = (struct document_undo*)malloc(sizeof(struct document_undo));
  undo->offset = offset;
  undo->length = length;
  undo->type = type;
  undo->buffer = range_tree_copy(file->buffer, offset, length);
  undo->cursor_delete = offset;
  undo->cursor_insert = offset+length;

  list_insert(file->undos, NULL, undo);
}

// Check if document modified
int document_undo_modified(struct document_file* file) {
   return (file->undo_save_point!=file->undos->count)?1:0;
}

// Set last save point
void document_undo_mark_save_point(struct document_file* file) {
  document_undo_chain(file, file->undos);
  file->undo_save_point = file->undos->count;
}

// Set last save point as invalid
void document_undo_check_save_point(struct document_file* file) {
  if (file->undos->count+file->redos->count<file->undo_save_point) {
    file->undo_save_point = SIZE_T_MAX;
  }
}

// Set marker for a chain of undo steps
void document_undo_chain(struct document_file* file, struct list* list) {
  struct list_node* node = list->first;
  if (!node) {
    return;
  }

  struct document_undo* prev = (struct document_undo*)node->object;
  if (prev->type!=TIPPSE_UNDO_TYPE_CHAIN) {
    struct document_undo* undo = (struct document_undo*)malloc(sizeof(struct document_undo));
    undo->type = TIPPSE_UNDO_TYPE_CHAIN;
    undo->buffer = NULL;

    list_insert(list, NULL, undo);
  }
}

// Clear undo list
void document_undo_empty(struct document_file* file, struct list* list) {
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

  document_undo_check_save_point(file);
}

// Execute a chain of undo steps
void document_undo_execute_chain(struct document_file* file, struct document_view* view, struct list* from, struct list* to, int reverse) {
  document_undo_execute(file, view, from, to, 1);
  while (document_undo_execute(file, view, from, to, reverse)) {}
}

// Execute an undo steps
int document_undo_execute(struct document_file* file, struct document_view* view, struct list* from, struct list* to, int override) {
  int chain = 0;
  struct list_node* node = from->first;
  if (!node) {
    return chain;
  }

  file_offset_t offset = 0;
  struct document_undo* undo = (struct document_undo*)node->object;
  if (undo->type==TIPPSE_UNDO_TYPE_INSERT) {
    file->buffer = range_tree_delete(file->buffer, undo->offset, undo->length, 0);
    offset = undo->cursor_delete;

    document_file_reduce_all(file, undo->offset, undo->length);

    undo->type = TIPPSE_UNDO_TYPE_DELETE;
    view->offset = offset;
    chain = 1;
  } else if (undo->type==TIPPSE_UNDO_TYPE_DELETE) {
    file->buffer = range_tree_paste(file->buffer, undo->buffer, undo->offset);
    offset = undo->cursor_insert;

    document_file_expand_all(file, undo->offset, undo->length);

    undo->type = TIPPSE_UNDO_TYPE_INSERT;
    chain = 1;
  }

  if (chain==1) {
    view->offset = offset;
  }

  if (undo->type!=TIPPSE_UNDO_TYPE_CHAIN || override) {
    list_insert(to, NULL, undo);
    list_remove(from, node);
  }

  return chain;
}

