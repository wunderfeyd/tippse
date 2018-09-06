#ifndef TIPPSE_STREAM_H
#define TIPPSE_STREAM_H

#include <stdlib.h>
#include "types.h"

#include "filecache.h"
#include "rangetree.h"

#define STREAM_TYPE_PLAIN 0
#define STREAM_TYPE_PAGED 1
#define STREAM_TYPE_FILE 2

struct stream {
  int type;                             // type of stream

  const uint8_t* plain;                 // address of buffer
  size_t displacement;                  // offset in buffer
  size_t cache_length;                  // length of buffer
  file_offset_t page_offset;            // offset in page
  struct file_cache_node* cache_node;   // file cache node

  union {
    struct range_tree_node* buffer;     // page in tree, if paged stream
    struct {
      struct file_cache* cache;         // file cache itself
      file_offset_t offset;             // current file position
    } file;
  };
};

void stream_from_plain(struct stream* base, const uint8_t* plain, size_t size);
void stream_from_page(struct stream* base, struct range_tree_node* buffer, file_offset_t displacement);
void stream_from_file(struct stream* base, struct file_cache* cache, file_offset_t offset);
void stream_destroy(struct stream* base);
void stream_clone(struct stream* base, struct stream* src);

int stream_rereference_page(struct stream* base);
void stream_reference_page(struct stream* base);
void stream_unreference_page(struct stream* base);

size_t stream_offset_plain(struct stream* base);
file_offset_t stream_offset_page(struct stream* base);
file_offset_t stream_offset_file(struct stream* base);
file_offset_t stream_offset(struct stream* base);

TIPPSE_INLINE size_t stream_cache_length(struct stream* base) {return base->cache_length;}
TIPPSE_INLINE size_t stream_displacement(struct stream* base) {return base->displacement;}
TIPPSE_INLINE const uint8_t* stream_buffer(struct stream* base) {return base->plain+base->displacement;}

uint8_t stream_read_forward_oob(struct stream* base);
TIPPSE_INLINE uint8_t stream_read_forward(struct stream* base) {
  if (base->displacement<base->cache_length) {
    return *(base->plain+base->displacement++);
  } else {
    return stream_read_forward_oob(base);
  }
}

uint8_t stream_read_reverse_oob(struct stream* base);
TIPPSE_INLINE uint8_t stream_read_reverse(struct stream* base) {
  base->displacement--;
  if (base->displacement<base->cache_length) {
    return *(base->plain+base->displacement);
  } else {
    return stream_read_reverse_oob(base);
  }
}

void stream_forward_oob(struct stream* base, size_t length);
TIPPSE_INLINE void stream_forward(struct stream* base, size_t length) {
  if (base->displacement+length<=base->cache_length) {
    base->displacement += length;
  } else {
    stream_forward_oob(base, length);
  }
}

void stream_reverse_oob(struct stream* base, size_t length);
TIPPSE_INLINE void stream_reverse(struct stream* base, size_t length) {
  if (base->displacement>=length) {
    base->displacement -= length;
  } else {
    stream_reverse_oob(base, length);
  }
}


// Helper for range tree end check
TIPPSE_INLINE int stream_end_oob(struct stream* base) {
  return ((base->type==STREAM_TYPE_PAGED && ((!base->buffer || !range_tree_next(base->buffer)) && base->page_offset+base->displacement>=range_tree_length(base->buffer))) || (base->type==STREAM_TYPE_FILE && base->cache_length<FILE_CACHE_PAGE_SIZE))?1:0;
}

// Helper for range tree start check
TIPPSE_INLINE int stream_start_oob(struct stream* base) {
  return ((base->type==STREAM_TYPE_PAGED && ((!base->buffer || !range_tree_prev(base->buffer)) && base->page_offset+base->displacement<=0)) || (base->type==STREAM_TYPE_FILE && base->file.offset==0))?1:0;
}

TIPPSE_INLINE int stream_end(struct stream* base) {
  return (base->displacement>=base->cache_length && (base->type==STREAM_TYPE_PLAIN || stream_end_oob(base)))?1:0;
}

TIPPSE_INLINE int stream_start(struct stream* base) {
  return ((ssize_t)base->displacement<=0 && (base->type==STREAM_TYPE_PLAIN || stream_start_oob(base)))?1:0;
}

TIPPSE_INLINE void stream_next(struct stream* base) {
  stream_forward_oob(base, base->cache_length-base->displacement);
}

TIPPSE_INLINE void stream_previous(struct stream* base) {
  stream_reverse_oob(base, base->displacement);
}

TIPPSE_INLINE file_offset_t stream_combined_offset(file_offset_t offset, size_t displacement) {
  return ((ssize_t)displacement>=0)?offset+displacement:offset-(file_offset_t)((ssize_t)0-(ssize_t)displacement);
}
#endif  /* #ifndef TIPPSE_STREAM_H */
