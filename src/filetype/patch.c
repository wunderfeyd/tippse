// Tippse - File type - Patch files

#include "patch.h"

struct file_type* file_type_patch_create(void) {
  struct file_type_patch* this = malloc(sizeof(struct file_type_patch));
  this->vtbl.create = file_type_patch_create;
  this->vtbl.destroy = file_type_patch_destroy;
  this->vtbl.name = file_type_patch_name;
  this->vtbl.mark = file_type_patch_mark;
  this->vtbl.bracket_match = file_type_bracket_match;
  return (struct file_type*)this;
}

void file_type_patch_destroy(struct file_type* base) {
}

const char* file_type_patch_name(void) {
  return "Patch";
}

void file_type_patch_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  int cp1 = encoding_cache_find_codepoint(cache, 0);

  *length = 1;
  int before = *visual_detail;
  int after = before&~(VISUAL_INFO_INDENTATION|VISUAL_INFO_WORD);

  if (before&VISUAL_INFO_NEWLINE) {
    after &= ~(VISUAL_INFO_STRING0|VISUAL_INFO_STRING1|VISUAL_INFO_STRING2|VISUAL_INFO_COMMENT0);
    if (cp1=='@') {
      after |= VISUAL_INFO_COMMENT0;
    } else if (cp1==' ') {
      after |= VISUAL_INFO_STRING0;
    } else if (cp1=='+') {
      after |= VISUAL_INFO_STRING1;
    } else if (cp1=='-') {
      after |= VISUAL_INFO_STRING2;
    }
  }

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_INFO_INDENTATION;
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_INFO_WORD;
  }

  if ((before|after)&(VISUAL_INFO_COMMENT0)) {
    *flags = VISUAL_FLAG_COLOR_STRING;
  } else if ((before|after)&(VISUAL_INFO_STRING0)) {
    *length = 0;
    *flags = 0;
  } else if ((before|after)&(VISUAL_INFO_STRING1)) {
    *flags = VISUAL_FLAG_COLOR_PLUS;
  } else if ((before|after)&(VISUAL_INFO_STRING2)) {
    *flags = VISUAL_FLAG_COLOR_MINUS;
  } else {
    *flags = VISUAL_FLAG_COLOR_LINECOMMENT;
  }

  *visual_detail = after;
}
