/* Tippse - List - Simple double linked list */

#include "list.h"

struct list* list_create(void) {
  struct list* list = malloc(sizeof(struct list));
  list->first = NULL;
  list->last = NULL;
  list->count = 0;
  return list;
}

void list_destroy(struct list* list) {
  if (list->count!=0) {
    printf("list not empty\r\n");
  }

  free(list);
}

struct list_node* list_insert(struct list* list, struct list_node* prev, void* object) {
  struct list_node* node = malloc(sizeof(struct list_node));
  node->object = object;
  if (prev==NULL) {
    node->prev = NULL;
    node->next = list->first;
    if (node->next) {
      node->next->prev = node;
    }

    if (!list->last) {
      list->last = node;
    }

    list->first = node;
  } else {
    node->prev = prev;
    node->next = prev->next;
    prev->next = node;
    if (node->next) {
      node->next->prev = node;
    } else {
      list->last = node;
    }
  }

  list->count++;

  return node;
}

void* list_remove(struct list* list, struct list_node* node) {
  void* object = node->object;

  if (list->first==node) {
    list->first = node->next;
  }

  if (list->last==node) {
    list->last = node->prev;
  }

  if (node->next) {
    node->next->prev = node->prev;
  }

  if (node->prev) {
    node->prev->next = node->next;
  }

  list->count--;
  free(node);
  return object;
}

