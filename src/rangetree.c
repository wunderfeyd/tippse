// Tippse - Range tree - For fast access to various attributes of the document

#include "rangetree.h"

// Debug: Recursively print tree nodes
void range_tree_print(struct range_tree_node* node, int depth, int side) {
  int tab = depth;
  while (tab>0) {
    printf("  ");
    tab--;
  }

  printf("%d %5d %5d %s(%p-%p) %5d %5d (%p) %d %d %d", side, (int)node->length, (int)node->visuals.lines, node->buffer?"B":" ", (void*)node->buffer, (void*)(node->buffer?node->buffer->buffer:NULL), (int)node->offset, node->depth, (void*)node, node->visuals.brackets[2].diff, node->visuals.brackets[2].min, node->visuals.brackets[2].max);
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
    printf("children 0 has wrong parent %p -> %p should %p\r\n", (void*)node->side[0], (void*)node->side[0]->parent, (void*)node);
  }

  if (node->side[1] && node->side[1]->parent!=node) {
    printf("children 1 has wrong parent %p -> %p should %p\r\n", (void*)node->side[1], (void*)node->side[1]->parent, (void*)node);
  }

  if ((node->side[0] && !node->side[1]) || (node->side[1] && !node->side[0])) {
    printf("unbalanced node %p: %p %p\r\n", (void*)node, (void*)node->side[0], (void*)node->side[1]);
  }

  if (node->buffer && (node->side[0] || node->side[1])) {
    printf("Leaf with children %p: %p %p\r\n", (void*)node, (void*)node->side[0], (void*)node->side[1]);
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
  if (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    node->length = node->side[0]->length+node->side[1]->length;
    node->depth = ((node->side[0]->depth>node->side[1]->depth)?node->side[0]->depth:node->side[1]->depth)+1;
    node->inserter = ((node->side[0]?node->side[0]->inserter:0)|(node->side[1]?node->side[1]->inserter:0))&~TIPPSE_INSERTER_LEAF;
    visual_info_combine(&node->visuals, &node->side[0]->visuals, &node->side[1]->visuals);
  } else {
    if (node->buffer) {
      if (node->buffer->type==FRAGMENT_FILE) {
        node->inserter |= TIPPSE_INSERTER_FILE;
      } else {
        node->inserter &= ~TIPPSE_INSERTER_FILE;
      }
    }
  }
}

// Recursively recalc all nodes on path up to the root node
void range_tree_update_calc_all(struct range_tree_node* node) {
  range_tree_update_calc(node);
  if (node->parent) {
    range_tree_update_calc_all(node->parent);
  }
}

// Remove node and all children
void range_tree_destroy(struct range_tree_node* node, struct document_file* file) {
  if (!node) {
    return;
  }

  range_tree_destroy(node->side[0], file);
  range_tree_destroy(node->side[1], file);

  if (node->buffer) {
    fragment_dereference(node->buffer, file);
  }

  free(node);
}

// Create node with given fragment
struct range_tree_node* range_tree_create(struct range_tree_node* parent, struct range_tree_node* side0, struct range_tree_node* side1, struct fragment* buffer, file_offset_t offset, file_offset_t length, int inserter, struct document_file* file) {
  struct range_tree_node* node = malloc(sizeof(struct range_tree_node));
  node->parent = parent;
  node->side[0] = side0;
  node->side[1] = side1;
  node->buffer = buffer;
  node->next = NULL;
  node->prev = NULL;
  if (node->buffer) {
    fragment_reference(node->buffer, file);
  }

  node->offset = offset;
  node->length = length;
  node->inserter = inserter;
  node->depth = 0;
  visual_info_clear(&node->visuals);
  return node;
}

// Find first leaf
struct range_tree_node* range_tree_first(struct range_tree_node* node) {
  if (!node) {
    return NULL;
  }

  while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    node = node->side[0];
  }

  return node;
}

// Find last leaf
struct range_tree_node* range_tree_last(struct range_tree_node* node) {
  if (!node) {
    return NULL;
  }

  while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
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
    printf("umm ... dead parent %p old %p new %p side 0 %p side 1 %p\r\n", (void*)node, (void*)old, (void*)new, (void*)node->side[0], (void*)node->side[1]);
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
  //range_tree_balance(redo);

  return node;
}

