// Tippse - File cache - Keep last accessed file pages in memory

#include "filecache.h"

// Create file cache
struct file_cache* file_cache_create(const char* filename) {
  struct file_cache* base = malloc(sizeof(struct file_cache));
  base->filename = strdup(filename);
  base->fd = file_create(base->filename, TIPPSE_FILE_READ);
  base->count = 1;
  base->left = FILE_CACHE_NODES;
  base->allocated = FILE_CACHE_NODES;
  base->first = NULL;
  base->last = NULL;
  base->size = 0;
  for (size_t n = 0; n<FILE_CACHE_NODES; n++) {
    base->ranged[n] = NULL;
  }

  if (base->fd) {
#ifdef _WINDOWS
    GetFileTime(base->fd->fd, NULL, NULL, &base->modification_time);
#else
    struct stat info;
    fstat(base->fd->fd, &info);
    base->modification_time = info.st_mtime;
#endif
  }
  return base;
}

// Increase reference counter of file cache
void file_cache_reference(struct file_cache* base) {
  base->count++;
}

// Decrease reference counter of file cache
void file_cache_dereference(struct file_cache* base) {
  base->count--;

  if (base->count==0) {
    for (size_t count = base->allocated; count<FILE_CACHE_NODES; count++) {
      free(base->nodes[count]);
    }

    if (base->fd) {
      file_destroy(base->fd);
    }
    free(base->filename);
    free(base);
  }
}

// Remove nodes to reduce cache size
void file_cache_cleanup(struct file_cache* base, size_t length) {
  while (base->size>FILE_CACHE_SIZE-length && base->left!=FILE_CACHE_NODES) {
    file_cache_release_node(base, base->last);
  }
}

// Add used node and return it
struct file_cache_node* file_cache_acquire_node(struct file_cache* base) {
  base->left--;
  if (base->left<base->allocated) {
    base->allocated = base->left;
    base->open[base->left] = base->nodes[base->left] = malloc(sizeof(struct file_cache_node));
  }

  struct file_cache_node* node = base->open[base->left];
  file_cache_link_node(base, node);

  return node;
}

// Add unused node
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

// Remove used node
void file_cache_release_node(struct file_cache* base, struct file_cache_node* node) {
  if (node->reference) {
    *node->reference = NULL;
  }

  free(node->buffer);
  base->size -= node->length;
  file_cache_unlink_node(base, node);
  base->open[base->left++] = node;
}

// Remove unused node
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

// Mark node as used and transfer file content
uint8_t* file_cache_use_node(struct file_cache* base, struct file_cache_node** reference, file_offset_t offset, size_t length) {
  struct file_cache_node* node = *reference;
  if (node) {
    file_cache_unlink_node(base, node);
    file_cache_link_node(base, node);
    node->reference = reference;
  } else {
    file_cache_cleanup(base, length);
    node = file_cache_acquire_node(base);
    *reference = node;
    node->offset = FILE_OFFSET_T_MAX;
    node->length = 0;
    node->reference = reference;
    node->buffer = NULL;
  }

  if (node->offset!=offset || node->length!=length) {
    node->offset = offset;

    if (base->fd) {
      if (node->length!=length) {
        base->size -= node->length;
        node->length = length;
        free(node->buffer);
        node->buffer = malloc(sizeof(uint8_t)*node->length);
        base->size += node->length;
      }

      file_seek(base->fd, node->offset, TIPPSE_SEEK_START);
      node->length = file_read(base->fd, node->buffer, node->length);
    } else {
      base->size -= node->length;
      free(node->buffer);
      node->buffer = NULL;
      node->length = 0;
    }
  }

  return node->buffer;
}

// Mark node as used and transfer file content depending on the base offset while the file cache itself holds the node references
uint8_t* file_cache_use_node_ranged(struct file_cache* base, struct file_cache_node** reference, file_offset_t offset, size_t length) {
  file_offset_t index = (offset/TREE_BLOCK_LENGTH_MAX)%FILE_CACHE_NODES;
  uint8_t* buffer = file_cache_use_node(base, &base->ranged[index], offset, length);
  *reference = base->ranged[index];
  return buffer;
}

// The file was modified while it was open?
int file_cache_modified(struct file_cache* base) {
#ifdef _WINDOWS
  FILETIME info;
  GetFileTime(base->fd->fd, NULL, NULL, &info);
  return memcmp(&base->modification_time, &info, sizeof(FILETIME))?1:0;
#else
  struct stat info;
  stat(base->filename, &info);
  return (base->modification_time!=info.st_mtime)?1:0;
#endif
}
