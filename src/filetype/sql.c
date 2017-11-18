// Tippse - File type - SQL

#include "sql.h"

struct file_type* file_type_sql_create(struct config* config) {
  struct file_type_sql* this = malloc(sizeof(struct file_type_sql));
  this->vtbl.config = config;
  this->vtbl.create = file_type_sql_create;
  this->vtbl.destroy = file_type_sql_destroy;
  this->vtbl.name = file_type_sql_name;
  this->vtbl.mark = file_type_sql_mark;
  this->vtbl.bracket_match = file_type_bracket_match;

  this->keywords = file_type_config_base((struct file_type*)this, "colors/keywords");

  return (struct file_type*)this;
}

void file_type_sql_destroy(struct file_type* base) {
  struct file_type_sql* this = (struct file_type_sql*)base;
  free(this);
}

const char* file_type_sql_name(void) {
  return "SQL";
}

void file_type_sql_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  struct file_type_sql* this = (struct file_type_sql*)base;

  int cp1 = encoding_cache_find_codepoint(cache, 0);
  int cp2 = encoding_cache_find_codepoint(cache, 1);

  *length = 1;
  int before = *visual_detail;
  if (before&VISUAL_INFO_NEWLINE) {
    before &= ~(VISUAL_INFO_STRING0|VISUAL_INFO_STRING1|VISUAL_INFO_STRING2|VISUAL_INFO_COMMENT1);
  }

  int before_masked = before&VISUAL_INFO_STATEMASK;
  int after = before&~(VISUAL_INFO_INDENTATION|VISUAL_INFO_WORD);

  if (before_masked&VISUAL_INFO_STRINGESCAPE) {
    after &= ~VISUAL_INFO_STRINGESCAPE;
  } else {
    if (cp1=='/') {
      if (cp2=='*' && before_masked==0) {
        *length = 2;
        after |= VISUAL_INFO_COMMENT0;
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
    } else if (cp1=='-') {
      if (cp2=='-') {
        *length = 2;
        after |= VISUAL_INFO_COMMENT1;
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
    } else if (cp1=='`') {
      if (before_masked==0) {
        after |= VISUAL_INFO_STRING2;
      } else if (before_masked==VISUAL_INFO_STRING2) {
        after &= ~VISUAL_INFO_STRING2;
      }
    }
  }

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_INFO_INDENTATION;
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_INFO_WORD;
  }

  if ((before|after)&(VISUAL_INFO_STRING0|VISUAL_INFO_STRING1|VISUAL_INFO_STRING2)) {
    *flags = VISUAL_FLAG_COLOR_STRING;
  } else if ((before|after)&(VISUAL_INFO_COMMENT0)) {
    *flags = VISUAL_FLAG_COLOR_BLOCKCOMMENT;
  } else if ((before|after)&(VISUAL_INFO_COMMENT1)) {
    *flags = VISUAL_FLAG_COLOR_LINECOMMENT;
  } else if ((after)&(VISUAL_INFO_INDENTATION)) {
    *length = 0;
    *flags = 0;
  } else {
    if (!(before&VISUAL_INFO_WORD) && (after&VISUAL_INFO_WORD)) {
      *length = 0;
      *flags = file_type_keyword_config(base, cache, this->keywords, length, 1);
    }

    if (*flags==0) {
      *length = 0;
    }
  }

  *visual_detail = after;
}
