/* Tippse - Range tree - For fast access to various attributes of the document */

#include "rangetree.h"

// Debug: Recursively print tree nodes
void range_tree_print(struct range_tree_node* node, int depth, int side) {
  int tab = depth;
  while (tab>0) {
    printf("  ");
    tab--;
  }

  printf ("%d %5d %5d %s(%p-%p) %5d %5d (%p) [0](%p) [1](%p) [P](%p) [U](%p) [d](%p) %d %d %d ", side, (int)node->length, (int)node->visuals.lines, node->buffer?"B":" ", node->buffer, node->buffer?node->buffer->buffer:NULL, (int)node->offset, node->depth, node, node->side[0], node->side[1], node->parent, node->next, node->prev, (int)node->visuals.ys, (int)node->visuals.xs, (int)node->visuals.dirty);
  printf("\r\n");
  if (node->side[0]) {
    range_tree_print(node->side[0], depth+1, 0);
  }

  if (node->side[1]) {
    range_tree_print(node->side[1], depth+1, 1);
  }
}

// Debug: Check tree for consistency (TODO: Next/Prev fields are not covered)
void range_tree_check(struct range_tree_node* node) {
  if (!node) {
    return;
  }

  if (node->side[0] && node->side[0]->parent!=node) {
    printf("children 0 has wrong parent %p -> %p should %p\r\n", node->side[0], node->side[0]->parent, node);
  }

  if (node->side[1] && node->side[1]->parent!=node) {
    printf("children 1 has wrong parent %p -> %p should %p\r\n", node->side[1], node->side[1]->parent, node);
  }

  if ((node->side[0] && !node->side[1]) || (node->side[1] && !node->side[0])) {
    printf("unbalanced node %p: %p %p\r\n", node, node->side[0], node->side[1]);
  }

  if (node->buffer && (node->side[0] || node->side[1])) {
    printf("Leaf with children %p: %p %p\r\n", node, node->side[0], node->side[1]);
  }

  if (node->side[0]) {
    range_tree_check(node->side[0]);
  }

  if (node->side[1]) {
    range_tree_check(node->side[1]);
  }
}

// Debug: Search root and print
void range_tree_print_root(struct range_tree_node* node, int depth, int side) {
  while(node->parent) {
    node = node->parent;
  }

  range_tree_print(node, depth, side);
}

// Combine statistics of child nodes
void range_tree_update_calc(struct range_tree_node* node) {
  if (!node->buffer) {
    node->length = node->side[0]->length+node->side[1]->length;
    node->depth = ((node->side[0]->depth>node->side[1]->depth)?node->side[0]->depth:node->side[1]->depth)+1;
    node->inserter = (node->side[0]?node->side[0]->inserter:0)&(node->side[1]?node->side[1]->inserter:0);
    visual_info_combine(&node->visuals, &node->side[0]->visuals, &node->side[1]->visuals);
  }

  node->subs = (node->side[0]?node->side[0]->subs+1:0)+(node->side[1]?node->side[1]->subs+1:0);
}

// Recursively recalc all nodes on path up to the root node
void range_tree_update_calc_all(struct range_tree_node* node) {
  range_tree_update_calc(node);
  if (node->parent) {
    range_tree_update_calc_all(node->parent);
  }
}

// Remove node and all children
void range_tree_destroy(struct range_tree_node* node) {
  if (node->side[0]) {
    range_tree_destroy(node->side[0]);
  }

  if (node->side[1]) {
    range_tree_destroy(node->side[1]);
  }

  if (node->buffer) {
    fragment_dereference(node->buffer);
  }

  free(node);
}

// Create node wih given fragment
struct range_tree_node* range_tree_create(struct range_tree_node* parent, struct range_tree_node* side0, struct range_tree_node* side1, struct fragment* buffer, file_offset_t offset, file_offset_t length, int inserter) {
  struct range_tree_node* node = malloc(sizeof(struct range_tree_node));
  node->parent = parent;
  node->side[0] = side0;
  node->side[1] = side1;
  node->buffer = buffer;
  node->next = NULL;
  node->prev = NULL;
  if (node->buffer) {
    fragment_reference(node->buffer);
  }

