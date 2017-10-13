/* Tippse - File type - PHP language */

#include "php.h"

struct trie_static keywords_language_php[] = {
  {"$this", VISUAL_FLAG_COLOR_TYPE},
  {"const", VISUAL_FLAG_COLOR_TYPE},
  {"var", VISUAL_FLAG_COLOR_TYPE},
  {"true", VISUAL_FLAG_COLOR_TYPE},
  {"false", VISUAL_FLAG_COLOR_TYPE},
  {"max", VISUAL_FLAG_COLOR_TYPE},
  {"min", VISUAL_FLAG_COLOR_TYPE},
  {"is_set", VISUAL_FLAG_COLOR_TYPE},
  {"is_null", VISUAL_FLAG_COLOR_TYPE},
  {"empty", VISUAL_FLAG_COLOR_TYPE},
  {"as", VISUAL_FLAG_COLOR_KEYWORD},
  {"foreach", VISUAL_FLAG_COLOR_KEYWORD},
  {"for", VISUAL_FLAG_COLOR_KEYWORD},
  {"do", VISUAL_FLAG_COLOR_KEYWORD},
  {"while", VISUAL_FLAG_COLOR_KEYWORD},
  {"if", VISUAL_FLAG_COLOR_KEYWORD},
  {"else", VISUAL_FLAG_COLOR_KEYWORD},
  {"return", VISUAL_FLAG_COLOR_KEYWORD},
  {"break", VISUAL_FLAG_COLOR_KEYWORD},
  {"continue", VISUAL_FLAG_COLOR_KEYWORD},
  {"function", VISUAL_FLAG_COLOR_KEYWORD},
  {"defined", VISUAL_FLAG_COLOR_KEYWORD},
  {"echo", VISUAL_FLAG_COLOR_KEYWORD},
  {"class", VISUAL_FLAG_COLOR_KEYWORD},
  {"global", VISUAL_FLAG_COLOR_KEYWORD},
  {"include", VISUAL_FLAG_COLOR_KEYWORD},
  {"require", VISUAL_FLAG_COLOR_KEYWORD},
  {"require_once", VISUAL_FLAG_COLOR_KEYWORD},
  {"switch", VISUAL_FLAG_COLOR_KEYWORD},
  {"case", VISUAL_FLAG_COLOR_KEYWORD},
  {"default", VISUAL_FLAG_COLOR_KEYWORD},
  {NULL, 0}
};

struct file_type* file_type_php_create(void) {
  struct file_type_php* this = malloc(sizeof(struct file_type_php));
  this->vtbl.create = file_type_php_create;
  this->vtbl.destroy = file_type_php_destroy;
  this->vtbl.name = file_type_php_name;
  this->vtbl.mark = file_type_php_mark;
  this->vtbl.bracket_match = file_type_bracket_match;

  this->keywords = trie_create();
  trie_load_array(this->keywords, &keywords_language_php[0]);

  return (struct file_type*)this;
}

void file_type_php_destroy(struct file_type* base) {
  struct file_type_php* this = (struct file_type_php*)base;
  trie_destroy(this->keywords);
  free(this);
}

const char* file_type_php_name(void) {
  return "PHP";
}

void file_type_php_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  struct file_type_php* this = (struct file_type_php*)base;

  int cp1 = encoding_cache_find_codepoint(cache, 0);
  int cp2 = encoding_cache_find_codepoint(cache, 1);

  *length = 1;
  int before = *visual_detail;
  if (before&VISUAL_INFO_NEWLINE) {
    before &= ~(VISUAL_INFO_STRING0|VISUAL_INFO_STRING1|VISUAL_INFO_COMMENT1);
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
    }
  }

  if (cp1=='\t' || cp1==' ') {
    after |= VISUAL_INFO_INDENTATION;
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_' || cp1=='$') {
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
    if (!(before&VISUAL_INFO_WORD) && (after&VISUAL_INFO_WORD)) {
      *length = 0;
      *flags = file_type_keyword(cache, this->keywords, length);
    }

    if (*flags==0) {
      *length = 0;
    }
  }

  *visual_detail = after;
}
