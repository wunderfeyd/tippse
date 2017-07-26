#ifndef __TIPPSE_FRAGMENT__
#define __TIPPSE_FRAGMENT__

struct fragment;

#define FRAGMENT_MEMORY 0
#define FRAGMENT_FILE 1

#include <stdlib.h>
#include "types.h"
//#include "documentfile.h"

struct fragment {
  int count;
  int type;
  char* buffer;
  file_offset_t offset;
  struct document_file* file;

  size_t length;
};

struct fragment* fragment_create_memory(char* buffer, size_t length);
struct fragment* fragment_create_file(struct document_file* file, file_offset_t offset, size_t length);
void fragment_reference(struct fragment* node);
void fragment_dereference(struct fragment* node);

#endif /* #ifndef __TIPPSE_FRAGMENT__ */
