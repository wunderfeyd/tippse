// Tippse - File type - XML

#include "xml.h"

struct file_type* file_type_xml_create(struct config* config, const char* file_type) {
  struct file_type_xml* self = malloc(sizeof(struct file_type_xml));
  self->vtbl.file_type = strdup(file_type);
  self->vtbl.create = file_type_xml_create;
  self->vtbl.destroy = file_type_xml_destroy;
  self->vtbl.name = file_type_xml_name;
  self->vtbl.mark = file_type_xml_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;
  return (struct file_type*)self;
}

void file_type_xml_destroy(struct file_type* base) {
  struct file_type_xml* self = (struct file_type_xml*)base;
  free(base->file_type);
  free(self);
}

const char* file_type_xml_name(void) {
  return "XML";
}

void file_type_xml_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int* length, int* flags) {
  codepoint_t cp1 = encoding_cache_find_codepoint(cache, 0);
  codepoint_t cp2 = encoding_cache_find_codepoint(cache, 1);

  *length = 1;
  int before = *visual_detail;
  if (before&VISUAL_DETAIL_NEWLINE) {
    before &= ~(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1);
  }

  int before_masked = before&VISUAL_DETAIL_STATEMASK;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (before_masked&VISUAL_DETAIL_STRINGESCAPE) {
    after &= ~VISUAL_DETAIL_STRINGESCAPE;
  } else {
    if (cp1=='<') {
      if (before_masked==0) {
        after |= VISUAL_DETAIL_COMMENT2;
      }
    } else if (cp1=='>') {
      if (before_masked==VISUAL_DETAIL_COMMENT2) {
        after &= ~VISUAL_DETAIL_COMMENT2;
      }
    } else if (cp1=='-') {
      if (cp2=='-') {
        if (before_masked==VISUAL_DETAIL_COMMENT2) {
          *length = 2;
          after |= VISUAL_DETAIL_COMMENT0;
        } else if (before_masked==(VISUAL_DETAIL_COMMENT2|VISUAL_DETAIL_COMMENT0)) {
          *length = 2;
          after &= ~VISUAL_DETAIL_COMMENT0;
        }
      }
    } else if (cp1=='"') {
      if (before_masked&VISUAL_DETAIL_STRING0) {
        after &= ~VISUAL_DETAIL_STRING0;
      } else if (before_masked==VISUAL_DETAIL_COMMENT2) {
        after |= VISUAL_DETAIL_STRING0;
      }
    } else if (cp1=='\'') {
      if (before_masked&VISUAL_DETAIL_STRING1) {
        after &= ~VISUAL_DETAIL_STRING1;
      } else if (before_masked==VISUAL_DETAIL_COMMENT2) {
        after |= VISUAL_DETAIL_STRING1;
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
    *flags = VISUAL_FLAG_COLOR_STRING;
  } else if ((before|after)&(VISUAL_DETAIL_COMMENT0)) {
    *flags = VISUAL_FLAG_COLOR_BLOCKCOMMENT;
  } else if ((before|after)&(VISUAL_DETAIL_COMMENT2)) {
    *flags = VISUAL_FLAG_COLOR_TYPE;
  } else if ((after)&(VISUAL_DETAIL_INDENTATION)) {
    *length = 0;
    *flags = 0;
  } else {
    if (!(before&VISUAL_DETAIL_WORD) && (after&VISUAL_DETAIL_WORD)) {
      *length = 0;
      *flags = 0;
    }

    if (*flags==0) {
      *length = 0;
    }
  }

  *visual_detail = after;
}
