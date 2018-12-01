// Tippse - List - Simple double linked list

#include "list.h"

// Build list
struct list* list_create(size_t node_size) {
  struct list* base = (struct list*)malloc(sizeof(struct list));
  list_create_inplace(base, node_size);
  return base;
}

// Build list in known memory location
void list_create_inplace(struct list* base, size_t node_size) {
  base->first = NULL;
  base->last = NULL;
  base->count = 0;
  base->node_size = node_size;
}

// Destroy list structure
void list_destroy_inplace(struct list* base) {
  if (base->count!=0) {
    printf("list not empty\r\n");
  }
}

// Remove list structure
void list_destroy(struct list* base) {
  list_destroy_inplace(base);
  free(base);
}

// Insert object
struct list_node* list_insert(struct list* base, struct list_node* prev, void* object) {
  struct list_node* node = list_insert_empty(base, prev);
  memcpy(list_object(node), object, base->node_size);
  return node;
}

// Allocate empty element
struct list_node* list_insert_empty(struct list* base, struct list_node* prev) {
  struct list_node* node = (struct list_node*)malloc(sizeof(struct list_node)+base->node_size);
  list_insert_node(base, node, prev);
  return node;
}

// Link the specified node
void list_insert_node(struct list* base, struct list_node* node, struct list_node* prev) {
  if (!prev) {
    node->prev = NULL;
    node->next = base->first;
    if (node->next) {
      node->next->prev = node;
    }

    if (!base->last) {
      base->last = node;
    }

    base->first = node;
  } else {
    node->prev = prev;
    node->next = prev->next;
    prev->next = node;
    if (node->next) {
      node->next->prev = node;
    } else {
      base->last = node;
    }
  }

  base->count++;
}

// Free element
void list_remove(struct list* base, struct list_node* node) {
  list_remove_node(base, node);
  free(node);
}

// Unlink element
void list_remove_node(struct list* base, struct list_node* node) {
  if (base->first==node) {
    base->first = node->next;
  }

  if (base->last==node) {
    base->last = node->prev;
  }

  if (node->next) {
    node->next->prev = node->prev;
  }

  if (node->prev) {
    node->prev->next = node->next;
  }

  base->count--;
}

// Unlink and link node again at different position
void list_move(struct list* base, struct list_node* node, struct list_node* prev) {
  if (node==prev) {
    return;
  }

  list_remove_node(base, node);
  list_insert_node(base, node, prev);
}
