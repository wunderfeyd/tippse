#ifndef __TIPPSE_DOCUMENTUNDO__
#define __TIPPSE_DOCUMENTUNDO__

#include <stdlib.h>

struct document_undo;

#include "list.h"
#include "rangetree.h"
#include "documentview.h"
#include "documentfile.h"
#include "document.h"

#define TIPPSE_UNDO_TYPE_CHAIN -1
#define TIPPSE_UNDO_TYPE_DELETE 0
#define TIPPSE_UNDO_TYPE_INSERT 1

struct document_undo {
  file_offset_t offset;
  file_offset_t length;

  file_offset_t cursor_insert;
  file_offset_t cursor_delete;
  file_offset_t selection_start;
  file_offset_t selection_end;

  file_offset_t selection_low;
  file_offset_t selection_high;

  int scroll_x;
  int scroll_y;

  int type;
  struct range_tree_node* buffer;
};

void document_undo_add(struct document_file* file, struct document_view* view, file_offset_t offset, file_offset_t length, int insert);
void document_undo_chain(struct document_file* file);
void document_undo_empty(struct list* list);
int document_undo_execute(struct document_file* file, struct document_view* view, struct list* from, struct list* to);

#endif /* #ifndef __TIPPSE_DOCUMENTUNDO__ */
