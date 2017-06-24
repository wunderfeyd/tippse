#ifndef __TIPPSE_LIST__
#define __TIPPSE_LIST__

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
  int count;
};

struct list* list_create();
void list_destroy(struct list* list);
void list_insert(struct list* list, struct list_node* prev, void* object);
void* list_remove(struct list* list, struct list_node* node);

#endif /* #ifndef __TIPPSE_LIST__ */
