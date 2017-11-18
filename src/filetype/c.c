// Tippse - File type - C language

#include "c.h"

struct file_type* file_type_c_create(struct config* config) {
  struct file_type_c* this = malloc(sizeof(struct file_type_c));
  this->vtbl.config = config;
  this->vtbl.create = file_type_c_create;
  this->vtbl.destroy = file_type_c_destroy;
  this->vtbl.name = file_type_c_name;
  this->vtbl.mark = file_type_c_mark;
  this->vtbl.bracket_match = file_type_bracket_match;

  this->keywords = file_type_config_base((struct file_type*)this, "colors/keywords");
  this->keywords_preprocessor = file_type_config_base((struct file_type*)this, "colors/preprocessor");

  return (struct file_type*)this;
}

void file_type_c_destroy(struct file_type* base) {
  struct file_type_c* this = (struct file_type_c*)base;
  free(this);
}

const char* file_type_c_name(void) {
  return "C";
}

void file_type_c_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  struct file_type_c* this = (struct file_type_c*)base;

  int cp1 = encoding_cache_find_codepoint(cache, 0);
  int cp2 = encoding_cache_find_codepoint(cache, 1);

  *length = 1;
  int before = *visual_detail;
  if (before&VISUAL_INFO_NEWLINE) {
    before &= ~(VISUAL_INFO_PREPROCESSOR|VISUAL_INFO_STRING0|VISUAL_INFO_STRING1|VISUAL_INFO_COMMENT1);
  }

  int before_masked = before&VISUAL_INFO_STATEMASK;
  int after = before&~(VISUAL_INFO_INDENTATION|VISUAL_INFO_WORD);

  if (before_masked&VISUAL_INFO_STRINGESCAPE) {
    after &= ~VISUAL_INFO_STRINGESCAPE;
  } else {
    if (cp1=='/') {
      if (before_masked==0) {
        if (cp2=='*') {
          *length = 2;
          after |= VISUAL_INFO_COMMENT0;
        } else if (cp2=='/') {
          *length = 2;
          after |= VISUAL_INFO_COMMENT1;
        }
      }
    } else if (cp1=='*') {
      if (cp2=='/' && before_masked==VISUAL_INFO_COMMENT0) {
        *length = 2;
        after &= ~VISUAL_INFO_COMMENT0;
      }
    } else if (cp1=='\\') {
      if (before_masked==VISUAL_INFO_STRING0 || before_masked==VISUAL_INFO_STRING1) {
        after |= VISUAL_INFO_STRINGESCAPE;
      }
    } else if (cp1=='"') {
      if (before_masked==0) {
        after |= VISUAL_INFO_STRING0;
      } else if (before_masked==VISUAL_INFO_STRING0) {
        after &= ~VISUAL_INFO_STRING0;
      }
    } else if (cp1=='\'') {
      if (before_masked==0) {
        after |= VISUAL_INFO_STRING1;
      } else if (before_masked==VISUAL_INFO_STRING1) {
        after &= ~VISUAL_INFO_STRING1;
      }
    } else if (cp1=='#') {
      if (before&(VISUAL_INFO_INDENTATION|VISUAL_INFO_NEWLINE)) {
        after |= VISUAL_INFO_PREPROCESSOR;
      }
    }
  }

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_INFO_INDENTATION;
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_INFO_WORD;
  }

  if ((before|after)&(VISUAL_INFO_STRING0|VISUAL_INFO_STRING1)) {
    *flags = VISUAL_FLAG_COLOR_STRING;
  } else if ((before|after)&(VISUAL_INFO_COMMENT0)) {
    *flags = VISUAL_FLAG_COLOR_BLOCKCOMMENT;
  } else if ((before|after)&(VISUAL_INFO_COMMENT1)) {
    *flags = VISUAL_FLAG_COLOR_LINECOMMENT;
  } else if ((after)&(VISUAL_INFO_INDENTATION)) {
    *length = 0;
    *flags = 0;
  } else {
    if (after&VISUAL_INFO_PREPROCESSOR) {
      if (cp1>' ' && (before&VISUAL_INFO_PREPROCESSOR)) {
        after &= ~VISUAL_INFO_PREPROCESSOR;
        *length = 0;
        *flags = file_type_keyword_config(base, cache, this->keywords_preprocessor, length, 0);
      }

      if (*flags==0) {
        if (after&VISUAL_INFO_PREPROCESSOR) {
          *flags = VISUAL_FLAG_COLOR_PREPROCESSOR;
          *length = 1;
        } else {
          *length = 0;
        }
      }
    } else {
      if (!(before&VISUAL_INFO_WORD) && (after&VISUAL_INFO_WORD)) {
        *length = 0;
        *flags = file_type_keyword_config(base, cache, this->keywords, length, 0);
      }

      if (*flags==0) {
        *length = 0;
      }
    }
  }

  *visual_detail = after;
}
