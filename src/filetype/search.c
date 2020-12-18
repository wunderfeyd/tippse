// Tippse - File type - Search output

#include "search.h"

#include "../document_text.h"
#include "../library/trie.h"
#include "../visualinfo.h"

struct file_type* file_type_search_create(struct config* config, const char* file_type) {
  struct file_type_search* self = (struct file_type_search*)malloc(sizeof(struct file_type_search));
  self->vtbl.file_type = strdup(file_type);
  self->vtbl.create = file_type_search_create;
  self->vtbl.destroy = file_type_search_destroy;
  self->vtbl.name = file_type_search_name;
  self->vtbl.mark = file_type_search_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;

  return (struct file_type*)self;
}

void file_type_search_destroy(struct file_type* base) {
  struct file_type_search* self = (struct file_type_search*)base;
  free(base->file_type);
  free(self);
}

const char* file_type_search_name(void) {
  return "Search";
}

void file_type_search_mark(struct document_text_render_info* render_info) {
  codepoint_t cp1 = render_info->sequence->cp[0];

  int before = render_info->visual_detail;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  } else if (unicode_word(cp1)) {
    after |= VISUAL_DETAIL_WORD;
  } else if (cp1=='\b') {
    after ^= VISUAL_DETAIL_STRING0;
  }

  if (before&VISUAL_DETAIL_NEWLINE) {
    after &= ~VISUAL_DETAIL_STRING0;
  }

  if ((after|before)&VISUAL_DETAIL_STRING0) {
    render_info->keyword_color = VISUAL_FLAG_COLOR_STRING;
    render_info->keyword_length = 1;
  } else {
    render_info->keyword_color = 0;
    render_info->keyword_length = 0;
  }

  render_info->visual_detail = after;
}
