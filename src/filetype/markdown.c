// Tippse - File type - Plain markdown

#include "markdown.h"

#include "../document_text.h"
#include "../trie.h"
#include "../visualinfo.h"

struct file_type* file_type_markdown_create(struct config* config, const char* file_type) {
  struct file_type_markdown* self = (struct file_type_markdown*)malloc(sizeof(struct file_type_markdown));
  self->vtbl.file_type = strdup(file_type);
  self->vtbl.create = file_type_markdown_create;
  self->vtbl.destroy = file_type_markdown_destroy;
  self->vtbl.name = file_type_markdown_name;
  self->vtbl.mark = file_type_markdown_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;

  return (struct file_type*)self;
}

void file_type_markdown_destroy(struct file_type* base) {
  struct file_type_markdown* self = (struct file_type_markdown*)base;
  free(base->file_type);
  free(self);
}

const char* file_type_markdown_name(void) {
  return "markdown";
}

void file_type_markdown_mark(struct document_text_render_info* render_info) {
  codepoint_t cp1 = render_info->sequence->cp[0];
  codepoint_t cp2 = unicode_sequencer_find(&render_info->sequencer, 1)->cp[0];
  codepoint_t cp3 = unicode_sequencer_find(&render_info->sequencer, 2)->cp[0];

  int before = render_info->visual_detail;
  if (before&VISUAL_DETAIL_NEWLINE) {
    before &= ~(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1|VISUAL_DETAIL_STRING2|VISUAL_DETAIL_COMMENT0);
  }

  int before_masked = before&(VISUAL_DETAIL_STATEMASK&~VISUAL_DETAIL_STRING0);
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);
  int flags = 0;

  if (cp1=='\t' || cp1==' ') {
    after &= ~VISUAL_DETAIL_STRING0;
    after |= VISUAL_DETAIL_INDENTATION;
  } else if ((cp1>='0' && cp1<='9') && !(before&VISUAL_DETAIL_STOPPED_INDENTATION)) {
    for (size_t n = 0; n<10; n++) {
      codepoint_t cp = unicode_sequencer_find(&render_info->sequencer, n+1)->cp[0];
      if (cp=='.') {
        flags = VISUAL_FLAG_COLOR_STRING;
        render_info->keyword_length = (int)n+2;
        break;
      } else if (cp<'0' || cp>'9') {
        after |= VISUAL_DETAIL_WORD;
        break;
      }
    }
  } else if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_DETAIL_WORD;
  } else if ((cp1=='*' || cp1=='-' || cp1=='+') && (cp2==' ' || cp2=='\t') && !(before&VISUAL_DETAIL_STOPPED_INDENTATION)) {
    flags = VISUAL_FLAG_COLOR_STRING;
    render_info->keyword_length = 1;
  } else if ((cp1=='=' || cp1=='-' || cp1=='#') && !(before&VISUAL_DETAIL_STOPPED_INDENTATION)) {
    after |= VISUAL_DETAIL_STRING0;
  } else if ((cp1=='>') && !(before&VISUAL_DETAIL_STOPPED_INDENTATION)) {
    after |= VISUAL_DETAIL_COMMENT0;
  } else if (cp1=='[') {
    if (before_masked==0) {
      after |= VISUAL_DETAIL_STRING2;
    }
  } else if (cp1==']') {
    if (before_masked==VISUAL_DETAIL_STRING2) {
      after &= ~VISUAL_DETAIL_STRING2;
    }
  } else if (cp1=='`' && cp2=='`' && cp3=='`' && !(before&VISUAL_DETAIL_STOPPED_INDENTATION)) {
    after ^= VISUAL_DETAIL_COMMENT1;
  } else if (cp1=='`') {
    if (before_masked==0) {
      after |= VISUAL_DETAIL_STRING1;
    } else {
      after &= ~VISUAL_DETAIL_STRING1;
    }
  }

  if ((before&after)&VISUAL_DETAIL_STRING2) {
    flags = VISUAL_FLAG_COLOR_PREPROCESSOR;
    render_info->keyword_length = 1;
  } else if ((before|after)&VISUAL_DETAIL_STRING0) {
    flags = VISUAL_FLAG_COLOR_TYPE;
    render_info->keyword_length = 1;
  } else if ((before|after)&VISUAL_DETAIL_COMMENT1) {
    flags = VISUAL_FLAG_COLOR_BLOCKCOMMENT;
    render_info->keyword_length = (after&VISUAL_DETAIL_COMMENT1)?1:3;
  } else if ((before|after)&VISUAL_DETAIL_COMMENT0) {
    flags = VISUAL_FLAG_COLOR_LINECOMMENT;
    render_info->keyword_length = 1;
  }

  render_info->visual_detail = after;
  render_info->keyword_color = flags;
}
