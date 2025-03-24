// Tippse - Range tree - For fast access to various attributes of the document

#include "rangetree.h"

#include "fragment.h"
#include "stream.h"

int64_t range_tree_node_fuse_id = 1;

struct range_tree* range_tree_create(struct range_tree_callback* callback, int caps) {
  struct range_tree* base = (struct range_tree*)malloc(sizeof(struct range_tree));
  range_tree_create_inplace(base, callback, caps);
  return base;
}

void range_tree_create_inplace(struct range_tree* base, struct range_tree_callback* callback, int caps) {
  base->root = NULL;
  base->callback = callback;
  base->caps = caps;
}

void range_tree_destroy(struct range_tree* base) {
  range_tree_destroy_inplace(base);
  free(base);
}

void range_tree_destroy_inplace(struct range_tree* base) {
  if (base->root) {
    range_tree_node_destroy(base->root, base);
    base->root = NULL;
    base->callback = NULL;
  }
}

// Allocate range tree node
struct range_tree_node* range_tree_invoke(struct range_tree* base) {
  struct range_tree_node* node = (struct range_tree_node*)malloc(sizeof(struct range_tree_node));
  return node;
}

// Deallocate range tree node
void range_tree_revoke(struct range_tree* base, struct range_tree_node* node) {
  if (base->callback) {
    (*base->callback->node_destroy)(base->callback, node, base);
  }

  if ((base->caps&TIPPSE_RANGETREE_CAPS_DEALLOCATE_USER_DATA)) {
    free(node->user_data);
    node->user_data = NULL;
  }

  free(node);
}

// Reallocate the fragment if it is referenced only once and had become smaller due to the edit process (save memory)
void range_tree_node_invalidate(struct range_tree_node* node, struct range_tree* base) {
  if (node->buffer) {
    if (base->callback) {
      (*base->callback->node_invalidate)(base->callback, node, base);
    }

    if (node->buffer->count==1 && (node->offset!=0 || node->length!=node->buffer->length) && node->length>0) {
      if (node->buffer->type==FRAGMENT_MEMORY) {
        uint8_t* text = (uint8_t*)malloc(node->length);
        memcpy(text, node->buffer->buffer+node->offset, node->length);
        free(node->buffer->buffer);
        node->buffer->buffer = text;
        node->buffer->length = node->length;
        node->offset = 0;
      }
    }
  }
}

// Reallocate the fragment if file cache is going to be removed
void range_tree_cache_invalidate(struct range_tree* base, struct file_cache* cache) {
  if (!base || !base->root || !(base->root->inserter&TIPPSE_INSERTER_FILE)) {
    return;
  }

  range_tree_node_cache_invalidate(base->root, base, cache);
}

// Reallocate the fragment if file cache is going to be removed recursively
void range_tree_node_cache_invalidate(struct range_tree_node* node, struct range_tree* base, struct file_cache* cache) {
  if (!node) {
    return;
  }

  if (node->buffer && node->buffer->type==FRAGMENT_FILE) {
    if (!cache || node->buffer->cache==cache) {
      struct stream stream;
      stream_from_page(&stream, node, 0);
      size_t length = stream_cache_length(&stream);
      uint8_t* buffer = (uint8_t*)malloc(length*sizeof(uint8_t));
      memcpy(buffer, stream_buffer(&stream), length);
      struct fragment* fragment = fragment_create_memory(buffer, length);
      stream_destroy(&stream);

      fragment_dereference(node->buffer, base->callback);
      node->buffer = fragment;
      node->offset = 0;
    }
  }

  range_tree_node_cache_invalidate(node->side[0], base, cache);
  range_tree_node_cache_invalidate(node->side[1], base, cache);
}

