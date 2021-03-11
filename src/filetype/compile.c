// Tippse - File type - Compile output

#include "compile.h"

#include "../document_text.h"
#include "../library/trie.h"
#include "../visualinfo.h"

struct file_type* file_type_compile_create(struct config* config, const char* type_name) {
  struct file_type_compile* self = (struct file_type_compile*)malloc(sizeof(struct file_type_compile));
  self->vtbl.config = config;
  self->vtbl.type_name = strdup(type_name);
  self->vtbl.create = file_type_compile_create;
  self->vtbl.destroy = file_type_compile_destroy;
  self->vtbl.name = file_type_compile_name;
  self->vtbl.mark = file_type_compile_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;

  self->keywords = file_type_config_base((struct file_type*)self, "colors/keywords");

  return (struct file_type*)self;
}

void file_type_compile_destroy(struct file_type* base) {
  struct file_type_compile* self = (struct file_type_compile*)base;
  free(base->type_name);
  free(self);
}

const char* file_type_compile_name(void) {
  return "Compile";
}

void file_type_compile_mark(struct document_text_render_info* render_info) {
  codepoint_t cp1 = render_info->sequence->cp[0];

  int before = render_info->visual_detail;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  } else if (unicode_word(cp1)) {
    after |= VISUAL_DETAIL_WORD;
  }

  if (before&VISUAL_DETAIL_NEWLINE) {
    if ((before&(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_COMMENT0)) && (after&VISUAL_DETAIL_INDENTATION)) {
      after |= VISUAL_DETAIL_COMMENT0;
    } else {
      after &= ~VISUAL_DETAIL_COMMENT0;
    }

    after &= ~VISUAL_DETAIL_STRING0;
  }

  if ((after|before)&VISUAL_DETAIL_STRING0) {
    render_info->keyword_color = VISUAL_FLAG_COLOR_STRING;
    render_info->keyword_length = 1;
  } else if (after&VISUAL_DETAIL_COMMENT0) {
    render_info->keyword_color = VISUAL_FLAG_COLOR_BLOCKCOMMENT;
    render_info->keyword_length = 1;
  } else {
    render_info->keyword_color = 0;
    if (!(before&VISUAL_DETAIL_WORD) && (after&VISUAL_DETAIL_WORD)) {
      struct file_type_compile* self = (struct file_type_compile*)render_info->file_type;
      render_info->keyword_length = 0;
      render_info->keyword_color = file_type_keyword_config(render_info->file_type, &render_info->sequencer, self->keywords, &render_info->keyword_length, 0);
      if (render_info->keyword_color==0) {
        render_info->keyword_length = 0;
      } else {
        after |= VISUAL_DETAIL_STRING0;
      }
    }
  }

  render_info->visual_detail = after;
}
