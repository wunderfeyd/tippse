#ifndef __TIPPSE_FILECACHE__
#define __TIPPSE_FILECACHE__

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include "types.h"

struct file_cache;
struct file_cache_node;

#define FILE_CACHE_NODES 16384

#include "rangetree.h"

struct file_cache_node {
  uint8_t buffer[TREE_BLOCK_LENGTH_MAX];
  struct file_cache_node** reference;
  file_offset_t offset;
  size_t length;
  struct file_cache_node* next;
  struct file_cache_node* prev;
};

struct file_cache {
  char* filename;
  int count;

  struct file_cache_node nodes[FILE_CACHE_NODES];
  struct file_cache_node* open[FILE_CACHE_NODES];
  size_t left;
  struct file_cache_node* first;
  struct file_cache_node* last;
};

struct file_cache* file_cache_create(const char* filename);
void file_cache_reference(struct file_cache* base);
void file_cache_dereference(struct file_cache* base);

struct file_cache_node* file_cache_acquire_node(struct file_cache* base);
void file_cache_release_node(struct file_cache* base, struct file_cache_node* node);
void file_cache_unlink_node(struct file_cache* base, struct file_cache_node* node);
void file_cache_link_node(struct file_cache* base, struct file_cache_node* node);
uint8_t* file_cache_use_node(struct file_cache* base, struct file_cache_node** reference, file_offset_t offset, size_t length);
#endif  /* #ifndef __TIPPSE_FILECACHE__ */