// Try to merge range of nodes which are below the maximum page size
void range_tree_fuse(struct range_tree* base, struct range_tree_node* first, struct range_tree_node* last) {
  if (!first) {
    first = range_tree_first(base);
  }

  if (!last) {
    last = range_tree_last(base);
  }

  while (first!=last) {
    struct range_tree_node* next = range_tree_node_next(first);

    if (first->inserter==next->inserter && (!(first->inserter&TIPPSE_INSERTER_NOFUSE) || first->fuse_id==next->fuse_id)) {
      if (!first->buffer && !next->buffer) {
        next->length = first->length+next->length;
        range_tree_node_update(next, base);
        first->inserter &= ~TIPPSE_INSERTER_LEAF;
        range_tree_node_update(first, base);
      } else if (first->length+next->length<TREE_BLOCK_LENGTH_MIN) {
        if (first->buffer && next->buffer && (first->buffer->type==FRAGMENT_MEMORY && next->buffer->type==FRAGMENT_MEMORY)) {

          struct stream stream_first;
          stream_from_page(&stream_first, first, 0);
          size_t length_first = stream_cache_length(&stream_first);

          struct stream stream_next;
          stream_from_page(&stream_next, next, 0);
          size_t length_next = stream_cache_length(&stream_next);

          uint8_t* copy = (uint8_t*)malloc(length_first+length_next);
          memcpy(copy, stream_buffer(&stream_first), length_first);
          memcpy(copy+length_first, stream_buffer(&stream_next), length_next);

          stream_destroy(&stream_next);
          stream_destroy(&stream_first);

          struct fragment* buffer = fragment_create_memory(copy, length_first+length_next);
          fragment_dereference(first->buffer, base->callback);

          first->buffer = buffer;
          first->length = buffer->length;
          first->offset = 0;

          range_tree_node_invalidate(first, base);
          range_tree_node_update(first, base);

          if (next==last) {
            last = first;
          }
          fragment_dereference(next->buffer, base->callback);
          next->buffer = NULL;
          next->inserter &= ~TIPPSE_INSERTER_LEAF;
          range_tree_node_update(next, base);
          continue;
        }
      }
    }

    first = next;
  }
}

// Insert fragment into specific offset and eventually break older nodes into parts
void range_tree_insert(struct range_tree* base, file_offset_t offset, struct fragment* buffer, file_offset_t buffer_offset, file_offset_t buffer_length, int inserter, int64_t fuse_id, void* user_data) {
  struct range_tree_node* build1 = range_tree_node_create(NULL, base, NULL, NULL, buffer, buffer_offset, buffer_length, inserter|TIPPSE_INSERTER_LEAF, fuse_id, user_data);
  build1->depth = 1;

  if (!base->root) {
    range_tree_node_invalidate(build1, base);
    base->root = build1;
    return;
  }

  file_offset_t split;
  struct range_tree_node* node = range_tree_node_find_offset(base->root, offset, &split);
  if (split>node->length) {
    fprintf(stderr, "Tried to append a new node past the end... Please file a bug report!");
    abort();
  }

  if (split>0 && split<node->length) {
    range_tree_split(base, &node, split, 1);
    split = 0;
  }

  struct range_tree_node* build0;
  if (split==node->length) {
    build0 = range_tree_node_create(node->parent, base, node, build1, NULL, 0, 0, 0, 0, NULL);

    build1->next = node->next;
    if (node->next) {
      node->next->prev = build1;
    }

    node->next = build1;
    build1->prev = node;
  } else {
    build0 = range_tree_node_create(node->parent, base, build1, node, NULL, 0, 0, 0, 0, NULL);

    build1->prev = node->prev;
    if (node->prev) {
      node->prev->next = build1;
    }

    node->prev = build1;
    build1->next = node;
  }

  build1->parent = build0;
  node->parent = build0;

  range_tree_node_exchange(build0->parent, node, build0);
  range_tree_node_invalidate(build1, base);
  range_tree_node_invalidate(node, base);
  range_tree_node_update(build0, base);
  range_tree_fuse(base, range_tree_node_prev(build1), range_tree_node_next(node));
}

