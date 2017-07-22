/* Tippse - File type - C language */

#include "c.h"

struct trie_static keywords_language_c[] = {
  {"int", VISUAL_FLAG_COLOR_TYPE},
  {"unsigned", VISUAL_FLAG_COLOR_TYPE},
  {"signed", VISUAL_FLAG_COLOR_TYPE},
  {"char", VISUAL_FLAG_COLOR_TYPE},
  {"short", VISUAL_FLAG_COLOR_TYPE},
  {"long", VISUAL_FLAG_COLOR_TYPE},
  {"void", VISUAL_FLAG_COLOR_TYPE},
  {"bool", VISUAL_FLAG_COLOR_TYPE},
  {"size_t", VISUAL_FLAG_COLOR_TYPE},
  {"int8_t", VISUAL_FLAG_COLOR_TYPE},
  {"uint8_t", VISUAL_FLAG_COLOR_TYPE},
  {"int16_t", VISUAL_FLAG_COLOR_TYPE},
  {"uint16_t", VISUAL_FLAG_COLOR_TYPE},
  {"int32_t", VISUAL_FLAG_COLOR_TYPE},
  {"uint32_t", VISUAL_FLAG_COLOR_TYPE},
  {"int64_t", VISUAL_FLAG_COLOR_TYPE},
  {"uint64_t", VISUAL_FLAG_COLOR_TYPE},
  {"nullptr", VISUAL_FLAG_COLOR_TYPE},
  {"const", VISUAL_FLAG_COLOR_TYPE},
  {"struct", VISUAL_FLAG_COLOR_TYPE},
  {"static", VISUAL_FLAG_COLOR_TYPE},
  {"inline", VISUAL_FLAG_COLOR_TYPE},
  {"for", VISUAL_FLAG_COLOR_KEYWORD},
  {"do", VISUAL_FLAG_COLOR_KEYWORD},
  {"while", VISUAL_FLAG_COLOR_KEYWORD},
  {"if", VISUAL_FLAG_COLOR_KEYWORD},
  {"else", VISUAL_FLAG_COLOR_KEYWORD},
  {"return", VISUAL_FLAG_COLOR_KEYWORD},
  {"break", VISUAL_FLAG_COLOR_KEYWORD},
  {"continue", VISUAL_FLAG_COLOR_KEYWORD},
  {"sizeof", VISUAL_FLAG_COLOR_KEYWORD},
  {NULL, 0}
};

struct trie_static keywords_preprocessor_language_c[] = {
  {"include", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {"define", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {"if", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {"ifdef", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {"ifndef", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {"else", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {"elif", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {"endif", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {NULL, 0}
};

struct file_type* file_type_c_create() {
  struct file_type_c* this = malloc(sizeof(struct file_type_c));
  this->vtbl.create = file_type_c_create;
  this->vtbl.destroy = file_type_c_destroy;
  this->vtbl.name = file_type_c_name;
  this->vtbl.mark = file_type_c_mark;

  this->keywords = trie_create();
  this->keywords_preprocessor = trie_create();
  trie_load_array(this->keywords, &keywords_language_c[0]);
  trie_load_array(this->keywords_preprocessor, &keywords_preprocessor_language_c[0]);

  return (struct file_type*)this;
}

void file_type_c_destroy(struct file_type* base) {
  struct file_type_c* this = (struct file_type_c*)base;
  trie_destroy(this->keywords_preprocessor);
  trie_destroy(this->keywords);
  free(this);
}

const char* file_type_c_name() {
  return "C";
}

void file_type_c_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  struct file_type_c* this = (struct file_type_c*)base;

  int cp1 = encoding_cache_find_codepoint(cache, 0);
  int cp2 = encoding_cache_find_codepoint(cache, 1);

  *length = 1;
  int before = *visual_detail;
  int before_masked = before&(~(VISUAL_INFO_INDENTATION|VISUAL_INFO_NEWLINE|VISUAL_INFO_WORD));
  int after = before;

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
  
  if (before&VISUAL_INFO_NEWLINE) {
    after |= VISUAL_INFO_INDENTATION;
    after &= ~VISUAL_INFO_NEWLINE;
  }

  if (cp1!='\t' && cp1!=' ') {
    after &= ~VISUAL_INFO_INDENTATION;
  }

  if (cp1=='\0' || cp1=='\n') {
    after |= VISUAL_INFO_NEWLINE;
    after &= ~(VISUAL_INFO_PREPROCESSOR|VISUAL_INFO_STRING0|VISUAL_INFO_STRING1|VISUAL_INFO_COMMENT1);
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_INFO_WORD;
  } else {
    after &= ~VISUAL_INFO_WORD;
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
        *flags = file_type_keyword(cache, this->keywords_preprocessor, length);
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
        *flags = file_type_keyword(cache, this->keywords, length);
      }

      if (*flags==0) {
        *length = 0;
      }
    }
  }

  *visual_detail = after;
}
