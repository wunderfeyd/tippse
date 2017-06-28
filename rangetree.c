/* Tippse - Range tree - For fast access to various attributes of the document */

#include "rangetree.h"

void range_tree_print(struct range_tree_node* node, int depth, int side) {
  int tab = depth;
  while (tab>0) {
    printf("  ");
    tab--;
  }
  
  printf ("%d %5d %5d %s(%p-%p) %5d %5d (%p) [0](%p) [1](%p) [P](%p)", side, (int)node->length, (int)node->lines, node->buffer?"B":" ", node->buffer, node->buffer?node->buffer->buffer:NULL, (int)node->offset, node->depth, node, node->side[0], node->side[1], node->parent);
  printf("\r\n");
  if (node->side[0]) {
    range_tree_print(node->side[0], depth+1, 0);
  }
  
  if (node->side[1]) {
    range_tree_print(node->side[1], depth+1, 1);
  }
}

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

void range_tree_print_root(struct range_tree_node* node, int depth, int side) {
  while(node->parent) {
    node = node->parent;
  }
  
  range_tree_print(node, depth, side);
}

void range_tree_update_calc(struct range_tree_node* node) {
  if (!node->buffer) {
    node->characters = node->side[0]->characters+node->side[1]->characters;
    node->lines = node->side[0]->lines+node->side[1]->lines;
    node->length = node->side[0]->length+node->side[1]->length;
    node->depth = ((node->side[0]->depth>node->side[1]->depth)?node->side[0]->depth:node->side[1]->depth)+1;
    node->inserter = (node->side[0]?node->side[0]->inserter:0)&(node->side[1]?node->side[1]->inserter:0);
  }

  node->subs = (node->side[0]?node->side[0]->subs+1:0)+(node->side[1]?node->side[1]->subs+1:0);
}

void range_tree_clear(struct range_tree_node* node) {
  if (node->side[0]) {
    range_tree_clear(node->side[0]);
  }

  if (node->side[1]) {
    range_tree_clear(node->side[1]);
  }

  if (node->buffer) {
    fragment_dereference(node->buffer);
  }

  free(node);
}

struct range_tree_node* range_tree_new_node(struct range_tree_node* parent, struct range_tree_node* side0, struct range_tree_node* side1, struct fragment* buffer, file_offset_t offset, file_offset_t length, int inserter) {
  struct range_tree_node* node = malloc(sizeof(struct range_tree_node));
  node->parent = parent;
  node->side[0] = side0;
  node->side[1] = side1;
  node->buffer = buffer;
  if (node->buffer) {
    fragment_reference(node->buffer);
  }

  node->offset = offset;
  node->length = length;
  node->column = -1;
  node->inserter = inserter;
  range_tree_update_calc(node);
  return node;
}

struct range_tree_node* range_tree_first(struct range_tree_node* node) {
  if (!node) {
    return NULL;
  }

  while (!node->buffer) {
    node = node->side[0];
  }

  return node;  
}

struct range_tree_node* range_tree_last(struct range_tree_node* node) {
  if (!node) {
    return NULL;
  }

  while (!node->buffer) {
    node = node->side[1];
  }

  return node;
}

struct range_tree_node* range_tree_next(struct range_tree_node* node) {
  while (1) {
    struct range_tree_node* parent = node->parent;
    if (!parent) {
      return NULL;
    }

    if (parent->side[0]==node) {
      node = parent->side[1];
      break;
    }
    
    node = parent;
  }
  
  return range_tree_first(node);
}

struct range_tree_node* range_tree_prev(struct range_tree_node* node) {
  while (1) {
    struct range_tree_node* parent = node->parent;
    if (!parent) {
      return NULL;
    }

    if (parent->side[1]==node) {
      node = parent->side[0];
      break;
    }
    
    node = parent;
  }
  
  return range_tree_last(node);
}

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

struct range_tree_node* range_tree_reorder(struct range_tree_node* node) {
  while (node->side[0]->depth-1>node->side[1]->depth && !node->side[0]->buffer) {
    struct range_tree_node* parent = node->parent;
    struct range_tree_node* side = node->side[0];
    //struct range_tree_node* redo = node;
    side->parent = parent;
    node->side[0] = side->side[1];
    side->side[1]->parent = node;
    side->side[1] = node;
    node->parent = side;
    range_tree_exchange(parent, node, side);
    range_tree_update_calc(node);
    node = side;
    range_tree_update_calc(node);
    //range_tree_reorder(redo);
  }

  while (node->side[0]->depth<node->side[1]->depth && !node->side[1]->buffer) {
    struct range_tree_node* parent = node->parent;
    struct range_tree_node* side = node->side[1];
    //struct range_tree_node* redo = node;
    side->parent = parent;
    node->side[1] = side->side[0];
    side->side[0]->parent = node;
    side->side[0] = node;
    node->parent = side;
    range_tree_exchange(parent, node, side);
    range_tree_update_calc(node);
    node = side;
    range_tree_update_calc(node);
    //range_tree_reorder(redo);
  }
  
  return node;
}