// Split input buffer into nodes with maximum allowed length
void range_tree_insert_split(struct range_tree* base, file_offset_t offset, const uint8_t* text, size_t length, int inserter) {
  for (size_t pos = 0; pos<length; pos+=TREE_BLOCK_LENGTH_MID) {
    size_t size = length-pos;
    if (size>TREE_BLOCK_LENGTH_MID) {
      size = TREE_BLOCK_LENGTH_MID;
    }

    uint8_t* copy = (uint8_t*)malloc(size);
    memcpy(copy, text+pos, size);
    struct fragment* buffer = fragment_create_memory(copy, size);
    range_tree_node_fuse_id++;
    range_tree_insert(base, offset, buffer, 0, buffer->length, inserter, range_tree_node_fuse_id, NULL);
    fragment_dereference(buffer, NULL);

    offset += TREE_BLOCK_LENGTH_MID;
  }
}

// Remove specific range from base (eventually break nodes into parts)
void range_tree_delete(struct range_tree* base, file_offset_t offset, file_offset_t length, int inserter) {
  while (length>0) {
    file_offset_t split = 0;
    struct range_tree_node* node = range_tree_node_find_offset(base->root, offset, &split);
    if (!node) {
      return;
    }

    range_tree_split(base, &node, split, 1);
    if (node->length>length) {
      struct range_tree_node* next = node;
      range_tree_split(base, &next, length, 1);
    }

    struct range_tree_node* before = range_tree_node_prev(node);
    struct range_tree_node* after = range_tree_node_next(node);

    length -= node->length;

    if (node->buffer) {
      fragment_dereference(node->buffer, base->callback);
      node->buffer = NULL;
    }
    node->inserter &= ~TIPPSE_INSERTER_LEAF;

    range_tree_node_update(node, base);
    if (before) {
      range_tree_node_invalidate(before, base);
      range_tree_node_update(before, base);
    }

    if (after) {
      range_tree_node_invalidate(after, base);
      range_tree_node_update(after, base);
    }

    if (base->root) {
      range_tree_fuse(base, before, after);
    }

    if (!after) {
      break;
    }
  }
}

// Create copy of a specific range
struct range_tree* range_tree_copy(struct range_tree* base, file_offset_t offset, file_offset_t length, struct range_tree_callback* callback) {
  struct range_tree* copy = range_tree_create(callback, 0);
  range_tree_node_copy_insert(base->root, offset, copy, 0, length);
  return copy;
}

// Insert already built nodes
void range_tree_paste(struct range_tree* base, struct range_tree_node* copy, file_offset_t offset) {
  copy = range_tree_node_first(copy);
  while (copy) {
    range_tree_insert(base, offset, copy->buffer, copy->offset, copy->length, copy->inserter, copy->fuse_id, copy->user_data);
    offset += copy->length;
    copy = range_tree_node_next(copy);
  }
}

// Copy specific range from base into a buffer
uint8_t* range_tree_raw(struct range_tree* base, file_offset_t start, file_offset_t end) {
  if (!base->root || end<=start) {
    return (uint8_t*)strdup("");
  }

  file_offset_t length = end-start;
  uint8_t* text = (uint8_t*)malloc(sizeof(uint8_t)*(length+1));
  uint8_t* out = text;

  file_offset_t split = 0;
  struct range_tree_node* node = range_tree_node_find_offset(base->root, start, &split);

  struct stream stream;
  stream_from_page(&stream, node, split);

  while (length>0) {
    size_t segment = stream_cache_length(&stream)-stream_displacement(&stream);
    if (segment>length) {
      segment = length;
    }

    memcpy(out, stream_buffer(&stream), segment);
    length -= segment;
    out += segment;
    stream_next(&stream);
  }
  stream_destroy(&stream);

  *out = '\0';
  return text;
}

// Cleanup base
void range_tree_empty(struct range_tree* base) {
  if (base->root) {
    range_tree_node_destroy(base->root, base);
    base->root = NULL;
  }
}

