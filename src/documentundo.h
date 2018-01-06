#ifndef TIPPSE_DOCUMENTUNDO_H
#define TIPPSE_DOCUMENTUNDO_H

#include <stdlib.h>
#include "types.h"

struct document_undo;
struct range_tree_node;

#define TIPPSE_UNDO_TYPE_CHAIN -1
#define TIPPSE_UNDO_TYPE_DELETE 0
#define TIPPSE_UNDO_TYPE_INSERT 1

#define TIPPSE_UNDO_MAX 5000

struct document_undo {
  file_offset_t offset;             // offset of change
  file_offset_t length;             // length of change

  file_offset_t cursor_insert;      // cursor offset after insert
  file_offset_t cursor_delete;      // cursor offset after delete

  int type;                         // type of change, delete or insert
  struct range_tree_node* buffer;   // refence to document buffer, corresponding page in tree
};

#include "list.h"
#include "rangetree.h"
#include "documentview.h"
#include "documentfile.h"
#include "document.h"

void document_undo_add(struct document_file* file, struct document_view* view, file_offset_t offset, file_offset_t length, int insert);
void document_undo_mark_save_point(struct document_file* file);
void document_undo_check_save_point(struct document_file* file);
int document_undo_modified(struct document_file* file);
void document_undo_chain(struct document_file* file, struct list* list);
void document_undo_empty(struct document_file* file, struct list* list);
void document_undo_execute_chain(struct document_file* file, struct document_view* view, struct list* from, struct list* to, int reverse);
int document_undo_execute(struct document_file* file, struct document_view* view, struct list* from, struct list* to, int override);

#endif /* #ifndef TIPPSE_DOCUMENTUNDO_H */