// Balance tree while left side must not have more depth (+1) than the right side
struct range_tree_node* range_tree_balance(struct range_tree_node* node) {
  while (node->side[0]->depth-1>node->side[1]->depth && !(node->side[0]->inserter&TIPPSE_INSERTER_LEAF)) {
    node = range_tree_rotate(node, 0);
  }

  while (node->side[0]->depth<node->side[1]->depth && !(node->side[1]->inserter&TIPPSE_INSERTER_LEAF)) {
    node = range_tree_rotate(node, 1);
  }

  range_tree_update_calc(node);

  return node;
}

// Update path to root node, throw away useless nodes and call the balancer (root node might change)
struct range_tree_node* range_tree_update(struct range_tree_node* node) {
  struct range_tree_node* last = node;
  while (node) {
    last = node;
    if (node->inserter&TIPPSE_INSERTER_LEAF) {
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

    node = range_tree_balance(node);

    last = node;
    node = node->parent;
  }

  return last;
}

// Find first non dirty node before or at specified visualisation attributes
struct range_tree_node* range_tree_find_visual(struct range_tree_node* node, int find_type, file_offset_t find_offset, position_t find_x, position_t find_y, position_t find_line, position_t find_column, file_offset_t* offset, position_t* x, position_t* y, position_t* line, position_t* column, int* indentation, int* indentation_extra, file_offset_t* character, int retry, file_offset_t before) {
  struct range_tree_node* root = node;
  file_offset_t location = 0;
  position_t ys = 0;
  position_t xs = 0;
  position_t lines = 0;
  position_t columns = 0;
  file_offset_t characters = 0;
  int indentations = 0;
  int indentations_extra = 0;

  while (node && !(node->inserter&TIPPSE_INSERTER_LEAF)) {
    position_t columns_new = columns;
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

    position_t xs_new = xs;
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

  if (node && (node->inserter&TIPPSE_INSERTER_LEAF)) {
    if (node->visuals.rewind>0 && !retry) {
      root = range_tree_find_visual(root, VISUAL_SEEK_OFFSET, location-node->visuals.rewind, find_x, find_y, find_line, find_column, offset, x, y, line, column, indentation, indentation_extra, character, 1, before);
      if (*offset>=before || location+node->length<=before) {
        return root;
      }
    }

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

// Return bracket depth
int range_tree_find_bracket(struct range_tree_node* node, size_t bracket) {
  int depth = 0;
  while (node->parent) {
    if (node->parent->side[1]==node) {
      depth += node->parent->side[0]->visuals.brackets[bracket].diff;
    }

    node = node->parent;
  }

  return depth;
}

// Find next closest forward match (may be wrong due to page invalidation, but while rendering we should find it anyway)
struct range_tree_node* range_tree_find_bracket_forward(struct range_tree_node* node, size_t bracket, int search) {
  if (!node || !node->parent) {
    return NULL;
  }

  file_offset_t before = range_tree_offset(node);
  int depth = range_tree_find_bracket(node, bracket);
  while (node->parent) {
    if (node->parent->side[1]==node) {
      depth -= node->parent->side[0]->visuals.brackets[bracket].diff;
    } else {
      if (depth-node->parent->visuals.brackets[bracket].min<=search && depth+node->parent->visuals.brackets[bracket].max>=search) {
        node = node->parent;
        break;
      }
    }

    node = node->parent;
  }

  depth += node->side[0]->visuals.brackets[bracket].diff;
  node = node->side[1];

  while (node && !(node->inserter&TIPPSE_INSERTER_LEAF)) {
    if (depth-node->side[0]->visuals.brackets[bracket].min<=search && depth+node->side[0]->visuals.brackets[bracket].max>=search) {
      node = node->side[0];
      continue;
    }

    depth += node->side[0]->visuals.brackets[bracket].diff;
    node = node->side[1];
  }

  file_offset_t after = range_tree_offset(node);
  if (after<=before) {
    node = NULL;
  }

  return node;
}

// Find next closest backwards match (may be wrong due to page invalidation, but while rendering we should find it anyway)
struct range_tree_node* range_tree_find_bracket_backward(struct range_tree_node* node, size_t bracket, int search) {
  if (!node || !node->parent) {
    return NULL;
  }

  int depth = range_tree_find_bracket(node, bracket);
  while (node->parent) {
    if (node->parent->side[1]==node) {
      depth -= node->parent->side[0]->visuals.brackets[bracket].diff;
      if (depth-node->parent->side[0]->visuals.brackets[bracket].min<=search && depth+node->parent->side[0]->visuals.brackets[bracket].max>=search) {
        node = node->parent;
        break;
      }
    }

    node = node->parent;
  }

  node = node->side[0];

  while (node && !(node->inserter&TIPPSE_INSERTER_LEAF)) {
    if (depth-node->side[1]->visuals.brackets[bracket].min+node->side[0]->visuals.brackets[bracket].diff<=search && depth+node->side[1]->visuals.brackets[bracket].max+node->side[0]->visuals.brackets[bracket].diff>=search) {
      depth += node->side[0]->visuals.brackets[bracket].diff;
      node = node->side[1];
      continue;
    }

    node = node->side[0];
  }

  return node;
}

// Check for lowest bracket depth
void range_tree_find_bracket_lowest(struct range_tree_node* node, int* mins, struct range_tree_node* last) {
  if (!node) {
    if (!last) {
      return;
    }

    node = last;

    for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
      int min1 = mins[n]-node->visuals.brackets_line[n].diff;
      int min2 = node->visuals.brackets_line[n].min;
      mins[n] = (min2>min1)?min2:min1;
    }

    if (node->visuals.lines!=0) {
      return;
    }
  }

  while (node->parent) {
    if (node->parent->side[1]==node) {
      if (node->parent->side[0]->visuals.lines!=0) {
        node = node->parent;
        break;
      }

      for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
        int min1 = mins[n]-node->parent->side[0]->visuals.brackets_line[n].diff;
        int min2 = node->parent->side[0]->visuals.brackets_line[n].min;
        mins[n] = (min2>min1)?min2:min1;
      }
    }

    node = node->parent;
  }

  if (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    node = node->side[0];

    while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
      if (node->side[1]->visuals.lines==0) {
        for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
          int min1 = mins[n]-node->side[1]->visuals.brackets_line[n].diff;
          int min2 = node->side[1]->visuals.brackets_line[n].min;
          mins[n] = (min2>min1)?min2:min1;
        }

        node = node->side[0];
      } else {
        node = node->side[1];
      }
    }
  }