// Cleanup base and build single full length node
void range_tree_static(struct range_tree* base, file_offset_t length, int inserter) {
  range_tree_empty(base);

  if (length==0) {
    return;
  }

  base->root = range_tree_node_create(NULL, base, NULL, NULL, NULL, 0, length, inserter|TIPPSE_INSERTER_LEAF, 0, NULL);
  range_tree_node_update(base->root, base);
}

// Resize base to match length
void range_tree_resize(struct range_tree* base, file_offset_t length, int inserter) {
  if (!base->root || base->root->length==0) {
    range_tree_static(base, length, inserter);
  } else if (base->root->length>length) {
    range_tree_reduce(base, length, base->root->length-length);
  } else if (base->root->length<length) {
    file_offset_t last = base->root->length;
    range_tree_expand(base, last, length-last);
    range_tree_mark(base, last, length-last, inserter);
  }
}

// Stretch node at offset by given length
void range_tree_expand(struct range_tree* base, file_offset_t offset, file_offset_t length) {
  file_offset_t split = 0;
  struct range_tree_node* node = range_tree_node_find_offset(base->root, offset, &split);
  if (node) {
    node->length += length;
    range_tree_node_update(node, base);
  } else {
    range_tree_resize(base, length, 0);
  }
}

// Remove nodes at offset until offset+length
void range_tree_reduce(struct range_tree* base, file_offset_t offset, file_offset_t length) {
  range_tree_delete(base, offset, length, 0);
}

// Split node into upper and lower parts
void range_tree_split(struct range_tree* base, struct range_tree_node** node, file_offset_t split, int invalidate) {
  if (*node && split>0 && split<(*node)->length) {
    struct range_tree_node* build1 = range_tree_node_create(NULL, base, NULL, NULL, (*node)->buffer, (*node)->offset+split, (*node)->length-split, (*node)->inserter, (*node)->fuse_id, (*node)->user_data);
    struct range_tree_node* build0 = range_tree_node_create((*node)->parent, base, *node, build1, NULL, 0, 0, 0, 0, NULL);
    (*node)->length = split;

    build1->parent = build0;
    (*node)->parent = build0;

    if ((*node)->next) {
      (*node)->next->prev = build1;
    }

    build1->next = (*node)->next;
    (*node)->next = build1;
    build1->prev = *node;

    range_tree_node_exchange(build0->parent, (*node), build0);
    if (invalidate) {
      range_tree_node_invalidate(*node, base);
      range_tree_node_invalidate(build1, base);
    }
    range_tree_node_update(build1, base);
    range_tree_node_update(*node, base);
    *node = build1;
  }
}

// Change marked attribute from offset to offset+length
void range_tree_mark(struct range_tree* base, file_offset_t offset, file_offset_t length, int inserter) {
  if (length==0) {
    return;
  }

  file_offset_t split = 0;

  struct range_tree_node* first = range_tree_node_find_offset(base->root, offset, &split);
  struct range_tree_node* before = range_tree_node_prev(first);
  if (!before) {
    before = first;
  }

  range_tree_split(base, &first, split, 0);

  struct range_tree_node* after = NULL;
  struct range_tree_node* last = range_tree_node_find_offset(base->root, offset+length, &split);
  if (offset+length<range_tree_length(base)) {
    after = last;
    range_tree_split(base, &after, split, 0);
  }

  range_tree_node_fuse_id++;

  while (1) {
    first->inserter = inserter|TIPPSE_INSERTER_LEAF;
    first->fuse_id = range_tree_node_fuse_id;
    range_tree_node_update(first, base);
    if (first==after) {
      break;
    }

    first = range_tree_node_next(first);
    if (first==after) {
      break;
    }
  }

  range_tree_fuse(base, before, last);
}

// Return next marked block after/at offset given
struct range_tree_node* range_tree_marked_next(struct range_tree* base, file_offset_t offset, file_offset_t* low, file_offset_t* high, int skip_first) {
  if (base->root && offset<range_tree_length(base)) {
    file_offset_t displacement;
    struct range_tree_node* node = range_tree_node_find_offset(base->root, offset, &displacement);
    offset -= displacement;
    while (node) {
      if (node->inserter&TIPPSE_INSERTER_MARK && !skip_first) {
        *low = offset;
        *high = offset+node->length;
        return node;
      }
      skip_first = 0;
      offset += node->length;
      node = range_tree_node_next(node);
    }
  }

