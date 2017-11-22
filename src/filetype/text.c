// Tippse - File type - Plain text

#include "text.h"

struct file_type* file_type_text_create(struct config* config) {
  struct file_type_text* this = malloc(sizeof(struct file_type_text));
  this->vtbl.create = file_type_text_create;
  this->vtbl.destroy = file_type_text_destroy;
  this->vtbl.name = file_type_text_name;
  this->vtbl.mark = file_type_text_mark;
  this->vtbl.bracket_match = file_type_bracket_match;

  return (struct file_type*)this;
}

void file_type_text_destroy(struct file_type* base) {
  struct file_type_text* this = (struct file_type_text*)base;
  free(this);
}

const char* file_type_text_name(void) {
  return "Text";
}

void file_type_text_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  codepoint_t cp1 = encoding_cache_find_codepoint(cache, 0);

  int before = *visual_detail;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_DETAIL_WORD;
  }

  *visual_detail = after;
}