  node->offset = offset;
  node->length = length;
  node->inserter = inserter;
  visual_info_clear(&node->visuals);
  range_tree_update_calc(node);
  return node;
}

// Find first leaf
struct range_tree_node* range_tree_first(struct range_tree_node* node) {
  if (!node) {
    return NULL;
  }

  while (!node->buffer) {
    node = node->side[0];
  }

  return node;
}

// Find last leaf
struct range_tree_node* range_tree_last(struct range_tree_node* node) {
  if (!node) {
    return NULL;
  }

  while (!node->buffer) {
    node = node->side[1];
  }

  return node;
}

// Exchange child node to another one
void range_tree_exchange(struct range_tree_node* node, struct range_tree_node* old, struct range_tree_node* new) {
  if (!node) {
    return;
  }

  if (node->side[0]==old) {
    node->side[0] = new;
  } else if (node->side[1]==old) {
    node->side[1] = new;
  } else {
    printf("umm ... dead parent %p old %p new %p side 0 %p side 1 %p\r\n", node, old, new, node->side[0], node->side[1]);
  }
}

// Rotate children left or right depending on side
struct range_tree_node* range_tree_rotate(struct range_tree_node* node, int side) {
  struct range_tree_node* parent = node->parent;
  struct range_tree_node* child = node->side[side];
  //struct range_tree_node* redo = node;
  child->parent = parent;
  node->side[side] = child->side[side^1];
  child->side[side^1]->parent = node;
  child->side[side^1] = node;
  node->parent = child;
  range_tree_exchange(parent, node, child);
  range_tree_update_calc(node);
  node = child;
  range_tree_update_calc(node);
  //range_tree_balance(redo);

  return node;
}

// Balance tree while left side must not have more depth (+1) than the right side
struct range_tree_node* range_tree_balance(struct range_tree_node* node) {
  while (node->side[0]->depth-1>node->side[1]->depth && !node->side[0]->buffer) {
    node = range_tree_rotate(node, 0);
  }

  while (node->side[0]->depth<node->side[1]->depth && !node->side[1]->buffer) {
    node = range_tree_rotate(node, 1);
  }

  return node;
}

// Update path to root node, throw away useless nodes and call the balancer (root node might change)
struct range_tree_node* range_tree_update(struct range_tree_node* node) {
  struct range_tree_node* last = node;
  while (node) {
    last = node;
    if (node->buffer) {
      node->depth = 1;
      node = node->parent;
      continue;
    }

    if (!node->side[0] && !node->side[1]) {
      struct range_tree_node* parent = node->parent;
      if (parent) {
        range_tree_exchange(parent, node, NULL);
      }

      if (node->prev) {
        node->prev->next = node->next;
      }

      if (node->next) {
        node->next->prev = node->prev;
      }

      free(node);
      last = parent;
      node = parent;

      continue;
    }

    if (!node->side[0] || !node->side[1]) {
      struct range_tree_node* parent = node->parent;
      struct range_tree_node* child = node->side[0]?node->side[0]:node->side[1];

      last = child;
      if (parent) {
        range_tree_exchange(parent, node, child);
      }

      child->parent = parent;

      free(node);
      node = parent;
      continue;
    }

    range_tree_update_calc(node);

    node = range_tree_balance(node);

    last = node;
    node = node->parent;
  }

  return last;
}

// Find first non dirty node before or at specified visualisation attributes
struct range_tree_node* range_tree_find_visual(struct range_tree_node* node, int find_type, file_offset_t find_offset, int find_x, int find_y, int find_line, int find_column, file_offset_t* offset, int* x, int* y, int* line, int* column, int* indentation, int* indentation_extra, file_offset_t* character) {
  file_offset_t location = 0;
  int ys = 0;
  int xs = 0;
  int lines = 0;
  int columns = 0;
  file_offset_t characters = 0;
  int indentations = 0;
  int indentations_extra = 0;