  *low = FILE_OFFSET_T_MAX;
  *high = FILE_OFFSET_T_MAX;
  return NULL;
}

// Return previous marked block before/at offset given
struct range_tree_node* range_tree_marked_prev(struct range_tree* base, file_offset_t offset, file_offset_t* low, file_offset_t* high, int skip_first) {
  if (base->root && (offset>0 || !skip_first)) {
    file_offset_t displacement;
    struct range_tree_node* node = range_tree_node_find_offset(base->root, offset, &displacement);
    offset -= displacement;
    while (node) {
      if (node->inserter&TIPPSE_INSERTER_MARK && !skip_first) {
        *low = offset;
        *high = offset+node->length;
        return node;
      }
      skip_first = 0;
      node = range_tree_node_prev(node);
      if (node) {
        offset -= node->length;
      }
    }
  }

  *low = FILE_OFFSET_T_MAX;
  *high = FILE_OFFSET_T_MAX;
  return NULL;
}


// Debug: Recursively print tree nodes
void range_tree_node_print(const struct range_tree_node* node, int depth, int side) {
  int tab = depth;
  while (tab>0) {
    fprintf(stderr, "  ");
    tab--;
  }

  fprintf(stderr, "%d %5d %s(%p-%p) %5d %5d (%p) (%x)", side, (int)node->length, node->buffer?"B":" ", (void*)node->buffer, (void*)(node->buffer?node->buffer->buffer:NULL), (int)node->offset, node->depth, (void*)node, node->inserter);
  fprintf(stderr, "\r\n");
  if (node->side[0]) {
    range_tree_node_print(node->side[0], depth+1, 0);
  }

  if (node->side[1]) {
    range_tree_node_print(node->side[1], depth+1, 1);
  }
}

// Debug: Check tree for consistency (TODO: Next/Prev fields are not covered)
void range_tree_node_check(const struct range_tree_node* node) {
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
    range_tree_node_check(node->side[0]);
  }

  if (node->side[1]) {
    range_tree_node_check(node->side[1]);
  }

  if (node->buffer->type==FRAGMENT_FILE) {
    if (!(node->inserter&TIPPSE_INSERTER_FILE)) {
      printf("fragment from file not marked\r\n");
    }
  } else {
    if ((node->inserter&TIPPSE_INSERTER_FILE)) {
      printf("fragment from memory not marked\r\n");
    }
  }
}

// Debug: Search root and print
void range_tree_node_print_root(const struct range_tree_node* node, int depth, int side) {
  while(node->parent) {
    node = node->parent;
  }

  range_tree_node_print(node, depth, side);
}

// Combine statistics of child nodes (on demand)
void range_tree_node_update_lazy(struct range_tree_node* node, struct range_tree* tree) {
  if ((node->inserter&TIPPSE_INSERTER_LAZY_COMBINE)!=0) {
    node->inserter &= ~TIPPSE_INSERTER_LAZY_COMBINE;
    range_tree_node_update_lazy(node->side[0], tree);
    range_tree_node_update_lazy(node->side[1], tree);
    if (tree->callback) {
      (*tree->callback->node_combine)(tree->callback, node, tree);
    }
  }
}

// Combine statistics of child nodes
void range_tree_node_update_calc(struct range_tree_node* node, struct range_tree* tree) {
  if (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    node->length = node->side[0]->length+node->side[1]->length;
    node->depth = ((node->side[0]->depth>node->side[1]->depth)?node->side[0]->depth:node->side[1]->depth)+1;
    node->inserter = (((node->side[0]?node->side[0]->inserter:0)|(node->side[1]?node->side[1]->inserter:0))&~TIPPSE_INSERTER_LEAF)|TIPPSE_INSERTER_LAZY_COMBINE;
    //node->inserter = ((node->side[0]?node->side[0]->inserter:0)|(node->side[1]?node->side[1]->inserter:0))&~TIPPSE_INSERTER_LEAF;
    //if (tree->callback) {
    //  (*tree->callback->node_combine)(tree->callback, node, tree);
    //}
  }
}

