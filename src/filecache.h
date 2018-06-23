#ifndef TIPPSE_FILECACHE_H
#define TIPPSE_FILECACHE_H

#include <stdlib.h>
#include <stdio.h>
#ifdef _WINDOWS
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include "types.h"

struct file_cache;
struct file_cache_node;
struct list_node;

// One megabyte of cache
#define FILE_CACHE_PAGE_SIZE (65536)
#define FILE_CACHE_SIZE (1024*1024)

#include "rangetree.h"
#include "file.h"

struct file_cache_node {
  int count;                                // number of references
  uint8_t* buffer;                          // buffer
  file_offset_t offset;                     // offset in file
  size_t length;                            // length of buffer
  struct list_node* list_node;              // node in list
};

struct file_cache {
  char* filename;                           // name of file
  struct file* fd;                          // file descriptor
  int count;                                // reference counter
#ifdef _WINDOWS
  FILETIME modification_time;               // modification time of file during load
#else
  time_t modification_time;                 // modification time of file during load
#endif

  size_t max;                               // maximum cache size
  size_t size;                              // current cache size
  struct range_tree_node* index;            // index
  struct list active;                       // list of active nodes
  struct list inactive;                     // lru list of inactive nodes
};

struct file_cache* file_cache_create(const char* filename);
void file_cache_reference(struct file_cache* base);
void file_cache_dereference(struct file_cache* base);
int file_cache_modified(struct file_cache* base);

void file_cache_empty(struct file_cache* base, struct list* nodes);
void file_cache_cleanup(struct file_cache* base);
struct file_cache_node* file_cache_invoke(struct file_cache* base, file_offset_t offset, size_t length, const uint8_t** buffer, size_t* buffer_length);
void file_cache_clone(struct file_cache* base, struct file_cache_node* node);
void file_cache_revoke(struct file_cache* base, struct file_cache_node* node);
void file_cache_debug(struct file_cache* base);
#endif  /* #ifndef TIPPSE_FILECACHE_H */
