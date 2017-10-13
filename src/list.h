#ifndef TIPPSE_LIST_H
#define TIPPSE_LIST_H

#include <stdlib.h>
#include <stdio.h>

struct list_node {
  struct list_node* next;
  struct list_node* prev;
  void* object;
};

struct list {
  struct list_node* first;
  struct list_node* last;
  size_t count;
};

struct list* list_create(void);
void list_destroy(struct list* list);
struct list_node* list_insert(struct list* list, struct list_node* prev, void* object);
void* list_remove(struct list* list, struct list_node* node);

#endif /* #ifndef TIPPSE_LIST_H */
