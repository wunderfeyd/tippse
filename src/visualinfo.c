// Tippse - Visual information store - Holds information about variable length or long visuals like comments/strings etc.

#include "visualinfo.h"

#include "config.h"
#include "rangetree.h"

// color entry names
struct config_cache visual_color_codes[VISUAL_FLAG_COLOR_MAX+1] = {
  {"background", VISUAL_FLAG_COLOR_BACKGROUND, NULL},
  {"text", VISUAL_FLAG_COLOR_TEXT, NULL},
  {"selection", VISUAL_FLAG_COLOR_SELECTION, NULL},
  {"status", VISUAL_FLAG_COLOR_STATUS, NULL},
  {"frame", VISUAL_FLAG_COLOR_FRAME, NULL},
  {"string", VISUAL_FLAG_COLOR_STRING, NULL},
  {"type", VISUAL_FLAG_COLOR_TYPE, NULL},
  {"keyword", VISUAL_FLAG_COLOR_KEYWORD, NULL},
  {"preprocessor", VISUAL_FLAG_COLOR_PREPROCESSOR, NULL},
  {"linecomment", VISUAL_FLAG_COLOR_LINECOMMENT, NULL},
  {"blockcomment", VISUAL_FLAG_COLOR_BLOCKCOMMENT, NULL},
  {"plus", VISUAL_FLAG_COLOR_PLUS, NULL},
  {"minus", VISUAL_FLAG_COLOR_MINUS, NULL},
  {"bracket", VISUAL_FLAG_COLOR_BRACKET, NULL},
  {"linenumber", VISUAL_FLAG_COLOR_LINENUMBER, NULL},
  {"bracketerror", VISUAL_FLAG_COLOR_BRACKETERROR, NULL},
  {"consolenormal", VISUAL_FLAG_COLOR_CONSOLENORMAL, NULL},
  {"consolewarning", VISUAL_FLAG_COLOR_CONSOLEWARNING, NULL},
  {"consoleerror", VISUAL_FLAG_COLOR_CONSOLEERROR, NULL},
  {"bookmark", VISUAL_FLAG_COLOR_BOOKMARK, NULL},
  {"directory", VISUAL_FLAG_COLOR_DIRECTORY, NULL},
  {"modified", VISUAL_FLAG_COLOR_MODIFIED, NULL},
  {"removed", VISUAL_FLAG_COLOR_REMOVED, NULL},
  {NULL, 0, NULL}
};

// Reset structure to known state
void visual_info_clear(struct visual_info* visuals) {
  memset(visuals, 0, sizeof(struct visual_info));
  visuals->dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
}

// Update page with informations about two other pages (usally its children)
void visual_info_combine(struct visual_info* visuals, const struct visual_info* left, const struct visual_info* right) {
  visuals->characters = left->characters+right->characters;
  visuals->lines = left->lines+right->lines;
  visuals->ys = left->ys+right->ys;
  if (right->ys!=0) {
    visuals->xs = right->xs;
  } else {
    visuals->xs = left->xs+right->xs;
  }

  if (right->lines!=0) {
    visuals->columns = right->columns;
    visuals->indentation = right->indentation;
    visuals->indentation_extra = right->indentation_extra;
  } else {
    visuals->columns = left->columns+right->columns;
    visuals->indentation = left->indentation+right->indentation;
    visuals->indentation_extra = left->indentation_extra+right->indentation_extra;
  }

  visuals->detail_before = left->detail_before;
  visuals->detail_after = ((left->detail_after&right->detail_after)&VISUAL_DETAIL_WHITESPACED_COMPLETE)|((left->detail_after|right->detail_after)&(VISUAL_DETAIL_WHITESPACED_START|VISUAL_DETAIL_STOPPED_INDENTATION|VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_NEWLINE));

  int dirty = (left->dirty|right->dirty)&VISUAL_DIRTY_UPDATE;
  dirty |= (left->dirty)&(VISUAL_DIRTY_SPLITTED|VISUAL_DIRTY_LEFT);

  if (!(left->dirty&VISUAL_DIRTY_UPDATE) && (right->dirty&VISUAL_DIRTY_LEFT) && !(dirty&VISUAL_DIRTY_SPLITTED)) {
    dirty |= VISUAL_DIRTY_LASTSPLIT|VISUAL_DIRTY_SPLITTED;
  }

  visuals->dirty = dirty;

  for (size_t n = 0; n<VISUAL_BRACKET_MAX; n++) {
    visuals->brackets[n].diff = right->brackets[n].diff+left->brackets[n].diff;

    visuals->brackets[n].max = right->brackets[n].max+left->brackets[n].diff;
    if (visuals->brackets[n].max<left->brackets[n].max) {
      visuals->brackets[n].max = left->brackets[n].max;
    }

    visuals->brackets[n].min = right->brackets[n].min-left->brackets[n].diff;
    if (visuals->brackets[n].min<left->brackets[n].min) {
      visuals->brackets[n].min = left->brackets[n].min;
    }

    visuals->brackets_line[n].diff = right->brackets_line[n].diff+left->brackets_line[n].diff;

    visuals->brackets_line[n].max = right->brackets_line[n].max+left->brackets_line[n].diff;
    if (visuals->brackets_line[n].max<left->brackets_line[n].max) {
      visuals->brackets_line[n].max = left->brackets_line[n].max;
    }

    visuals->brackets_line[n].min = right->brackets_line[n].min-left->brackets_line[n].diff;
    if (visuals->brackets_line[n].min<left->brackets_line[n].min) {
      visuals->brackets_line[n].min = left->brackets_line[n].min;
    }
  }
}

