#ifndef TIPPSE_FILETYPE_XML_H
#define TIPPSE_FILETYPE_XML_H

#include <stdlib.h>

struct file_type_xml;

#include "../filetype.h"

struct file_type_xml {
  struct file_type vtbl;
};

#include "../trie.h"

struct file_type* file_type_xml_create(struct config* config);
void file_type_xml_destroy(struct file_type* base);
void file_type_xml_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags);
const char* file_type_xml_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_XML_H */
