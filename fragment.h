#ifndef __TIPPSE_SHAREDBUFFER__
#define __TIPPSE_SHAREDBUFFER__

#include <stdlib.h>

struct fragment {
  int count;
  char* buffer;
  size_t length;
};

struct fragment* fragment_create(char* buffer, size_t length);
void fragment_reference(struct fragment* node);
void fragment_dereference(struct fragment* node);

#endif /* #ifndef __TIPPSE_SHAREDBUFFER__ */
