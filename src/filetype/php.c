// Tippse - File type - PHP language

#include "php.h"

struct file_type* file_type_php_create(struct config* config) {
  struct file_type_php* this = malloc(sizeof(struct file_type_php));
  this->vtbl.config = config;
  this->vtbl.create = file_type_php_create;
  this->vtbl.destroy = file_type_php_destroy;
  this->vtbl.name = file_type_php_name;
  this->vtbl.mark = file_type_php_mark;
  this->vtbl.bracket_match = file_type_bracket_match;

  this->keywords = file_type_config_base((struct file_type*)this, "colors/keywords");

  return (struct file_type*)this;
}

void file_type_php_destroy(struct file_type* base) {
  struct file_type_php* this = (struct file_type_php*)base;
  free(this);
}

const char* file_type_php_name(void) {
  return "PHP";
}

void file_type_php_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  struct file_type_php* this = (struct file_type_php*)base;

  codepoint_t cp1 = encoding_cache_find_codepoint(cache, 0);
  codepoint_t cp2 = encoding_cache_find_codepoint(cache, 1);

  *length = 1;
  int before = *visual_detail;
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
          *length = 2;
          after |= VISUAL_DETAIL_COMMENT0;
        } else if (cp2=='/') {
          *length = 2;
          after |= VISUAL_DETAIL_COMMENT1;
        }
      }
    } else if (cp1=='*') {
      if (cp2=='/' && before_masked==VISUAL_DETAIL_COMMENT0) {
        *length = 2;
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

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_' || cp1=='$') {
    after |= VISUAL_DETAIL_WORD;
  }

  if ((before|after)&(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1)) {
    *flags = VISUAL_FLAG_COLOR_STRING;
  } else if ((before|after)&(VISUAL_DETAIL_COMMENT0)) {
    *flags = VISUAL_FLAG_COLOR_BLOCKCOMMENT;
  } else if ((before|after)&(VISUAL_DETAIL_COMMENT1)) {
    *flags = VISUAL_FLAG_COLOR_LINECOMMENT;
  } else if ((after)&(VISUAL_DETAIL_INDENTATION)) {
    *length = 0;
    *flags = 0;
  } else {
    if (!(before&VISUAL_DETAIL_WORD) && (after&VISUAL_DETAIL_WORD)) {
      *length = 0;
      *flags = file_type_keyword_config(base, cache, this->keywords, length, 0);
    }

    if (*flags==0) {
      *length = 0;
    }
  }

  *visual_detail = after;
}
