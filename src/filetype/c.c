// Tippse - File type - C language

#include "c.h"

#include "../document_text.h"
#include "../trie.h"
#include "../visualinfo.h"

struct file_type* file_type_c_create(struct config* config, const char* file_type) {
  struct file_type_c* self = malloc(sizeof(struct file_type_c));
  self->vtbl.config = config;
  self->vtbl.file_type = strdup(file_type);
  self->vtbl.create = file_type_c_create;
  self->vtbl.destroy = file_type_c_destroy;
  self->vtbl.name = file_type_c_name;
  self->vtbl.mark = file_type_c_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;

  self->keywords = file_type_config_base((struct file_type*)self, "colors/keywords");
  self->keywords_preprocessor = file_type_config_base((struct file_type*)self, "colors/preprocessor");

  return (struct file_type*)self;
}

void file_type_c_destroy(struct file_type* base) {
  struct file_type_c* self = (struct file_type_c*)base;
  free(base->file_type);
  free(self);
}

const char* file_type_c_name(void) {
  return "C";
}

void file_type_c_mark(struct document_text_render_info* render_info ) {
  int flags = 0;
  struct file_type_c* self = (struct file_type_c*)render_info->file_type;

  codepoint_t cp1 = render_info->sequence->cp[0];
  codepoint_t cp2 = unicode_sequencer_find(&render_info->sequencer, 1)->cp[0];

  render_info->keyword_length = 1;
  int before = render_info->visual_detail;
  if (before&VISUAL_DETAIL_NEWLINE) {
    before &= ~(VISUAL_DETAIL_PREPROCESSOR|VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1|VISUAL_DETAIL_COMMENT1);
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
      if (before_masked&(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1)) {
        after |= VISUAL_DETAIL_STRINGESCAPE;
      }
    } else if (cp1=='"') {
      if (before_masked==0) {
        after |= VISUAL_DETAIL_STRING0;
      } else {
        after &= ~VISUAL_DETAIL_STRING0;
      }
    } else if (cp1=='\'') {
      if (before_masked==0) {
        after |= VISUAL_DETAIL_STRING1;
      } else {
        after &= ~VISUAL_DETAIL_STRING1;
      }
    } else if (cp1=='#') {
      if (before&(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_NEWLINE)) {
        after |= VISUAL_DETAIL_PREPROCESSOR;
      }
    }
  }

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
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
    if (after&VISUAL_DETAIL_PREPROCESSOR) {
      if (cp1>' ' && (before&VISUAL_DETAIL_PREPROCESSOR)) {
        after &= ~VISUAL_DETAIL_PREPROCESSOR;
        render_info->keyword_length = 0;
        flags = file_type_keyword_config(render_info->file_type, &render_info->sequencer, self->keywords_preprocessor, &render_info->keyword_length, 0);
      }

      if (flags==0) {
        if (after&VISUAL_DETAIL_PREPROCESSOR) {
          flags = VISUAL_FLAG_COLOR_PREPROCESSOR;
          render_info->keyword_length = 1;
        } else {
          render_info->keyword_length = 0;
        }
      }
    } else {
      if (!(before&VISUAL_DETAIL_WORD) && (after&VISUAL_DETAIL_WORD)) {
        render_info->keyword_length = 0;
        flags = file_type_keyword_config(render_info->file_type, &render_info->sequencer, self->keywords, &render_info->keyword_length, 0);
      }

      if (flags==0) {
        render_info->keyword_length = 0;
      }
    }
  }

  render_info->visual_detail = after;
  render_info->keyword_color = flags;
}
