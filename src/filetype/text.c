// Tippse - File type - Plain text

#include "text.h"

#include "../document_text.h"
#include "../library/trie.h"
#include "../visualinfo.h"

struct file_type* file_type_text_create(struct config* config, const char* type_name) {
  struct file_type_text* self = (struct file_type_text*)malloc(sizeof(struct file_type_text));
  self->vtbl.type_name = strdup(type_name);
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
  free(base->type_name);
  free(self);
}

const char* file_type_text_name(void) {
  return "Text";
}

void file_type_text_mark(struct document_text_render_info* render_info, struct unicode_sequencer* sequencer, struct unicode_sequence* sequence) {
  codepoint_t cp1 = sequence->cp[0];

  int before = render_info->visual_detail;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  } else if (unicode_word(cp1)) {
    after |= VISUAL_DETAIL_WORD;
  }

  render_info->visual_detail = after;
}