  for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
    int min1 = mins[n]-node->visuals.brackets_line[n].diff;
    int min2 = node->visuals.brackets_line[n].min;
    mins[n] = (min2>min1)?min2:min1;
  }
}

// Find last indentation on line
struct range_tree_node* range_tree_find_indentation_last(struct range_tree_node* node, position_t lines, struct range_tree_node* last) {
  if (!node) {
    if (!last) {
      return NULL;
    }

    node = last;
    lines = 0;
  }

  if (!node->parent) {
    return node;
  }

  struct range_tree_node* from = node;
  // Climb to first node of line
  if (lines==0 && node->prev!=NULL) {
    while (node->parent) {
      if (node->parent->side[1]==node) {
        if (node->parent->side[0]->visuals.lines!=0) {
          node = node->parent;
          break;
        }
      }

      node = node->parent;
    }

    node = node->side[0];

    while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
      if (node->side[1]->visuals.lines==0) {
        node = node->side[0];
      } else {
        node = node->side[1];
      }
    }
  }

  if ((node->visuals.lines!=lines && from==node) || (node->visuals.detail_after&VISUAL_DETAIL_STOPPED_INDENTATION)) {
    return node;
  }

  // Climb to last node of line or not indented node
  while (node->parent) {
    if (node->parent->side[0]==node) {
      if (node->parent->side[1]->visuals.lines!=0 || (node->parent->side[1]->visuals.detail_after&VISUAL_DETAIL_STOPPED_INDENTATION)) {
        node = node->parent;
        break;
      }
    }

    node = node->parent;
  }

  node = node->side[1];

  while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    if (node->side[0]->visuals.lines==0 && !(node->side[0]->visuals.detail_after&VISUAL_DETAIL_STOPPED_INDENTATION)) {
      node = node->side[1];
    } else {
      node = node->side[0];
    }
  }

  return node;
}

