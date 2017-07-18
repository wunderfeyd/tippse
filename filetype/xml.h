#ifndef __TIPPSE_FILETYPE_XML__
#define __TIPPSE_FILETYPE_XML__

#include <stdlib.h>

struct file_type_xml;

#include "../trie.h"
#include "../filetype.h"
#include "../rangetree.h"

struct file_type_xml {
  struct file_type vtbl;

  struct trie* keywords;
};

struct file_type* file_type_xml_create();
void file_type_xml_destroy(struct file_type* base);
void file_type_xml_mark(struct file_type* base, int* visual_detail, struct encoding* encoding, struct encoding_stream stream, int same_line, int* length, int* flags);
const char* file_type_xml_name();

#endif  /* #ifndef __TIPPSE_FILETYPE_C__ */