  while (node && !node->buffer) {
    int columns_new = columns;
    int indentations_new = indentations;
    int indentations_extra_new = indentations_extra;
    if (node->side[0]->visuals.lines!=0) {
      columns_new = node->side[0]->visuals.columns;
      indentations_new = node->side[0]->visuals.indentation;
      indentations_extra_new = node->side[0]->visuals.indentation_extra;
    } else {
      columns_new += node->side[0]->visuals.columns;
      indentations_new += node->side[0]->visuals.indentation;
      indentations_extra_new += node->side[0]->visuals.indentation_extra;
    }

    int xs_new = xs;
    if (node->side[0]->visuals.ys!=0) {
      xs_new = node->side[0]->visuals.xs;
    } else {
      xs_new += node->side[0]->visuals.xs;
    }

    if ((node->side[0]->visuals.dirty&VISUAL_DIRTY_UPDATE) || (node->visuals.dirty&VISUAL_DIRTY_LASTSPLIT) || (find_type==VISUAL_SEEK_X_Y && (node->side[0]->visuals.ys+ys>find_y || (node->side[0]->visuals.ys+ys==find_y && indentations_new+indentations_extra_new+xs_new>find_x))) || (find_type==VISUAL_SEEK_OFFSET && location+node->side[0]->length>find_offset) || (find_type==VISUAL_SEEK_LINE_COLUMN && (node->side[0]->visuals.lines+lines>find_line || (node->side[0]->visuals.lines+lines==find_line && columns_new>find_column)))) {
      node = node->side[0];
    } else {
      location += node->side[0]->length;

      ys += node->side[0]->visuals.ys;
      lines += node->side[0]->visuals.lines;
      characters += node->side[0]->visuals.characters;

      xs = xs_new;
      columns = columns_new;
      indentations = indentations_new;
      indentations_extra = indentations_extra_new;

      node = node->side[1];
    }
  }

  if (node && node->visuals.dirty) {
    *x = 0;
    *y = 0;
    *line = 0;
    *column = 0;
    *character = 0;
    *offset = 0;
    *indentation = 0;
    *indentation_extra = 0;
    return node;
  }

  // TODO: At the end of the file we should return a NULL
/*  if (node && (node->visual.rows+rows<row || (node->visual.rows+rows==row && node->visuals.columns+columns<column))) {
    return NULL;
  }*/

  if (node && node->buffer) {
    *x = xs;
    *y = ys;
    *line = lines;
    *column = columns;
    *character = characters;
    *offset = location;
    *indentation = indentations;
    *indentation_extra = indentations_extra;
  }

  return node;
}

// Check if whitespacing stops at line end
int range_tree_find_whitespaced(struct range_tree_node* node) {
  if (node->visuals.detail_after&VISUAL_INFO_WHITESPACED_START) {
    return 1;
  }

  if (!(node->visuals.detail_after&VISUAL_INFO_WHITESPACED_COMPLETE)) {
    return 0;
  }

  while (node->parent) {
    if (node->parent->side[0]==node) {
      if (!(node->parent->side[1]->visuals.detail_after&VISUAL_INFO_WHITESPACED_COMPLETE)) {
        node = node->parent->side[1];
        break;
      }
    }

    node = node->parent;
  }

  if (!node->parent) {
    return 1;
  }

  while (!node->buffer) {
    if (!(node->side[0]->visuals.detail_after&VISUAL_INFO_WHITESPACED_COMPLETE)) {
      node = node->side[0];
    } else {
      node = node->side[1];
    }
  }

  return (node->visuals.detail_after&(VISUAL_INFO_WHITESPACED_COMPLETE|VISUAL_INFO_WHITESPACED_START))?1:0;
}

// Return base offset of given node
file_offset_t range_tree_offset(struct range_tree_node* node) {
  file_offset_t offset = 0;
  while (node->parent) {
    if (node->parent->side[1]==node) {
      offset += node->parent->side[0]->length;
    }

    node = node->parent;
  }

  return offset;
}

// Return length between end of start node and beginning of end node
file_offset_t range_tree_distance_offset(struct range_tree_node* root, struct range_tree_node* start, struct range_tree_node* end) {
  start = range_tree_next(start);
  if (!start) {
    return 0;
  }

  if (!end) {
    return root->length-range_tree_offset(start);
  } else {
    return range_tree_offset(end)-range_tree_offset(start);
  }
}