// Check for identation reaching given node
int range_tree_find_indentation(struct range_tree_node* node) {
  if (!node || !node->parent) {
    return 0;
  }

  while (node->parent) {
    if (node->parent->side[1]==node) {
      if (node->parent->side[0]->visuals.lines!=0) {
        node = node->parent;
        break;
      }

      if (node->parent->side[0]->visuals.detail_after&VISUAL_DETAIL_STOPPED_INDENTATION) {
        return 0;
      }
    }

    node = node->parent;
  }

  node = node->side[0];

  while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    if (node->side[1]->visuals.lines==0) {
      if (node->side[1]->visuals.detail_after&VISUAL_DETAIL_STOPPED_INDENTATION) {
        return 0;
      }

      node = node->side[0];
    } else {
      node = node->side[1];
    }
  }

  return (node->visuals.detail_after&VISUAL_DETAIL_STOPPED_INDENTATION)?0:1;
}

// Check if whitespacing stops at line end
int range_tree_find_whitespaced(struct range_tree_node* node) {
  if (node->visuals.detail_after&VISUAL_DETAIL_WHITESPACED_START) {
    return 1;
  }

  if (!(node->visuals.detail_after&VISUAL_DETAIL_WHITESPACED_COMPLETE)) {
    return 0;
  }

  while (node->parent) {
    if (node->parent->side[0]==node) {
      if (!(node->parent->side[1]->visuals.detail_after&VISUAL_DETAIL_WHITESPACED_COMPLETE)) {
        node = node->parent->side[1];
        break;
      }
    }

    node = node->parent;
  }

  if (!node->parent) {
    return 1;
  }

  while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    if (!(node->side[0]->visuals.detail_after&VISUAL_DETAIL_WHITESPACED_COMPLETE)) {
      node = node->side[0];
    } else {
      node = node->side[1];
    }
  }

  return (node->visuals.detail_after&(VISUAL_DETAIL_WHITESPACED_COMPLETE|VISUAL_DETAIL_WHITESPACED_START))?1:0;
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

