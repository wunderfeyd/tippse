/* Tippse - File type - C language */

#include "filetype_c.h"

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
  {"class", VISUAL_FLAG_COLOR_TYPE},
  {"public", VISUAL_FLAG_COLOR_TYPE},
  {"private", VISUAL_FLAG_COLOR_TYPE},
  {"protected", VISUAL_FLAG_COLOR_TYPE},
  {"virtual", VISUAL_FLAG_COLOR_TYPE},
  {"template", VISUAL_FLAG_COLOR_TYPE},
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
  {"using", VISUAL_FLAG_COLOR_KEYWORD},
  {"namespace", VISUAL_FLAG_COLOR_KEYWORD},
  {"new", VISUAL_FLAG_COLOR_KEYWORD},
  {"delete", VISUAL_FLAG_COLOR_KEYWORD},
  {"#include", VISUAL_FLAG_COLOR_PREPROCESSOR},
  {NULL, 0}
};

struct file_type* file_type_c_create() {
  struct file_type_c* this = malloc(sizeof(struct file_type_c));
  this->vtbl.create = file_type_c_create;
  this->vtbl.destroy = file_type_c_destroy;
  this->vtbl.keywords = file_type_c_keywords;
  this->vtbl.mark = file_type_c_mark;

  this->keywords = trie_create();
  trie_load_array(this->keywords, &keywords_language_c[0]);

  return (struct file_type*)this;
}

void file_type_c_destroy(struct file_type* base) {
  struct file_type_c* this = (struct file_type_c*)base;
  trie_destroy(this->keywords);
  free(this);
}

struct trie* file_type_c_keywords(struct file_type* base) {
  struct file_type_c* this = (struct file_type_c*)base;
  return this->keywords;
}

void file_type_c_mark(struct file_type* base, int* visual_detail, struct encoding* encoding, struct encoding_stream stream, int same_line, int* length, int* flags) {
  struct file_type_c* this = (struct file_type_c*)base;

  struct encoding_stream copy = stream;

  size_t cp_length = 0;
  int cp1 = (*encoding->decode)(encoding, &stream, ~0, &cp_length);
  encoding_stream_forward(&stream, cp_length);
  int cp2 = (*encoding->decode)(encoding, &stream, ~0, &cp_length);

  *length = 1;
  int before = *visual_detail;
  int before_masked = before&(~(VISUAL_INFO_INDENTATION|VISUAL_INFO_NEWLINE|VISUAL_INFO_WORD));
  int after = before;

  if (before_masked&VISUAL_INFO_STRINGESCAPE) {
    after &= ~VISUAL_INFO_STRINGESCAPE;
  } else if (cp1=='/' && cp2=='*' && before_masked==0) {
    *length = 2;
    after = VISUAL_INFO_COMMENT0;
  } else if (cp1=='*' && cp2=='/' && before_masked==VISUAL_INFO_COMMENT0) {
    *length = 2;
    after = 0;
  } else if (cp1=='/' && cp2=='/' && before_masked==0) {
    *length = 2;
    after = VISUAL_INFO_COMMENT1;
  } else if (cp1=='\n' && before_masked==VISUAL_INFO_COMMENT1) {
    after = 0;
  } else if (cp1=='\\' && (before_masked==VISUAL_INFO_STRING0 || before_masked==VISUAL_INFO_STRING1)) {
    after |= VISUAL_INFO_STRINGESCAPE;
  } else if (cp1=='"' && before_masked==0) {
    after = VISUAL_INFO_STRING0;
  } else if ((cp1=='"' || cp1=='\n') && before_masked==VISUAL_INFO_STRING0) {
    after = 0;
  } else if (cp1=='\'' && before_masked==0) {
    after = VISUAL_INFO_STRING1;
  } else if ((cp1=='\'' || cp1=='\n') && before_masked==VISUAL_INFO_STRING1) {
    after = 0;
  } else if (cp1!='\t' && cp1!=' ' && before_masked==0) {
    after = 0;
  } else if (before==VISUAL_INFO_NEWLINE) {
    after = VISUAL_INFO_INDENTATION;
  } else if ((cp1=='\0' || cp1=='\n')) {
    after = VISUAL_INFO_NEWLINE;
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_INFO_WORD;
  }

  *visual_detail = after;

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
    if (!(before&VISUAL_INFO_WORD)) {
      *length = 0;
      *flags = file_type_keyword(encoding, copy, this->keywords, length);
    }

    if (*flags==0) {
      *length = 0;
    }
  }
}