// Search node by offset and return difference to its base offset
struct range_tree_node* range_tree_find_offset(struct range_tree_node* node, file_offset_t offset, file_offset_t* diff) {
  while (node && !node->buffer) {
    if (node->side[0]->length>offset) {
      node = node->side[0];
    } else {
      offset -= node->side[0]->length;
      node = node->side[1];
    }
  }

  *diff = offset;
  return node;
}

// Reallocate the fragment if it is references only once and had become smaller due to the edit process (save memory)
void range_tree_shrink(struct range_tree_node* node, struct file_type* type) {
  if (!node || !node->buffer) {
    return;
  }

  if (node->buffer->count==1 && (node->offset!=0 || node->length!=node->buffer->length) && node->length>0) {
    if (node->buffer->type==FRAGMENT_MEMORY) {
      char* text = malloc(node->length);
      memcpy(text, node->buffer->buffer+node->offset, node->length);
      free(node->buffer->buffer);
      node->buffer->buffer = text;
      node->buffer->length = node->length;
      node->offset = 0;
    }
  }

  node->visuals.dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
}

// Try to merge range of nodes which are below the maximum page size
struct range_tree_node* range_tree_fuse(struct range_tree_node* root, struct file_type* type, struct range_tree_node* first, struct range_tree_node* last) {
  if (!first) {
    first = range_tree_first(root);
  }

  if (!last) {
    last = range_tree_last(root);
  }

  while (first!=last) {
    struct range_tree_node* next = range_tree_next(first);

    if (first->length+next->length<TREE_BLOCK_LENGTH_MIN && !(first->inserter&TIPPSE_INSERTER_READONLY) && !(next->inserter&TIPPSE_INSERTER_READONLY)) {
      if (first->length+next->length<TREE_BLOCK_LENGTH_MAX) {
        // TODO: Think about to allow merging fragments from different locations in future
        if (first->buffer->type==FRAGMENT_MEMORY && next->buffer->type==FRAGMENT_MEMORY) {
          char* copy = (char*)malloc(first->length+next->length);
          memcpy(copy, first->buffer->buffer+first->offset, first->length);
          memcpy(copy+first->length, next->buffer->buffer+next->offset, next->length);
          struct fragment* buffer = fragment_create_memory(copy, first->length+next->length);
          fragment_dereference(next->buffer);
          next->buffer = buffer;
          next->length = buffer->length;
          next->offset = 0;

          range_tree_shrink(next, type);
          range_tree_update(next);

          fragment_dereference(first->buffer);
          first->buffer = NULL;
          range_tree_shrink(first, type);
          root = range_tree_update(first);
        }
      }
    }

    first = next;
  }

  return root;
}

// Insert fragment into specific offset and eventually break older nodes into parts
struct range_tree_node* range_tree_insert(struct range_tree_node* root, struct file_type* type, file_offset_t offset, struct fragment* buffer, file_offset_t buffer_offset, file_offset_t buffer_length, int inserter) {
  struct range_tree_node* node;
  struct range_tree_node* build0;
  struct range_tree_node* build1;
  struct range_tree_node* build2;
  struct range_tree_node* build3;
  file_offset_t split = 0;