// Search node by offset and return difference to its base offset
struct range_tree_node* range_tree_find_offset(struct range_tree_node* node, file_offset_t offset, file_offset_t* diff) {
  while (node && !(node->inserter&TIPPSE_INSERTER_LEAF)) {
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
void range_tree_shrink(struct range_tree_node* node) {
  if (!node || !(node->inserter&TIPPSE_INSERTER_LEAF)) {
    return;
  }

  if (node->buffer) {
    node->visuals.dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
    struct range_tree_node* invalidate = range_tree_next(node);
    if (invalidate) {
      invalidate->visuals.dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
      range_tree_update_calc_all(invalidate);
    }

    invalidate = node;

    file_offset_t rewind = invalidate->visuals.rewind;
    while (rewind>0) {
      invalidate = range_tree_prev(invalidate);
      if (!invalidate) {
        break;
      }

      invalidate->visuals.dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
      range_tree_update_calc_all(invalidate);
      if (invalidate->length>rewind) {
        break;
      }

      rewind -= invalidate->length;
    }

    if (node->buffer->count==1 && (node->offset!=0 || node->length!=node->buffer->length) && node->length>0) {
      if (node->buffer->type==FRAGMENT_MEMORY) {
        uint8_t* text = malloc(node->length);
        memcpy(text, node->buffer->buffer+node->offset, node->length);
        free(node->buffer->buffer);
        node->buffer->buffer = text;
        node->buffer->length = node->length;
        node->offset = 0;
      }
    }
  }
}

// Try to merge range of nodes which are below the maximum page size
struct range_tree_node* range_tree_fuse(struct range_tree_node* root, struct range_tree_node* first, struct range_tree_node* last, struct document_file* file) {
  if (!first) {
    first = range_tree_first(root);
  }

  if (!last) {
    last = range_tree_last(root);
  }

  while (first!=last) {
    struct range_tree_node* next = range_tree_next(first);

    if (first->inserter==next->inserter && !(first->inserter&TIPPSE_INSERTER_NOFUSE)) {
      if (!first->buffer && !next->buffer) {
        next->length = first->length+next->length;
        range_tree_update(next);
        first->inserter &= ~TIPPSE_INSERTER_LEAF;
        root = range_tree_update(first);
      } else if (first->length+next->length<TREE_BLOCK_LENGTH_MAX) {
        if (first->buffer && next->buffer && (first->buffer->type==FRAGMENT_MEMORY || next->buffer->type==FRAGMENT_MEMORY)) {
          fragment_cache(first->buffer);
          fragment_cache(next->buffer);
          uint8_t* copy = (uint8_t*)malloc(first->length+next->length);
          memcpy(copy, first->buffer->buffer+first->offset, first->length);
          memcpy(copy+first->length, next->buffer->buffer+next->offset, next->length);
          struct fragment* buffer = fragment_create_memory(copy, first->length+next->length);
          fragment_dereference(first->buffer, file);
          first->buffer = buffer;
          first->length = buffer->length;
          first->offset = 0;

          range_tree_shrink(first);
          range_tree_update(first);

          if (next==last) {
            last = first;
          }
          fragment_dereference(next->buffer, file);
          next->buffer = NULL;
          next->inserter &= ~TIPPSE_INSERTER_LEAF;
          root = range_tree_update(next);
          continue;
        }
      }
    }

    first = next;
  }

  return root;
}

// Insert fragment into specific offset and eventually break older nodes into parts
struct range_tree_node* range_tree_insert(struct range_tree_node* root, file_offset_t offset, struct fragment* buffer, file_offset_t buffer_offset, file_offset_t buffer_length, int inserter, struct document_file* file) {
  struct range_tree_node* node;
  struct range_tree_node* build0;
  struct range_tree_node* build1;
  struct range_tree_node* build2;
  struct range_tree_node* build3;
  file_offset_t split = 0;

  if (root) {
    node = range_tree_find_offset(root, offset, &split);
    if (!node) {
      node = range_tree_last(root);
      split = node->length;
    }

    struct range_tree_node* before = NULL;
    struct range_tree_node* after = NULL;

    build1 = range_tree_create(NULL, NULL, NULL, buffer, buffer_offset, buffer_length, inserter|TIPPSE_INSERTER_LEAF, file);

    if (split==node->length) {
      build0 = range_tree_create(node->parent, node, build1, NULL, 0, 0, 0, NULL);
      build1->parent = build0;
      node->parent = build0;

      build1->next = node->next;
      if (node->next) {
        node->next->prev = build1;
      }

      node->next = build1;
      build1->prev = node;

      range_tree_exchange(build0->parent, node, build0);
      range_tree_shrink(build1);
      range_tree_shrink(node);
      range_tree_update_calc(build1);
      range_tree_update_calc(node);
      root = range_tree_update(build0);
      before = range_tree_prev(build1);
      after = range_tree_next(node);
    } else if (split==0) {
      build0 = range_tree_create(node->parent, build1, node, NULL, 0, 0, 0, NULL);
      build1->parent = build0;
      node->parent = build0;

      build1->prev = node->prev;
      if (node->prev) {
        node->prev->next = build1;
      }

      node->prev = build1;
      build1->next = node;

      range_tree_exchange(build0->parent, node, build0);
      range_tree_shrink(build1);
      range_tree_shrink(node);
      range_tree_update_calc(build1);
      range_tree_update_calc(node);
      root = range_tree_update(build0);
      before = range_tree_prev(build1);
      after = range_tree_next(node);
    } else {
      build2 = range_tree_create(NULL, NULL, NULL, node->buffer, node->offset, split, node->inserter|TIPPSE_INSERTER_LEAF, file);
      build0 = range_tree_create(NULL, build2, build1, NULL, 0, 0, 0, NULL);
      build3 = range_tree_create(node->parent, build0, node, NULL, 0, 0, 0, NULL);
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

      range_tree_shrink(build2);
      range_tree_shrink(build1);
      range_tree_shrink(node);

      range_tree_update_calc(build2);
      range_tree_update_calc(build1);
      range_tree_update_calc(build0);
      range_tree_update_calc(node);
      root = range_tree_update(build3);

      before = range_tree_prev(build2);
      after = range_tree_next(node);
    }

    root = range_tree_fuse(root, before, after, file);
  } else {
    root = range_tree_create(NULL, NULL, NULL, buffer, buffer_offset, buffer_length, inserter|TIPPSE_INSERTER_LEAF, file);
    range_tree_shrink(root);
    range_tree_update_calc(root);
  }

  return root;
}

// Split input buffer into nodes with maximum allowed length
struct range_tree_node* range_tree_insert_split(struct range_tree_node* root, file_offset_t offset, const uint8_t* text, size_t length, int inserter) {
  for (size_t pos = 0; pos<length; pos+=TREE_BLOCK_LENGTH_MAX) {
    size_t size = length-pos;
    if (size>TREE_BLOCK_LENGTH_MAX) {
      size = TREE_BLOCK_LENGTH_MAX;
    }

    uint8_t* copy = (uint8_t*)malloc(size);
    memcpy(copy, text+pos, size);
    struct fragment* buffer = fragment_create_memory(copy, size);
    root = range_tree_insert(root, offset, buffer, 0, buffer->length, inserter, NULL);
    fragment_dereference(buffer, NULL);

    offset += TREE_BLOCK_LENGTH_MAX;
  }

  return root;
}

// Remove specific range from tree (eventually break nodes into parts)
struct range_tree_node* range_tree_delete(struct range_tree_node* root, file_offset_t offset, file_offset_t length, int inserter, struct document_file* file) {
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
    if (split>0 && length+split<node->length) {
      struct range_tree_node* build1 = range_tree_create(NULL, NULL, NULL, node->buffer, split+length+node->offset, node->length-split-length, node->inserter|TIPPSE_INSERTER_LEAF, file);
      struct range_tree_node* build0 = range_tree_create(node->parent, node, build1, NULL, 0, 0, 0, NULL);
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
      range_tree_shrink(node);
      range_tree_shrink(build1);
      range_tree_update(build1);
      root = range_tree_update(node);
    } else {
      node->length -= sub;
      if (split==0) {
        node->offset += sub;
      }
    }

    length -= sub;

    range_tree_shrink(node);

    if (node->length<=0) {
      if (node->buffer) {
        fragment_dereference(node->buffer, file);
        node->buffer = NULL;
      }
      node->inserter &= ~TIPPSE_INSERTER_LEAF;
    }

    root = range_tree_update(node);
    if (root) {
      root = range_tree_fuse(root, before, after, file);
    }
  }

  return root;
}

// Create copy of a specific range and insert into another or same tree
struct range_tree_node* range_tree_copy_insert(struct range_tree_node* root_from, file_offset_t offset_from, struct range_tree_node* root_to, file_offset_t offset_to, file_offset_t length, struct document_file* file) {
  file_offset_t split = 0;
  file_offset_t split2 = 0;
  int same = (root_from==root_to)?1:0;
  struct range_tree_node* node = range_tree_find_offset(root_from, offset_from, &split);
  while (node && length>0) {
    split2 = node->length-split;
    if (split2>length) {
      split2 = length;
    }

    root_to = range_tree_insert(root_to, offset_to, node->buffer, node->offset+split, split2, node->inserter, file);
    offset_to += split2;
    offset_from += split2;
    length -= split2;
    if (same) {
      node = range_tree_find_offset(root_to, offset_from, &split);
    } else {
      split = 0;
      node = range_tree_next(node);
    }
  }

