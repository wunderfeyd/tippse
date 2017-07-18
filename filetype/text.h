#ifndef __TIPPSE_FILETYPE_TEXT__
#define __TIPPSE_FILETYPE_TEXT__

#include <stdlib.h>

struct file_type_text;

#include "../trie.h"
#include "../filetype.h"
#include "../rangetree.h"

struct file_type_text {
  struct file_type vtbl;
};

struct file_type* file_type_text_create();
void file_type_text_destroy(struct file_type* base);
void file_type_text_mark(struct file_type* base, int* visual_detail, struct encoding* encoding, struct encoding_stream stream, int same_line, int* length, int* flags);
const char* file_type_text_name();

#endif  /* #ifndef __TIPPSE_FILETYPE_TEXT__ */