  if (root) {
    node = range_tree_find_offset(root, offset, &split);
    if (!node) {
      node = root;

      while (!node->buffer) {
        node = node->side[1];
      }

      split = node->length;
    }

    struct range_tree_node* prev = range_tree_prev(node);

    struct range_tree_node* before = NULL;
    struct range_tree_node* after = NULL;

    build1 = range_tree_create(NULL, NULL, NULL, buffer, buffer_offset, buffer_length, inserter);

    if (split==node->length && ((node->inserter&TIPPSE_INSERTER_AFTER) || (inserter&TIPPSE_INSERTER_AUTO))) {
      build0 = range_tree_create(node->parent, node, build1, NULL, 0, 0, 0);
      build1->parent = build0;
      node->parent = build0;

      build1->next = node->next;
      if (node->next) {
        node->next->prev = build1;
      }

      node->next = build1;
      build1->prev = node;

      range_tree_exchange(build0->parent, node, build0);
      range_tree_shrink(build1, type);
      range_tree_shrink(node, type);
      range_tree_update(build1);
      root = range_tree_update(node);
      before = range_tree_prev(build1);
      after = range_tree_next(node);
    } else if (split==0 && ((node->inserter&TIPPSE_INSERTER_BEFORE) || (inserter&TIPPSE_INSERTER_AUTO) || (prev && (node->inserter&TIPPSE_INSERTER_AFTER)))) {
      build0 = range_tree_create(node->parent, build1, node, NULL, 0, 0, 0);
      build1->parent = build0;
      node->parent = build0;

      build1->prev = node->prev;
      if (node->prev) {
        node->prev->next = build1;
      }

      node->prev = build1;
      build1->next = node;

      range_tree_exchange(build0->parent, node, build0);
      range_tree_shrink(build1, type);
      range_tree_shrink(node, type);
      range_tree_update(build1);
      root = range_tree_update(node);
      before = range_tree_prev(build1);
      after = range_tree_next(node);
    } else if (!(node->inserter&TIPPSE_INSERTER_READONLY) || (inserter&TIPPSE_INSERTER_AUTO)) {
      build2 = range_tree_create(NULL, NULL, NULL, node->buffer, node->offset, split, node->inserter);
      build0 = range_tree_create(NULL, build2, build1, NULL, 0, 0, 0);
      build3 = range_tree_create(node->parent, build0, node, NULL, 0, 0, 0);
      build2->parent = build0;
      build1->parent = build0;
      build0->parent = build3;
      node->parent = build3;

      build2->prev = node->prev;
      if (node->prev) {
        node->prev->next = build2;
      }

      node->prev = build1;
      build2->next = build1;
      build1->prev = build2;
      build1->next = node;

      range_tree_exchange(build3->parent, node, build3);
      node->offset += split;
      node->length -= split;

      range_tree_shrink(build2, type);
      range_tree_shrink(build1, type);
      range_tree_shrink(node, type);
      range_tree_update(build2);
      range_tree_update(build1);
      root = range_tree_update(node);
      before = range_tree_prev(build2);
      after = range_tree_next(node);
    } else {
      before = node;
      after = node;
    }

    root = range_tree_fuse(root, type, before, after);
  } else {
    root = range_tree_create(NULL, NULL, NULL, buffer, buffer_offset, buffer_length, inserter);
    range_tree_shrink(root, type);
  }

  fragment_dereference(buffer);

  return root;
}

// Helper for textual insertions
struct range_tree_node* range_tree_insert_split(struct range_tree_node* root, struct file_type* type, file_offset_t offset, const char* text, size_t length, int inserter, struct range_tree_node** inserts) {
  file_offset_t pos = 0;
  file_offset_t old = 0;
  while (1) {
    if (pos==length || pos-old>TREE_BLOCK_LENGTH_MAX || ((inserter&TIPPSE_INSERTER_ESCAPE) && text[pos]=='\x7f')) {
      char* copy = (char*)malloc(pos-old);
      memcpy(copy, text+old, pos-old);
      if (inserts) {
        *inserts = range_tree_last(root);
        inserts++;
      }

      struct fragment* buffer = fragment_create_memory(copy, pos-old);
      root = range_tree_insert(root, type, offset, buffer, 0, buffer->length, inserter);

      offset += pos-old;
      old = pos;

      if (pos<length && (inserter&TIPPSE_INSERTER_ESCAPE) && text[pos]=='\x7f') {
        old++;
      }
    }

    if (pos==length) {
      break;
    }

    pos++;
  }

  if (inserts) {
    *inserts = range_tree_last(root);
    inserts++;
  }

  return root;
}

