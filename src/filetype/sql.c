// Tippse - File type - SQL

#include "sql.h"

#include "../document_text.h"
#include "../library/trie.h"
#include "../visualinfo.h"

struct file_type* file_type_sql_create(struct config* config, const char* type_name) {
  struct file_type_sql* self = (struct file_type_sql*)malloc(sizeof(struct file_type_sql));
  self->vtbl.config = config;
  self->vtbl.type_name = strdup(type_name);
  self->vtbl.create = file_type_sql_create;
  self->vtbl.destroy = file_type_sql_destroy;
  self->vtbl.name = file_type_sql_name;
  self->vtbl.mark = file_type_sql_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;

  self->keywords = file_type_config_base((struct file_type*)self, "colors/keywords");

  return (struct file_type*)self;
}

void file_type_sql_destroy(struct file_type* base) {
  struct file_type_sql* self = (struct file_type_sql*)base;
  free(base->type_name);
  free(self);
}

const char* file_type_sql_name(void) {
  return "SQL";
}

void file_type_sql_mark(struct document_text_render_info* render_info, struct unicode_sequencer* sequencer, struct unicode_sequence* sequence) {
  int flags = 0;
  struct file_type_sql* self = (struct file_type_sql*)render_info->file_type;

  codepoint_t cp1 = sequence->cp[0];
  codepoint_t cp2 = unicode_sequencer_find(sequencer, 1)->cp[0];

  render_info->keyword_length = 1;
  int before = render_info->visual_detail;
  if (before&VISUAL_DETAIL_NEWLINE) {
    before &= ~(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1|VISUAL_DETAIL_STRING2|VISUAL_DETAIL_COMMENT1);
  }

  int before_masked = before&VISUAL_DETAIL_STATEMASK;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (before_masked&VISUAL_DETAIL_STRINGESCAPE) {
    after &= ~VISUAL_DETAIL_STRINGESCAPE;
  } else {
    if (cp1=='/') {
      if (cp2=='*' && before_masked==0) {
        render_info->keyword_length = 2;
        after |= VISUAL_DETAIL_COMMENT0;
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
    } else if (cp1=='-') {
      if (cp2=='-') {
        render_info->keyword_length = 2;
        after |= VISUAL_DETAIL_COMMENT1;
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
    } else if (cp1=='`') {
      if (before_masked==0) {
        after |= VISUAL_DETAIL_STRING2;
      } else if (before_masked==VISUAL_DETAIL_STRING2) {
        after &= ~VISUAL_DETAIL_STRING2;
      }
    }
  }

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  }

  if (unicode_word(cp1)) {
    after |= VISUAL_DETAIL_WORD;
  }

  if ((before|after)&(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1|VISUAL_DETAIL_STRING2)) {
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
      flags = file_type_keyword_config(render_info->file_type, sequencer, self->keywords, &render_info->keyword_length, 1);
    }

    if (flags==0) {
      render_info->keyword_length = 0;
    }
  }

  render_info->visual_detail = after;
  render_info->keyword_color = flags;
}