  return root_to;
}

// Create copy of a specific range
struct range_tree_node* range_tree_copy(struct range_tree_node* root, file_offset_t offset, file_offset_t length) {
  return range_tree_copy_insert(root, offset, NULL, 0, length, NULL);
}

// Insert already built nodes
struct range_tree_node* range_tree_paste(struct range_tree_node* root, struct range_tree_node* copy, file_offset_t offset, struct document_file* file) {
  copy = range_tree_first(copy);
  while (copy) {
    root = range_tree_insert(root, offset, copy->buffer, copy->offset, copy->length, copy->inserter, file);
    offset += copy->length;
    copy = range_tree_next(copy);
  }

  return root;
}

// Copy specific range from tree into a buffer
uint8_t* range_tree_raw(struct range_tree_node* root, file_offset_t start, file_offset_t end) {
  if (!root || end<=start) {
    return (uint8_t*)strdup("");
  }

  file_offset_t length = end-start;
  uint8_t* text = malloc(sizeof(uint8_t)*(length+1));
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

// Cleanup tree and build single full length node
struct range_tree_node* range_tree_static(struct range_tree_node* root, file_offset_t length, int inserter) {
  if (root) {
    range_tree_destroy(root, NULL);
  }

  if (length==0) {
    return NULL;
  }

  root = range_tree_create(NULL, NULL, NULL, NULL, 0, length, inserter|TIPPSE_INSERTER_LEAF, NULL);
  range_tree_update(root);
  return root;
}

// Resize tree to match length
struct range_tree_node* range_tree_resize(struct range_tree_node* root, file_offset_t length, int inserter) {
  if (!root || root->length==0) {
    root = range_tree_static(root, length, inserter);
  } else if (root->length>length) {
    root = range_tree_reduce(root, length, root->length-length);
  } else if (root->length<length) {
    file_offset_t last = root->length;
    root = range_tree_expand(root, last, length-last);
    root = range_tree_mark(root, last, length-last, inserter);
  }
  return root;
}

// Stretch node at offset by given length
struct range_tree_node* range_tree_expand(struct range_tree_node* root, file_offset_t offset, file_offset_t length) {
  file_offset_t split = 0;
  struct range_tree_node* node = range_tree_find_offset(root, offset, &split);
  if (node) {
    node->length += length;
    root = range_tree_update(node);
  } else {
    root = range_tree_resize(root, length, 0);
  }

  return root;
}

// Remove nodes at offset until offset+length
struct range_tree_node* range_tree_reduce(struct range_tree_node* root, file_offset_t offset, file_offset_t length) {
  return range_tree_delete(root, offset, length, 0, NULL);
}

// Split node into upper and lower parts (TODO: Reuse me in other functions doing the same)
struct range_tree_node* range_tree_split(struct range_tree_node* root, struct range_tree_node** node, file_offset_t split, struct document_file* file) {
  if (*node && split>0 && split<(*node)->length) {
    struct range_tree_node* build1 = range_tree_create(NULL, NULL, NULL, (*node)->buffer, (*node)->offset+split, (*node)->length-split, (*node)->inserter, file);
    struct range_tree_node* build0 = range_tree_create((*node)->parent, *node, build1, NULL, 0, 0, 0, NULL);
    (*node)->length = split;

    build1->parent = build0;
    (*node)->parent = build0;

    if ((*node)->next) {
      (*node)->next->prev = build1;
    }

    build1->next = (*node)->next;
    (*node)->next = build1;
    build1->prev = *node;

    range_tree_exchange(build0->parent, (*node), build0);
    range_tree_update(build1);
    root = range_tree_update(*node);
    *node = build1;
  }

  return root;
}

// Change marked attribute from offset to offset+length
struct range_tree_node* range_tree_mark(struct range_tree_node* root, file_offset_t offset, file_offset_t length, int inserter) {
  if (length==0) {
    return root;
  }

  file_offset_t split = 0;

  struct range_tree_node* first = range_tree_find_offset(root, offset, &split);
  struct range_tree_node* before = range_tree_prev(first);
  if (!before) {
    before = first;
  }

  root = range_tree_split(root, &first, split, NULL);

  struct range_tree_node* last = range_tree_find_offset(root, offset+length, &split);
  struct range_tree_node* after = last;
  root = range_tree_split(root, &after, split, NULL);

  while (1) {
    first->inserter = inserter|TIPPSE_INSERTER_LEAF;
    root = range_tree_update(first);
    if (first==last) {
      break;
    }

    first = range_tree_next(first);
    if (first==last) {
      break;
    }
  }

  return range_tree_fuse(root, before, after, NULL);
}

// Check for marked attribute
int range_tree_marked(struct range_tree_node* node, file_offset_t offset, file_offset_t length, int inserter) {
  if (!node) {
    return 0;
  }

  if (length>node->length) {
    length = node->length;
  }

  if ((node->inserter&TIPPSE_INSERTER_LEAF) || (offset==0 && node->length==length)) {
    return ((node->inserter&inserter)==inserter)?1:0;
  }

  int marked = 0;
  file_offset_t length0 = (node->side[0]->length>offset)?node->side[0]->length-offset:0;
  if (length0>length) {
    length0 = length;
  }

  file_offset_t length1 = length-length0;

  if (length0>0) {
    marked |= range_tree_marked(node->side[0], offset, length0, inserter);
  }

  if (length1>0) {
    marked |= range_tree_marked(node->side[1], offset-node->side[0]->length, length1, inserter);
  }

  return marked;
}

// Invert given flag on whole tree
struct range_tree_node* range_tree_invert_mark(struct range_tree_node* node, int inserter) {
  if (node->inserter&TIPPSE_INSERTER_LEAF) {
    node->inserter ^= inserter;
    return node;
  }

  range_tree_invert_mark(node->side[0], inserter);
  range_tree_invert_mark(node->side[1], inserter);
  range_tree_update_calc(node);
  return node;
}