struct range_tree_node* range_tree_update(struct range_tree_node* node) {
  int run = 0;
  struct range_tree_node* last = node;
  while (node) {
    last = node;
    run++;
    if (node->buffer) {
      node->depth = 1;
      if (run>1) {
        printf("Buffered update (urks) / range_tree_build_update\r\n");
        break;
      }
      
      node = node->parent;
      continue;
    }
    
    if (!node->side[0] && !node->side[1]) {
      struct range_tree_node* parent = node->parent;
      if (parent) {
        range_tree_exchange(parent, node, NULL);
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
    
    node = range_tree_reorder(node);
    
    last = node;
    node = node->parent;
  }
  
  return last;
}

struct range_tree_node* range_tree_find_line_start(struct range_tree_node* node, int line, int column, file_offset_t* diff, file_offset_t* offset, int* x) {
  file_offset_t location = 0;
  while (node && !node->buffer) {
    if (node->side[0]->lines>=line) {
      node = node->side[0];
    } else {
      line -= node->side[0]->lines;
      location += node->side[0]->length;
      node = node->side[1];
    }
  }
  
  file_offset_t pos = 0;
  if (node && node->buffer) {
    file_offset_t left = node->lines;
    while (line>0 && pos<node->length) {
      if (node->buffer->buffer[node->offset+pos]=='\n') {
        line--;
        left--;
      }
      pos++;
      location++;
    }
    
    if (left==0) {
      while (node && node->column<column && node->column!=-1) {
        if (x) {
          *x = node->column;
        }
        location += node->length-pos;
        pos = 0;
        node = range_tree_next(node);
        if (!node || node->lines!=0) {
          break;
        }
      }
    }
  }
  
  *diff = pos;
  *offset = location;
  return node;
}

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

int range_tree_find_line_offset(struct range_tree_node* node, file_offset_t offset) {
  int line = 0;
  while (node && !node->buffer) {
    if (node->side[0]->length>offset) {
      node = node->side[0];
    } else {
      offset -= node->side[0]->length;
      line += node->side[0]->lines;
      node = node->side[1];
    }
  }
  
  file_offset_t pos = 0;
  if (node && node->buffer) {
    while (offset>0 && pos<node->length) {
      if (node->buffer->buffer[node->offset+pos]=='\n') {
        line++;
      }
      pos++;
      offset--;
    }
  }
  
  return line;
}

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

int range_tree_simple_return(struct range_tree_node* node) {
  if (!node || !node->buffer) {
    return 0;
  }  

  return (node->buffer->length==1 && node->buffer->buffer[node->offset]=='\n')?1:0;
}

void range_tree_invalidate_renderinfo(struct range_tree_node* node) {
  node->column = -1;
  struct range_tree_node* next = range_tree_next(node);
  while (next && next->column!=-1) {
    next->column = -1;
    if (next->lines!=0) {
      break;
    }
    next = range_tree_next(next);
  }

  struct range_tree_node* prev = range_tree_next(node);
  while (prev && prev->column!=-1) {
    prev->column = -1;
    if (prev->lines!=0) {
      break;
    }
    prev = range_tree_next(prev);
  }
}

void range_tree_retext(struct range_tree_node* node, struct file_type* type) {
  if (!node || !node->buffer) {
    return;
  }

  if (node->buffer->count==1 && (node->offset!=0 || node->length!=node->buffer->length) && node->length>0) {
    char* text = malloc(node->length);
    memcpy(text, node->buffer->buffer+node->offset, node->length);
    free(node->buffer->buffer);
    node->buffer->buffer = text;
    node->buffer->length = node->length;
    node->offset = 0;
  }
  
  const char* text = node->buffer->buffer+node->offset;
  const char* end = node->buffer->buffer+node->offset+node->length;
  file_offset_t lines = 0;
  file_offset_t characters = 0;
  while (text!=end) {
    characters++;
    int cp = -1;
    text = utf8_decode(&cp, text, end-text, 1);
    if (cp==-1) {
      cp = 0xfffd;
    }

    if (cp=='\n') {
      lines++;
    }
  }

  node->lines = lines;
  node->characters = characters;
  node->column = -1;
}

struct range_tree_node* range_tree_compact(struct range_tree_node* root, struct file_type* type, struct range_tree_node* first, struct range_tree_node* last) {
  if (!first) {
    first = range_tree_first(root);
  }

  if (!last) {
    last = range_tree_last(root);
  }
  
  while (first!=last) {
    struct range_tree_node* next = range_tree_next(first);
    
    if (((!range_tree_simple_return(first) && !range_tree_simple_return(next)) || first->length+next->length<TREE_BLOCK_LENGTH_MIN) && !(first->inserter&TIPPSE_INSERTER_READONLY) && !(next->inserter&TIPPSE_INSERTER_READONLY)) {
      if (first->length+next->length<TREE_BLOCK_LENGTH_MAX) {
        char* copy = (char*)malloc(first->length+next->length);
        memcpy(copy, first->buffer->buffer+first->offset, first->length);
        memcpy(copy+first->length, next->buffer->buffer+next->offset, next->length);
        struct fragment* buffer = fragment_create(copy, first->length+next->length);
        fragment_dereference(next->buffer);
        next->buffer = buffer;
        next->length = buffer->length;
        next->offset = 0;

        range_tree_retext(next, type);
        range_tree_update(next);

        fragment_dereference(first->buffer);
        first->buffer = NULL;
        range_tree_retext(first, type);
        root = range_tree_update(first);
      }
    }

    first = next;
  }
  
  return root;
}

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

    build1 = range_tree_new_node(NULL, NULL, NULL, buffer, buffer_offset, buffer_length, inserter);
    if (split==node->length && ((node->inserter&TIPPSE_INSERTER_AFTER) || (inserter&TIPPSE_INSERTER_AUTO))) {
      build0 = range_tree_new_node(node->parent, node, build1, NULL, 0, 0, 0);
      build1->parent = build0;
      node->parent = build0;
      range_tree_exchange(build0->parent, node, build0);  
      range_tree_retext(build1, type);
      range_tree_retext(node, type);
      range_tree_update(build1);
      root = range_tree_update(node);
      before = range_tree_prev(build1);
      after = range_tree_next(node);
    } else if (split==0 && ((node->inserter&TIPPSE_INSERTER_BEFORE) || (inserter&TIPPSE_INSERTER_AUTO) || (prev && (node->inserter&TIPPSE_INSERTER_AFTER)))) {
      build0 = range_tree_new_node(node->parent, build1, node, NULL, 0, 0, 0);
      build1->parent = build0;
      node->parent = build0;
      range_tree_exchange(build0->parent, node, build0);  
      range_tree_retext(build1, type);
      range_tree_retext(node, type);
      range_tree_update(build1);
      root = range_tree_update(node);
      before = range_tree_prev(build1);
      after = range_tree_next(node);
    } else if (!(node->inserter&TIPPSE_INSERTER_READONLY) || (inserter&TIPPSE_INSERTER_AUTO)) {
      build2 = range_tree_new_node(NULL, NULL, NULL, node->buffer, node->offset, split, node->inserter);
      build0 = range_tree_new_node(NULL, build2, build1, NULL, 0, 0, 0);
      build3 = range_tree_new_node(node->parent, build0, node, NULL, 0, 0, 0);
      build2->parent = build0;
      build1->parent = build0;
      build0->parent = build3;
      node->parent = build3;
      range_tree_exchange(build3->parent, node, build3);
      node->offset += split;
      node->length -= split;

      range_tree_retext(build2, type);
      range_tree_retext(build1, type);
      range_tree_retext(node, type);
      range_tree_update(build2);
      range_tree_update(build1);
      root = range_tree_update(node);
      before = range_tree_prev(build2);
      after = range_tree_next(node);
    } else {
      before = node;
      after = node;
    }

    root = range_tree_compact(root, type, before, after);
  } else {
    root = range_tree_new_node(NULL, NULL, NULL, buffer, buffer_offset, buffer_length, inserter);
    range_tree_retext(root, type);
  }
  