// Remove specific range from tree (eventually break nodes into parts)
struct range_tree_node* range_tree_delete(struct range_tree_node* root, struct file_type* type, file_offset_t offset, file_offset_t length, int inserter) {
  while (length>0) {
    file_offset_t split = 0;
    struct range_tree_node* node = range_tree_find_offset(root, offset, &split);
    if (!node) {
      return NULL;
    }

    struct range_tree_node* before = range_tree_prev(node);
    struct range_tree_node* after = range_tree_next(node);

    file_offset_t node_length = node->length-split;
    file_offset_t sub = node_length>length?length:node_length;
    if (!(node->inserter&TIPPSE_INSERTER_READONLY) || (inserter&TIPPSE_INSERTER_AUTO)) {
      if (split>0 && length+split<node->length) {
        struct range_tree_node* build1 = range_tree_create(NULL, NULL, NULL, node->buffer, split+length+node->offset, node->length-split-length, node->inserter);
        struct range_tree_node* build0 = range_tree_create(node->parent, node, build1, NULL, 0, 0, 0);
        build1->parent = build0;
        node->parent = build0;
        node->length = split;

        build1->next = node->next;
        if (node->next) {
          node->next->prev = build1;
        }

        node->next = build1;
        build1->prev = node;

        range_tree_exchange(build0->parent, node, build0);
        range_tree_shrink(node, type);
        range_tree_shrink(build1, type);
        range_tree_update(build1);
        root = range_tree_update(node);
      } else {
        node->length -= sub;
        if (split==0) {
          node->offset += sub;
        }
      }
    } else {
      offset += sub;
    }

    length -= sub;

    if (node->length<=0) {
      fragment_dereference(node->buffer);
      node->buffer = NULL;
    }

    range_tree_shrink(node, type);
    root = range_tree_update(node);
    if (root) {
      root = range_tree_fuse(root, type, before, after);
    }
  }

  return root;
}

// Create copy of a specific range and keep fragments untouched (referenced copy)
struct range_tree_node* range_tree_copy(struct range_tree_node* root, struct file_type* type, file_offset_t offset, file_offset_t length) {
  struct range_tree_node* build0;
  struct range_tree_node* build1;
  struct range_tree_node* copy = NULL;
  file_offset_t split = 0;
  file_offset_t split2 = 0;
  struct range_tree_node* node = range_tree_find_offset(root, offset, &split);
  struct range_tree_node* last = NULL;
  while (node && length>0) {
    split2 = node->length-split;
    if (split2>length) {
      split2 = length;
    }

    build1 = range_tree_create(NULL, NULL, NULL, node->buffer, node->offset+split, split2, node->inserter);
    split = 0;
    if (last) {
      build0 = range_tree_create(last->parent, last, build1, NULL, 0, 0, 0);
      build1->parent = build0;
      last->parent = build0;

      build1->next = last->next;
      last->next = build1;
      build1->prev = last;

      range_tree_exchange(build0->parent, last, build0);
      range_tree_update(build1);
      copy = range_tree_update(last);
    } else {
      copy = build1;
    }
    last = build1;

    length -= build1->length;
    node = range_tree_next(node);
  }

  return copy;
}

// Insert already built nodes
struct range_tree_node* range_tree_paste(struct range_tree_node* root, struct file_type* type, struct range_tree_node* copy, file_offset_t offset) {
  copy = range_tree_first(copy);
  while (copy) {
    fragment_reference(copy->buffer);
    root = range_tree_insert(root, type, offset, copy->buffer, copy->offset, copy->length, TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER);
    offset += copy->length;
    copy = range_tree_next(copy);
  }

  return root;
}

// Copy specific range from tree into a buffer
char* range_tree_raw(struct range_tree_node* root, file_offset_t start, file_offset_t end) {
  if (!root) {
    return strdup("");
  }

  file_offset_t length = end-start;
  char* text = malloc(sizeof(char)*(length+1));
  file_offset_t offset = 0;

  while (length>0) {
    file_offset_t split = 0;
    struct range_tree_node* node = range_tree_find_offset(root, start, &split);
    if (!node) {
      break;
    }

    file_offset_t sub = node->length-split;
    if (sub>length) {
      sub = length;
    }

    memcpy(text+offset, node->buffer->buffer+node->offset+split, sub);
    length -= sub;
    offset += sub;
    start += sub;
  }

  *(text+offset) = '\0';
  return text;
}