// Invalidate visualization status of current node and maybe previous nodes
void visual_info_invalidate(struct range_tree_node* node, struct range_tree* tree) {
  if (!node->buffer) {
    return;
  }

  node->visuals.dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
  struct range_tree_node* invalidate = range_tree_node_next(node);
  if (invalidate) {
    invalidate->visuals.dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
    range_tree_node_update_calc_all(invalidate, tree);
  }

  invalidate = node;

  file_offset_t rewind = invalidate->visuals.rewind;

  // TODO: Rewind is needed for word wrapping correction ... figure out when exactly and reduce the rewind as much as possible for performance
  if (rewind<1024) {
    rewind = 1024;
  }

  while (rewind>0) {
    invalidate = range_tree_node_prev(invalidate);
    if (!invalidate) {
      break;
    }

    invalidate->visuals.dirty = VISUAL_DIRTY_UPDATE|VISUAL_DIRTY_LEFT;
    range_tree_node_update_calc_all(invalidate, tree);
    if (invalidate->length>rewind) {
      break;
    }

    rewind -= invalidate->length;
  }
}

// Find first non dirty node before or at specified visualisation attributes
struct range_tree_node* visual_info_find(struct range_tree_node* node, int find_type, file_offset_t find_offset, position_t find_x, position_t find_y, position_t find_line, position_t find_column, file_offset_t* offset, position_t* x, position_t* y, position_t* line, position_t* column, int* indentation, int* indentation_extra, file_offset_t* character, int retry, file_offset_t before) {
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
      root = visual_info_find(root, VISUAL_SEEK_OFFSET, location-node->visuals.rewind, find_x, find_y, find_line, find_column, offset, x, y, line, column, indentation, indentation_extra, character, 1, before);
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
int visual_info_find_bracket(const struct range_tree_node* node, size_t bracket) {
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
struct range_tree_node* visual_info_find_bracket_forward(struct range_tree_node* node, size_t bracket, int search) {
  if (!node || !node->parent) {
    return NULL;
  }

  file_offset_t before = range_tree_node_offset(node);
  int depth = visual_info_find_bracket(node, bracket);
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

  file_offset_t after = range_tree_node_offset(node);
  if (after<=before) {
    node = NULL;
  }

  return node;
}

// Find next closest backwards match (may be wrong due to page invalidation, but while rendering we should find it anyway)
struct range_tree_node* visual_info_find_bracket_backward(struct range_tree_node* node, size_t bracket, int search) {
  if (!node || !node->parent) {
    return NULL;
  }

  int depth = visual_info_find_bracket(node, bracket);
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
void visual_info_find_bracket_lowest(const struct range_tree_node* node, int* mins, const struct range_tree_node* last) {
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
struct range_tree_node* visual_info_find_indentation_last(struct range_tree_node* node, position_t lines, struct range_tree_node* last) {
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
int visual_info_find_indentation(const struct range_tree_node* node) {
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
int visual_info_find_whitespaced(const struct range_tree_node* node) {
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
