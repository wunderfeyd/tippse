// Tippse - Document undo - Manages undo and redo lists

#include "documentundo.h"

#include "document.h"
#include "documentfile.h"
#include "documentview.h"
#include "list.h"
#include "rangetree.h"

// Add an undo step
void document_undo_add(struct document_file* file, struct document_view* view, file_offset_t offset, file_offset_t length, int type) {
  if (length==0 || !file->undo) {
    return;
  }

  if ((type==TIPPSE_UNDO_TYPE_INSERT || type==TIPPSE_UNDO_TYPE_DELETE) && !file->autocomplete_rescan) {
    file->autocomplete_rescan = 1;
  }

  while (file->undos->count>TIPPSE_UNDO_MAX) {
    struct document_undo* undo = (struct document_undo*)list_object(file->undos->last);
    if (undo->buffer) {
      range_tree_destroy(undo->buffer);
    }

    list_remove(file->undos, file->undos->last);
    if (file->undo_save_point>0) {
      file->undo_save_point--;
    } else {
      file->undo_save_point = SIZE_T_MAX;
    }
  }

  struct document_undo* undo = (struct document_undo*)list_object(list_insert_empty(file->undos, NULL));
  undo->offset = offset;
  undo->length = length;
  undo->type = type;
  undo->buffer = range_tree_copy(&file->buffer, offset, length, file);
  undo->cursor_delete = offset;
  undo->cursor_insert = offset+length;
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

  struct document_undo* prev = (struct document_undo*)list_object(node);
  if (prev->type!=TIPPSE_UNDO_TYPE_CHAIN) {
    struct document_undo* undo = (struct document_undo*)list_object(list_insert_empty(list, NULL));
    undo->type = TIPPSE_UNDO_TYPE_CHAIN;
    undo->buffer = NULL;
  }
}

// Clear undo list
void document_undo_empty(struct document_file* file, struct list* list) {
  while (list->first) {
    struct document_undo* undo = (struct document_undo*)list_object(list->first);
    if (undo->buffer) {
      range_tree_destroy(undo->buffer);
    }

    list_remove(list, list->first);
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
  struct document_undo* undo = (struct document_undo*)list_object(node);
  if (undo->type==TIPPSE_UNDO_TYPE_INSERT) {
    range_tree_delete(&file->buffer, undo->offset, undo->length, 0);
    offset = undo->cursor_delete;

    document_file_reduce_all(file, undo->offset, undo->length);

    undo->type = TIPPSE_UNDO_TYPE_DELETE;
    view->offset = offset;
    chain = 1;
  } else if (undo->type==TIPPSE_UNDO_TYPE_DELETE) {
    range_tree_paste(&file->buffer, undo->buffer->root, undo->offset);
    offset = undo->cursor_insert;

    document_file_expand_all(file, undo->offset, undo->length);

    undo->type = TIPPSE_UNDO_TYPE_INSERT;
    view->offset = offset;
    chain = 1;
  }

  if (undo->type!=TIPPSE_UNDO_TYPE_CHAIN || override) {
    list_insert(to, NULL, undo);
    list_remove(from, node);
  }

  return chain;
}

// Copy tree data from invalidated cache on specific list
void document_undo_cache_invalidate_list(struct document_file* file, struct file_cache* cache, struct list* list) {
  struct list_node* node = list->first;
  while (node) {
    struct document_undo* undo = (struct document_undo*)list_object(node);
    range_tree_cache_invalidate(undo->buffer, cache);
    node = node->next;
  }
}

// File cache was invalidated copy over all trees if file was associated
void document_undo_cache_invalidate(struct document_file* file, struct file_cache* cache) {
  document_undo_cache_invalidate_list(file, cache, file->undos);
  document_undo_cache_invalidate_list(file, cache, file->redos);
}
