// Tippse - File cache - Keep last accessed file pages in memory

#include "filecache.h"

// Create file cache
struct file_cache* file_cache_create(const char* filename) {
  struct file_cache* base = malloc(sizeof(struct file_cache));
  base->filename = strdup(filename);
  base->fd = open(base->filename, O_RDONLY);
  base->count = 1;
  base->left = FILE_CACHE_NODES;
  base->allocated = FILE_CACHE_NODES;
  base->first = NULL;
  base->last = NULL;
  for (size_t n = 0; n<FILE_CACHE_NODES; n++) {
    base->ranged[n] = NULL;
  }

  struct stat info;
  fstat(base->fd, &info);
  base->modification_time = info.st_mtime;
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

    close(base->fd);
    free(base->filename);
    free(base);
  }
}

// Add used node and return it
struct file_cache_node* file_cache_acquire_node(struct file_cache* base) {
  if (base->left==0) {
    file_cache_release_node(base, base->last);
  }

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
    node = file_cache_acquire_node(base);
    *reference = node;
    node->offset = FILE_OFFSET_T_MAX;
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

    if (base->fd!=-1) {
      lseek(base->fd, (off_t)node->offset, SEEK_SET);
      ssize_t in = read(base->fd, &node->buffer[0], node->length);
      if (in>0) {
        node->length = (size_t)in;
      } else {
        node->length = 0;
      }
    } else {
      node->length = 0;
    }
  }

  return &node->buffer[0];
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
  struct stat info;
  stat(base->filename, &info);
  return (base->modification_time!=info.st_mtime)?1:0;
}
