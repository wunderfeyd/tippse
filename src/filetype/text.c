// Tippse - File type - Plain text

#include "text.h"

#include "../document_text.h"
#include "../trie.h"
#include "../visualinfo.h"

struct file_type* file_type_text_create(struct config* config, const char* file_type) {
  struct file_type_text* self = (struct file_type_text*)malloc(sizeof(struct file_type_text));
  self->vtbl.file_type = strdup(file_type);
  self->vtbl.create = file_type_text_create;
  self->vtbl.destroy = file_type_text_destroy;
  self->vtbl.name = file_type_text_name;
  self->vtbl.mark = file_type_text_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;

  return (struct file_type*)self;
}

void file_type_text_destroy(struct file_type* base) {
  struct file_type_text* self = (struct file_type_text*)base;
  free(base->file_type);
  free(self);
}

const char* file_type_text_name(void) {
  return "Text";
}

void file_type_text_mark(struct document_text_render_info* render_info) {
  codepoint_t cp1 = render_info->sequence->cp[0];

  int before = render_info->visual_detail;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  } else if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_DETAIL_WORD;
  }

  render_info->visual_detail = after;
}
