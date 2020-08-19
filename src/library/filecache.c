// Tippse - File cache - Keep last accessed file pages in memory

#include "filecache.h"

#include "file.h"
#include "rangetree.h"

#define FILE_CACHE_DEBUG 0

// Create file cache
struct file_cache* file_cache_create(const char* filename) {
  struct file_cache* base = (struct file_cache*)malloc(sizeof(struct file_cache));
  base->filename = strdup(filename);
  base->fd = file_create(base->filename, TIPPSE_FILE_READ);
  base->count = 1;
  range_tree_create_inplace(&base->index, NULL, 0);
  list_create_inplace(&base->active, sizeof(struct file_cache_node));
  list_create_inplace(&base->inactive, sizeof(struct file_cache_node));
  base->size = 0;
  base->max = FILE_CACHE_SIZE;

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
    range_tree_destroy_inplace(&base->index);

    file_cache_empty(base, &base->active);
    file_cache_empty(base, &base->inactive);

    if (base->fd) {
      file_destroy(base->fd);
    }

    free(base->filename);
    free(base);
  }
}

// Decrease reference counter of file cache
void file_cache_empty(struct file_cache* base, struct list* nodes) {
  while (nodes->first) {
    struct file_cache_node* node = (struct file_cache_node*)list_object(nodes->first);
    free(node->buffer);
    if (node->count!=0) {
      fprintf(stderr, "Hmm, a cached node is still in use! (%d-%llu) \r\n", node->count, (long long unsigned int)node->offset);
    }
    list_remove(nodes, nodes->first);
  }
}

// Check if cache size is exhausted
void file_cache_cleanup(struct file_cache* base) {
  while (base->size>base->max) {
    struct file_cache_node* node = (struct file_cache_node*)list_object(base->inactive.last);
    if (node->count!=0) {
      fprintf(stderr, "Hmm, a cached node is still in use in inactive list (%d-%llu)!\r\n", node->count, (long long unsigned int)node->offset);
      break;
    }

    if (FILE_CACHE_DEBUG) {
      fprintf(stderr, "Remove %p %llx\r\n", node, (long long unsigned int)node->offset);
    }

    range_tree_mark(&base->index, node->offset, FILE_CACHE_PAGE_SIZE, 0);
    free(node->buffer);
    list_remove(&base->inactive, base->inactive.last);
    base->size -= FILE_CACHE_PAGE_SIZE;
  }
}

// Create a reference (or valid buffer) to a cache
struct file_cache_node* file_cache_invoke(struct file_cache* base, file_offset_t offset, size_t length, const uint8_t** buffer, size_t* buffer_length) {
  file_offset_t index = offset/FILE_CACHE_PAGE_SIZE;
  file_offset_t low = index*FILE_CACHE_PAGE_SIZE;
  file_offset_t high = low+FILE_CACHE_PAGE_SIZE;
  if (range_tree_length(&base->index)<high) {
    range_tree_resize(&base->index, high, 0);
  }

  file_offset_t diff;
  struct file_cache_node* node;
  // TODO: optimize ... some parts are doing some work twice
  if (range_tree_node_marked(base->index.root, low, high-low, TIPPSE_INSERTER_MARK)) {
    struct range_tree_node* tree = range_tree_node_find_offset(base->index.root, low, &diff);
    struct list_node* it = (struct list_node*)tree->user_data;
    node = (struct file_cache_node*)list_object(it);
    if (it!=base->active.first) {
      if (node->count==0) {
        list_remove_node(&base->inactive, it);
        list_insert_node(&base->active, it, NULL);
      } else {
        list_move(&base->active, it, NULL);
      }
    }
    node->count++;
  } else {
    range_tree_mark(&base->index, low, high-low, TIPPSE_INSERTER_MARK|TIPPSE_INSERTER_NOFUSE);
    struct range_tree_node* tree = range_tree_node_find_offset(base->index.root, low, &diff);
    tree->user_data = list_insert_empty(&base->active, NULL);
    node = (struct file_cache_node*)list_object((struct list_node*)tree->user_data);
    node->list_node = (struct list_node*)tree->user_data;
    node->count = 1;
    node->buffer = (uint8_t*)malloc(FILE_CACHE_PAGE_SIZE);
    node->offset = low;
    node->length = FILE_CACHE_PAGE_SIZE;
    base->size += FILE_CACHE_PAGE_SIZE;

    if (base->fd) {
      file_seek(base->fd, low, TIPPSE_SEEK_START);
      node->length = file_read(base->fd, node->buffer, FILE_CACHE_PAGE_SIZE);
      if (FILE_CACHE_DEBUG) {
        fprintf(stderr, "Read %p %llux %x\r\n", node, (long long unsigned int)low, (unsigned int)node->length);
      }
    }
  }

  file_cache_cleanup(base);
  file_cache_debug(base);

  size_t displacement = offset%FILE_CACHE_PAGE_SIZE;
  *buffer = node->buffer+displacement;
  *buffer_length = (displacement<=node->length)?node->length-displacement:0;
  return node;
}

// Clone reference
void file_cache_clone(struct file_cache* base, struct file_cache_node* node) {
  if (node->count==0) {
    fprintf(stderr, "Node to clone was not in use...");
    abort();
  }
  node->count++;

  file_cache_debug(base);
}

// Release reference
void file_cache_revoke(struct file_cache* base, struct file_cache_node* node) {
  node->count--;

  if (node->count==0) {
    list_remove_node(&base->active, node->list_node);
    list_insert_node(&base->inactive, node->list_node, NULL);
  } else if (node->count<0) {
    fprintf(stderr, "Node count underflow!!! There's a serious problem.");
    abort();
  }

  file_cache_debug(base);
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

// Print cache debug information
void file_cache_debug(struct file_cache* base) {
  if (FILE_CACHE_DEBUG) {
    int count = 0;

    struct list_node* it = base->active.first;
    while (it) {
      struct file_cache_node* node = (struct file_cache_node*)list_object(it);
      count += node->count;

      it = it->next;
    }

    fprintf(stderr, "FC '%s' %d+%d/%d & %d refs (%d/%d)\r\n", base->filename, (int)base->active.count, (int)base->inactive.count, (int)(base->active.count+base->inactive.count), count, (int)base->size, (int)base->max);
  }
}