// Recursively recalc all nodes on path up to the root node
void range_tree_node_update_calc_all(struct range_tree_node* node, struct range_tree* tree) {
  range_tree_node_update_calc(node, tree);
  if (node->parent) {
    range_tree_node_update_calc_all(node->parent, tree);
  }
}

// Remove node and all children
void range_tree_node_destroy(struct range_tree_node* node, struct range_tree* tree) {
  if (!node) {
    return;
  }

  range_tree_node_destroy(node->side[0], tree);
  range_tree_node_destroy(node->side[1], tree);

  if (node->buffer) {
    fragment_dereference(node->buffer, tree->callback);
  }

  range_tree_revoke(tree, node);
}

// Create node with given fragment
struct range_tree_node* range_tree_node_create(struct range_tree_node* parent, struct range_tree* tree, struct range_tree_node* side0, struct range_tree_node* side1, struct fragment* buffer, file_offset_t offset, file_offset_t length, int inserter, int64_t fuse_id, void* user_data) {
  struct range_tree_node* node = range_tree_invoke(tree);
  node->parent = parent;
  node->side[0] = side0;
  node->side[1] = side1;
  node->buffer = buffer;
  node->next = NULL;
  node->prev = NULL;
  if (node->buffer) {
    fragment_reference(node->buffer, tree->callback);
    if (node->buffer->type==FRAGMENT_FILE) {
      inserter |= TIPPSE_INSERTER_FILE;
    }
  }

  node->offset = offset;
  node->length = length;
  node->inserter = inserter;
  node->depth = 0;
  node->fuse_id = fuse_id;
  node->user_data = user_data;
  node->visuals = NULL;
  node->view_uid = 0;
  return node;
}

// Find first leaf
struct range_tree_node* range_tree_node_first(struct range_tree_node* node) {
  if (!node) {
    return NULL;
  }

  while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    node = node->side[0];
  }

  return node;
}

// Find last leaf
struct range_tree_node* range_tree_node_last(struct range_tree_node* node) {
  if (!node) {
    return NULL;
  }

  while (!(node->inserter&TIPPSE_INSERTER_LEAF)) {
    node = node->side[1];
  }

  return node;
}

// Exchange child node to another one
void range_tree_node_exchange(struct range_tree_node* node, struct range_tree_node* old, struct range_tree_node* update) {
  if (!node) {
    return;
  }

  if (node->side[0]==old) {
    node->side[0] = update;
  } else if (node->side[1]==old) {
    node->side[1] = update;
  } else {
    printf("umm ... dead parent %p old %p update %p side 0 %p side 1 %p\r\n", (void*)node, (void*)old, (void*)update, (void*)node->side[0], (void*)node->side[1]);
  }
}

// Rotate children left or right depending on side
struct range_tree_node* range_tree_node_rotate(struct range_tree_node* node, struct range_tree* tree, int side) {
  struct range_tree_node* parent = node->parent;
  struct range_tree_node* child = node->side[side];
  child->parent = parent;
  node->side[side] = child->side[side^1];
  child->side[side^1]->parent = node;
  child->side[side^1] = node;
  node->parent = child;
  range_tree_node_exchange(parent, node, child);
  range_tree_node_update_calc(node, tree);
  node = child;
  return node;
}

// Balance tree while left side must not have more depth (+1) than the right side
struct range_tree_node* range_tree_node_balance(struct range_tree_node* node, struct range_tree* tree) {
  while (node->side[0]->depth-1>node->side[1]->depth && !(node->side[0]->inserter&TIPPSE_INSERTER_LEAF)) {
    node = range_tree_node_rotate(node, tree, 0);
  }

