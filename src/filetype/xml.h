#ifndef TIPPSE_FILETYPE_XML_H
#define TIPPSE_FILETYPE_XML_H

#include <stdlib.h>
#include "../types.h"

#include "../filetype.h"

struct file_type_xml {
  struct file_type vtbl;
};

struct file_type* file_type_xml_create(struct config* config, const char* type_name);
void file_type_xml_destroy(struct file_type* base);
void file_type_xml_mark(struct document_text_render_info* render_info);
const char* file_type_xml_name(void);

#endif  /* #ifndef TIPPSE_FILETYPE_XML_H */
