// Tippse - File type - Patch files

#include "patch.h"

struct file_type* file_type_patch_create(struct config* config, const char* file_type) {
  struct file_type_patch* self = malloc(sizeof(struct file_type_patch));
  self->vtbl.file_type = strdup(file_type);
  self->vtbl.create = file_type_patch_create;
  self->vtbl.destroy = file_type_patch_destroy;
  self->vtbl.name = file_type_patch_name;
  self->vtbl.mark = file_type_patch_mark;
  self->vtbl.bracket_match = file_type_bracket_match;
  self->vtbl.type = file_type_file_type;
  return (struct file_type*)self;
}

void file_type_patch_destroy(struct file_type* base) {
  struct file_type_patch* self = (struct file_type_patch*)base;
  free(base->file_type);
  free(self);
}

const char* file_type_patch_name(void) {
  return "Patch";
}

void file_type_patch_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  codepoint_t cp1 = encoding_cache_find_codepoint(cache, 0);

  *length = 1;
  int before = *visual_detail;
  int after = before&~(VISUAL_DETAIL_INDENTATION|VISUAL_DETAIL_WORD);

  if (before&VISUAL_DETAIL_NEWLINE) {
    after &= ~(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1|VISUAL_DETAIL_STRING2|VISUAL_DETAIL_COMMENT0);
    if (cp1=='@') {
      after |= VISUAL_DETAIL_COMMENT0;
    } else if (cp1==' ') {
      after |= VISUAL_DETAIL_STRING0;
    } else if (cp1=='+') {
      after |= VISUAL_DETAIL_STRING1;
    } else if (cp1=='-') {
      after |= VISUAL_DETAIL_STRING2;
    }
  }

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_DETAIL_INDENTATION;
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_DETAIL_WORD;
  }

  if ((before|after)&(VISUAL_DETAIL_COMMENT0)) {
    *flags = VISUAL_FLAG_COLOR_STRING;
  } else if ((before|after)&(VISUAL_DETAIL_STRING0)) {
    *length = 0;
    *flags = 0;
  } else if ((before|after)&(VISUAL_DETAIL_STRING1)) {
    *flags = VISUAL_FLAG_COLOR_PLUS;
  } else if ((before|after)&(VISUAL_DETAIL_STRING2)) {
    *flags = VISUAL_FLAG_COLOR_MINUS;
  } else {
    *flags = VISUAL_FLAG_COLOR_LINECOMMENT;
  }

  *visual_detail = after;
}