  while (node->side[0]->depth<node->side[1]->depth && !(node->side[1]->inserter&TIPPSE_INSERTER_LEAF)) {
    node = range_tree_node_rotate(node, tree, 1);
  }

  range_tree_node_update_calc(node, tree);
  return node;
}

// Update path to root node, throw away useless nodes and call the balancer (root node might change)
void range_tree_node_update(struct range_tree_node* node, struct range_tree* tree) {
  struct range_tree_node* last = node;
  while (node) {
    last = node;
    if (node->inserter&TIPPSE_INSERTER_LEAF) {
      node = node->parent;
      continue;
    }

    if (!node->side[0] && !node->side[1]) {
      struct range_tree_node* parent = node->parent;
      if (parent) {
        range_tree_node_exchange(parent, node, NULL);
      }

      if (node->prev) {
        node->prev->next = node->next;
      }

      if (node->next) {
        node->next->prev = node->prev;
      }

      range_tree_revoke(tree, node);
      last = parent;
      node = parent;
      continue;
    }

    if (!node->side[0] || !node->side[1]) {
      struct range_tree_node* parent = node->parent;
      struct range_tree_node* child = node->side[0]?node->side[0]:node->side[1];

      last = child;
      if (parent) {
        range_tree_node_exchange(parent, node, child);
      }

      child->parent = parent;

      range_tree_revoke(tree, node);
      node = parent;
      continue;
    }

    node = range_tree_node_balance(node, tree);

    last = node;
    node = node->parent;
  }

  tree->root = last;
}

// Return base offset of given node
file_offset_t range_tree_node_offset(const struct range_tree_node* node) {
  file_offset_t offset = 0;
  while (node && node->parent) {
    if (node->parent->side[1]==node) {
      offset += node->parent->side[0]->length;
    }

    node = node->parent;
  }

  return offset;
}

// Search node by offset and return difference to its base offset
struct range_tree_node* range_tree_node_find_offset(struct range_tree_node* node, file_offset_t offset, file_offset_t* diff) {
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

// Create copy of a specific range and insert into another or same tree
void range_tree_node_copy_insert(struct range_tree_node* root_from, file_offset_t offset_from, struct range_tree* tree_to, file_offset_t offset_to, file_offset_t length) {
  file_offset_t split = 0;
  int same = (root_from==tree_to->root)?1:0;
  struct range_tree_node* node = range_tree_node_find_offset(root_from, offset_from, &split);
  while (node && length>0) {
    file_offset_t split2 = node->length-split;
    if (split2>length) {
      split2 = length;
    }

    range_tree_insert(tree_to, offset_to, node->buffer, node->offset+split, split2, node->inserter, node->fuse_id, node->user_data);
    offset_to += split2;
    offset_from += split2;
    length -= split2;
    if (same) {
      node = range_tree_node_find_offset(tree_to->root, offset_from, &split);
    } else {
      split = 0;
      node = range_tree_node_next(node);
    }
  }
}

// Check for marked attribute
int range_tree_node_marked(const struct range_tree_node* node, file_offset_t offset, file_offset_t length, int inserter) {
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
    marked |= range_tree_node_marked(node->side[0], offset, length0, inserter);
  }

  if (length1>0 && !marked) {
    marked |= range_tree_node_marked(node->side[1], (node->side[0]->length>offset)?0:offset-node->side[0]->length, length1, inserter);
  }

  return marked;
}

// Invert given flag on whole tree
struct range_tree_node* range_tree_node_invert_mark(struct range_tree_node* node, struct range_tree* tree, int inserter) {
  if (node->inserter&TIPPSE_INSERTER_LEAF) {
    node->inserter ^= inserter;
    return node;
  }

  range_tree_node_invert_mark(node->side[0], tree, inserter);
  range_tree_node_invert_mark(node->side[1], tree, inserter);
  range_tree_node_update_calc(node, tree);
  return node;
}
