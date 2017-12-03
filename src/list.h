#ifndef TIPPSE_LIST_H
#define TIPPSE_LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct list_node {
  struct list_node* next;
  struct list_node* prev;
};

struct list {
  struct list_node* first;
  struct list_node* last;
  size_t count;
  size_t node_size;
};

struct list* list_create(size_t node_size);
void list_destroy(struct list* base);
inline void* list_object(struct list_node* node) {return (void*)(node+1);}
struct list_node* list_insert_empty(struct list* base, struct list_node* prev);
struct list_node* list_insert(struct list* base, struct list_node* prev, void* object);
void list_remove(struct list* base, struct list_node* node);

#endif /* #ifndef TIPPSE_LIST_H */
