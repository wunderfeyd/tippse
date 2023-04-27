// Tippse - File type - PHP language

#include "php.h"

#include "../document_text.h"
#include "../library/trie.h"
#include "../visualinfo.h"

struct file_type* file_type_php_create(struct config* config, const char* type_name) {
  struct file_type_php* self = (struct file_type_php*)malloc(sizeof(struct file_type_php));
  self->vtbl.config = config;
  self->vtbl.type_name = strdup(type_name);
  self->vtbl.create = file_type_php_create;
  self->vtbl.destroy = file_type_php_destroy;
  self->vtbl.name = file_type_php_name;
  self->vtbl.mark = file_type_php_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;

  self->keywords = file_type_config_base((struct file_type*)self, "colors/keywords");

  return (struct file_type*)self;
}

void file_type_php_destroy(struct file_type* base) {
  struct file_type_php* self = (struct file_type_php*)base;
  free(base->type_name);
  free(self);
}

const char* file_type_php_name(void) {
  return "PHP";
}

void file_type_php_mark(struct document_text_render_info* render_info, int bracket_match) {
  int flags = 0;
  struct file_type_php* self = (struct file_type_php*)render_info->file_type;

  codepoint_t cp1 = render_info->sequence->cp[0];
  codepoint_t cp2 = unicode_sequencer_find(&render_info->sequencer, 1)->cp[0];

  render_info->keyword_length = 1;
  int before = render_info->visual_detail;
  if (before&VISUAL_DETAIL_NEWLINE) {
    before &= ~(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1|VISUAL_DETAIL_COMMENT1);
  }

  int before_masked = before&VISUAL_DETAIL_STATEMASK;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (before_masked&VISUAL_DETAIL_STRINGESCAPE) {
    after &= ~VISUAL_DETAIL_STRINGESCAPE;
  } else {
    if (cp1=='/') {
      if (before_masked==0) {
        if (cp2=='*') {
          render_info->keyword_length = 2;
          after |= VISUAL_DETAIL_COMMENT0;
        } else if (cp2=='/') {
          render_info->keyword_length = 2;
          after |= VISUAL_DETAIL_COMMENT1;
        }
      }
    } else if (cp1=='*') {
      if (cp2=='/' && before_masked==VISUAL_DETAIL_COMMENT0) {
        render_info->keyword_length = 2;
        after &= ~VISUAL_DETAIL_COMMENT0;
      }
    } else if (cp1=='\\') {
      if (before_masked==VISUAL_DETAIL_STRING0 || before_masked==VISUAL_DETAIL_STRING1) {
        after |= VISUAL_DETAIL_STRINGESCAPE;
      }
    } else if (cp1=='"') {
      if (before_masked==0) {
        after |= VISUAL_DETAIL_STRING0;
      } else if (before_masked==VISUAL_DETAIL_STRING0) {
        after &= ~VISUAL_DETAIL_STRING0;
      }
    } else if (cp1=='\'') {
      if (before_masked==0) {
        after |= VISUAL_DETAIL_STRING1;
      } else if (before_masked==VISUAL_DETAIL_STRING1) {
        after &= ~VISUAL_DETAIL_STRING1;
      }
    }
  }

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  }

  if (unicode_word(cp1) || cp1=='$') {
    after |= VISUAL_DETAIL_WORD;
  }

  if ((before|after)&(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1)) {
    flags = VISUAL_FLAG_COLOR_STRING;
  } else if ((before|after)&(VISUAL_DETAIL_COMMENT0)) {
    flags = VISUAL_FLAG_COLOR_BLOCKCOMMENT;
  } else if ((before|after)&(VISUAL_DETAIL_COMMENT1)) {
    flags = VISUAL_FLAG_COLOR_LINECOMMENT;
  } else if ((after)&(VISUAL_DETAIL_INDENTATION)) {
    render_info->keyword_length = 0;
    flags = 0;
  } else {
    if (!(before&VISUAL_DETAIL_WORD) && (after&VISUAL_DETAIL_WORD)) {
      render_info->keyword_length = 0;
      flags = file_type_keyword_config(render_info->file_type, &render_info->sequencer, self->keywords, &render_info->keyword_length, 0);
    }

    if (flags==0) {
      render_info->keyword_length = 0;
    }
  }

  render_info->visual_detail = after;
  render_info->keyword_color = flags;
}
