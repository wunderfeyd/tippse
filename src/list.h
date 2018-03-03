#ifndef TIPPSE_LIST_H
#define TIPPSE_LIST_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Node of list
struct list_node {
  struct list_node* next;   // Next node in list
  struct list_node* prev;   // Previous node in list
};


// List base object
struct list {
  struct list_node* first;  // First node of list
  struct list_node* last;   // Last node of list
  size_t count;             // Number of nodes in list
  size_t node_size;         // User object size
};

struct list* list_create(size_t node_size);
void list_create_inplace(struct list* base, size_t node_size);
void list_destroy(struct list* base);
void list_destroy_inplace(struct list* base);
inline void* list_object(struct list_node* node) {return (void*)(node+1);}
struct list_node* list_insert_empty(struct list* base, struct list_node* prev);
struct list_node* list_insert(struct list* base, struct list_node* prev, void* object);
void list_remove(struct list* base, struct list_node* node);

#endif /* #ifndef TIPPSE_LIST_H */
