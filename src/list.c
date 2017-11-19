// Tippse - List - Simple double linked list

#include "list.h"

struct list* list_create(void) {
  struct list* base = malloc(sizeof(struct list));
  base->first = NULL;
  base->last = NULL;
  base->count = 0;
  return base;
}

void list_destroy(struct list* base) {
  if (base->count!=0) {
    printf("list not empty\r\n");
  }

  free(base);
}

struct list_node* list_insert(struct list* base, struct list_node* prev, void* object) {
  struct list_node* node = malloc(sizeof(struct list_node));
  node->object = object;
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

  return node;
}

void* list_remove(struct list* base, struct list_node* node) {
  void* object = node->object;

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
  free(node);
  return object;
}

