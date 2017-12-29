#ifndef TIPPSE_FILECACHE_H
#define TIPPSE_FILECACHE_H

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

// One megabyte of cache
#define FILE_CACHE_NODES (((1024*1024)/TREE_BLOCK_LENGTH_MAX)+1)

#include "rangetree.h"

struct file_cache_node {
  uint8_t buffer[TREE_BLOCK_LENGTH_MAX];    // buffer
  struct file_cache_node** reference;       // reference node used for linking to caller
  file_offset_t offset;                     // file offset
  size_t length;                            // length of buffer
  struct file_cache_node* next;             // next node
  struct file_cache_node* prev;             // previous node
};

struct file_cache {
  char* filename;                           // name of file
  int fd;                                   // file descriptor
  int count;                                // reference counter
  time_t modification_time;                 // modification time of file during load

  struct file_cache_node* nodes[FILE_CACHE_NODES]; // nodes pool
  struct file_cache_node* open[FILE_CACHE_NODES]; // nodes unused
  size_t allocated;                         // position of first allocated node in pool
  size_t left;                              // number of unused nodes
  struct file_cache_node* first;            // first used node
  struct file_cache_node* last;             // last used node
  struct file_cache_node* ranged[FILE_CACHE_NODES]; // nodes within last offset
};

struct file_cache* file_cache_create(const char* filename);
void file_cache_reference(struct file_cache* base);
void file_cache_dereference(struct file_cache* base);
int file_cache_modified(struct file_cache* base);

struct file_cache_node* file_cache_acquire_node(struct file_cache* base);
void file_cache_release_node(struct file_cache* base, struct file_cache_node* node);
void file_cache_unlink_node(struct file_cache* base, struct file_cache_node* node);
void file_cache_link_node(struct file_cache* base, struct file_cache_node* node);
uint8_t* file_cache_use_node(struct file_cache* base, struct file_cache_node** reference, file_offset_t offset, size_t length);
uint8_t* file_cache_use_node_ranged(struct file_cache* base, struct file_cache_node** reference, file_offset_t offset, size_t length);
#endif  /* #ifndef TIPPSE_FILECACHE_H */
