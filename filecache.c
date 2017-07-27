/* Tippse - File cache - Keep last accessed file pages in memory */

#include "filecache.h"

struct file_cache* file_cache_create(const char* filename) {
  struct file_cache* base = malloc(sizeof(struct file_cache));
  base->filename = strdup(filename);
  base->count = 1;

  size_t count;
  for (count = 0; count<FILE_CACHE_NODES; count++) {
    base->open[count] = &base->nodes[FILE_CACHE_NODES-1-count];
  }

  base->left = FILE_CACHE_NODES;
  base->first = NULL;
  base->last = NULL;
  return base;
}

void file_cache_reference(struct file_cache* base) {
  base->count++;
}

void file_cache_dereference(struct file_cache* base) {
  base->count--;

  if (base->count==0) {
    free(base->filename);
    free(base);
  }
}

struct file_cache_node* file_cache_acquire_node(struct file_cache* base) {
  if (base->left==0) {
    file_cache_release_node(base, base->last);
  }

  base->left--;
  struct file_cache_node* node = base->open[base->left];
  file_cache_link_node(base, node);

  return node;
}

void file_cache_link_node(struct file_cache* base, struct file_cache_node* node) {
  node->next = base->first;
  node->prev = NULL;

  if (base->first) {
    base->first->prev = node;
  }

  base->first = node;

  if (!base->last) {
    base->last = node;
  }
}

void file_cache_release_node(struct file_cache* base, struct file_cache_node* node) {
  if (node->reference) {
    *node->reference = NULL;
  }

  file_cache_unlink_node(base, node);
  base->open[base->left++] = node;
}

void file_cache_unlink_node(struct file_cache* base, struct file_cache_node* node) {
  if (node->prev) {
    node->prev->next = node->next;
  }

  if (node->next) {
    node->next->prev = node->prev;
  }

  if (base->last==node) {
    base->last = node->prev;
  }

  if (base->first==node) {
    base->first = node->next;
  }
}

uint8_t* file_cache_use_node(struct file_cache* base, struct file_cache_node** reference, file_offset_t offset, size_t length) {
  struct file_cache_node* node = *reference;
  if (node) {
    file_cache_unlink_node(base, node);
    file_cache_link_node(base, node);
    node->reference = reference;
  } else {
    node = file_cache_acquire_node(base);
    *reference = node;
    node->offset = ~0;
    node->length = 0;
    node->reference = reference;
  }

  if (node->offset!=offset || node->length!=length) {
    if (length>TREE_BLOCK_LENGTH_MAX) {
      // Woops
      printf("\r\nTried to allocate a page larger than the set up maximum\r\n");
      abort();
    }

    node->offset = offset;
    node->length = length;

    int f = open(base->filename, O_RDONLY);
    if (f!=-1) {
      lseek(f, (off_t)node->offset, SEEK_SET);
      read(f, &node->buffer[0], node->length);
      close(f);
    }
  }

  return &node->buffer[0];
}