  fragment_dereference(buffer);
  
  return root;
}

struct range_tree_node* range_tree_insert_split(struct range_tree_node* root, struct file_type* type, file_offset_t offset, const char* text, size_t length, int inserter, struct range_tree_node** inserts) {
  file_offset_t pos = 0;
  file_offset_t old = 0;
  int split = 0;
  while (1) {
    if (pos==length || pos-old>TREE_BLOCK_LENGTH_MAX || (text[pos]=='\n' && pos-old>TREE_BLOCK_LENGTH_MIN) || split || ((inserter&TIPPSE_INSERTER_ESCAPE) && text[pos]=='\x7f')) {
      split = (text[pos]=='\n')?1:0;
      char* copy = (char*)malloc(pos-old);
      memcpy(copy, text+old, pos-old);
      if (inserts) {
        *inserts = range_tree_last(root);
        inserts++;
      }

      struct fragment* buffer = fragment_create(copy, pos-old);
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

struct range_tree_node* range_tree_delete(struct range_tree_node* root, struct file_type* type, file_offset_t offset, file_offset_t length, int inserter) {
  struct range_tree_node* build0;
  struct range_tree_node* build1;

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
        build1 = range_tree_new_node(NULL, NULL, NULL, node->buffer, split+length+node->offset, node->length-split-length, node->inserter);
        build0 = range_tree_new_node(node->parent, node, build1, NULL, 0, 0, 0);
        build1->parent = build0;
        node->parent = build0;
        node->length = split;
        range_tree_exchange(build0->parent, node, build0);
        range_tree_retext(node, type);
        range_tree_retext(build1, type);
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
    
    range_tree_retext(node, type);
    root = range_tree_update(node);
    if (root) {
      root = range_tree_compact(root, type, before, after);
    }
  }

  return root;
}

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

    build1 = range_tree_new_node(NULL, NULL, NULL, node->buffer, node->offset+split, split2, node->inserter);
    split = 0;
    if (last) {
      build0 = range_tree_new_node(last->parent, last, build1, NULL, 0, 0, 0);
      build1->parent = build0;
      last->parent = build0;
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

