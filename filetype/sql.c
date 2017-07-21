/* Tippse - File type - SQL */

#include "sql.h"

struct trie_static keywords_language_sql[] = {
  {"create", VISUAL_FLAG_COLOR_KEYWORD},
  {"database", VISUAL_FLAG_COLOR_KEYWORD},
  {"use", VISUAL_FLAG_COLOR_KEYWORD},
  {"from", VISUAL_FLAG_COLOR_KEYWORD},
  {"to", VISUAL_FLAG_COLOR_KEYWORD},
  {"into", VISUAL_FLAG_COLOR_KEYWORD},
  {"join", VISUAL_FLAG_COLOR_KEYWORD},
  {"where", VISUAL_FLAG_COLOR_KEYWORD},
  {"order", VISUAL_FLAG_COLOR_KEYWORD},
  {"group", VISUAL_FLAG_COLOR_KEYWORD},
  {"by", VISUAL_FLAG_COLOR_KEYWORD},
  {"on", VISUAL_FLAG_COLOR_KEYWORD},
  {"as", VISUAL_FLAG_COLOR_KEYWORD},
  {"is", VISUAL_FLAG_COLOR_KEYWORD},
  {"in", VISUAL_FLAG_COLOR_KEYWORD},
  {"out", VISUAL_FLAG_COLOR_KEYWORD},
  {"begin", VISUAL_FLAG_COLOR_KEYWORD},
  {"end", VISUAL_FLAG_COLOR_KEYWORD},
  {"delimiter", VISUAL_FLAG_COLOR_KEYWORD},
  {"sql", VISUAL_FLAG_COLOR_KEYWORD},
  {"data", VISUAL_FLAG_COLOR_KEYWORD},
  {"insert", VISUAL_FLAG_COLOR_KEYWORD},
  {"select", VISUAL_FLAG_COLOR_KEYWORD},
  {"update", VISUAL_FLAG_COLOR_KEYWORD},
  {"delete", VISUAL_FLAG_COLOR_KEYWORD},
  {"replace", VISUAL_FLAG_COLOR_KEYWORD},
  {"drop", VISUAL_FLAG_COLOR_KEYWORD},
  {"duplicate", VISUAL_FLAG_COLOR_KEYWORD},
  {"temporary", VISUAL_FLAG_COLOR_KEYWORD},
  {"modifies", VISUAL_FLAG_COLOR_KEYWORD},
  {"reads", VISUAL_FLAG_COLOR_KEYWORD},
  {"returns", VISUAL_FLAG_COLOR_KEYWORD},
  {"exists", VISUAL_FLAG_COLOR_KEYWORD},
  {"return", VISUAL_FLAG_COLOR_KEYWORD},
  {"leave", VISUAL_FLAG_COLOR_KEYWORD},
  {"call", VISUAL_FLAG_COLOR_KEYWORD},
  {"execute", VISUAL_FLAG_COLOR_KEYWORD},
  {"show", VISUAL_FLAG_COLOR_KEYWORD},
  {"function", VISUAL_FLAG_COLOR_KEYWORD},
  {"procedure", VISUAL_FLAG_COLOR_KEYWORD},
  {"declare", VISUAL_FLAG_COLOR_KEYWORD},
  {"if", VISUAL_FLAG_COLOR_KEYWORD},
  {"then", VISUAL_FLAG_COLOR_KEYWORD},
  {"elseif", VISUAL_FLAG_COLOR_KEYWORD},
  {"else", VISUAL_FLAG_COLOR_KEYWORD},
  {"not", VISUAL_FLAG_COLOR_KEYWORD},
  {"table", VISUAL_FLAG_COLOR_KEYWORD},
  {"values", VISUAL_FLAG_COLOR_KEYWORD},
  {"key", VISUAL_FLAG_COLOR_KEYWORD},
  {"unique", VISUAL_FLAG_COLOR_KEYWORD},
  {"primary", VISUAL_FLAG_COLOR_KEYWORD},
  {"foreign", VISUAL_FLAG_COLOR_KEYWORD},
  {"constraint", VISUAL_FLAG_COLOR_KEYWORD},
  {"references", VISUAL_FLAG_COLOR_KEYWORD},
  {"cascade", VISUAL_FLAG_COLOR_KEYWORD},
  {"default", VISUAL_FLAG_COLOR_KEYWORD},
  {"engine", VISUAL_FLAG_COLOR_KEYWORD},
  {"charset", VISUAL_FLAG_COLOR_KEYWORD},
  {"auto_increment", VISUAL_FLAG_COLOR_KEYWORD},
  {"definer", VISUAL_FLAG_COLOR_KEYWORD},
  {"and", VISUAL_FLAG_COLOR_KEYWORD},
  {"or", VISUAL_FLAG_COLOR_KEYWORD},
  {"set", VISUAL_FLAG_COLOR_KEYWORD},
  {"count", VISUAL_FLAG_COLOR_KEYWORD},
  {"limit", VISUAL_FLAG_COLOR_KEYWORD},
  {"now", VISUAL_FLAG_COLOR_TYPE},
  {"max", VISUAL_FLAG_COLOR_TYPE},
  {"min", VISUAL_FLAG_COLOR_TYPE},
  {"sum", VISUAL_FLAG_COLOR_TYPE},
  {"null", VISUAL_FLAG_COLOR_TYPE},
  {"bit", VISUAL_FLAG_COLOR_TYPE},
  {"int", VISUAL_FLAG_COLOR_TYPE},
  {"bigint", VISUAL_FLAG_COLOR_TYPE},
  {"char", VISUAL_FLAG_COLOR_TYPE},
  {"varchar", VISUAL_FLAG_COLOR_TYPE},
  {"string", VISUAL_FLAG_COLOR_TYPE},
  {"boolean", VISUAL_FLAG_COLOR_TYPE},
  {"short", VISUAL_FLAG_COLOR_TYPE},
  {"date", VISUAL_FLAG_COLOR_TYPE},
  {"time", VISUAL_FLAG_COLOR_TYPE},
  {"datetime", VISUAL_FLAG_COLOR_TYPE},
  {"decimal", VISUAL_FLAG_COLOR_TYPE},
  {"true", VISUAL_FLAG_COLOR_TYPE},
  {"false", VISUAL_FLAG_COLOR_TYPE},
  {NULL, 0}
};

struct file_type* file_type_sql_create() {
  struct file_type_sql* this = malloc(sizeof(struct file_type_sql));
  this->vtbl.create = file_type_sql_create;
  this->vtbl.destroy = file_type_sql_destroy;
  this->vtbl.name = file_type_sql_name;
  this->vtbl.mark = file_type_sql_mark;

  this->keywords = trie_create();
  trie_load_array_nocase(this->keywords, &keywords_language_sql[0]);

  return (struct file_type*)this;
}

void file_type_sql_destroy(struct file_type* base) {
  struct file_type_sql* this = (struct file_type_sql*)base;
  trie_destroy(this->keywords);
  free(this);
}

const char* file_type_sql_name() {
  return "SQL";
}

void file_type_sql_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  struct file_type_sql* this = (struct file_type_sql*)base;

  int cp1 = encoding_cache_find_codepoint(cache, 0);
  int cp2 = encoding_cache_find_codepoint(cache, 1);

  *length = 1;
  int before = *visual_detail;
  int before_masked = before&(~(VISUAL_INFO_INDENTATION|VISUAL_INFO_NEWLINE|VISUAL_INFO_WORD));
  int after = before;

  if (before_masked&VISUAL_INFO_STRINGESCAPE) {
    after &= ~VISUAL_INFO_STRINGESCAPE;
  } else if (cp1=='/' && cp2=='*' && before_masked==0) {
    *length = 2;
    after |= VISUAL_INFO_COMMENT0;
  } else if (cp1=='*' && cp2=='/' && before_masked==VISUAL_INFO_COMMENT0) {
    *length = 2;
    after &= ~VISUAL_INFO_COMMENT0;
  } else if (cp1=='-' && cp2=='-' && before_masked==0) {
    *length = 2;
    after |= VISUAL_INFO_COMMENT1;
  } else if (cp1=='\n' && before_masked==VISUAL_INFO_COMMENT1) {
    after &= ~VISUAL_INFO_COMMENT1;
  } else if (cp1=='\\' && (before_masked==VISUAL_INFO_STRING0 || before_masked==VISUAL_INFO_STRING1 || before_masked==VISUAL_INFO_STRING2)) {
    after |= VISUAL_INFO_STRINGESCAPE;
  } else if (cp1=='"' && before_masked==0) {
    after |= VISUAL_INFO_STRING0;
  } else if ((cp1=='"' || cp1=='\n') && before_masked==VISUAL_INFO_STRING0) {
    after &= ~VISUAL_INFO_STRING0;
  } else if (cp1=='\'' && before_masked==0) {
    after |= VISUAL_INFO_STRING1;
  } else if ((cp1=='\'' || cp1=='\n') && before_masked==VISUAL_INFO_STRING1) {
    after &= ~VISUAL_INFO_STRING1;
  } else if (cp1=='`' && before_masked==0) {
    after |= VISUAL_INFO_STRING2;
  } else if ((cp1=='`' || cp1=='\n') && before_masked==VISUAL_INFO_STRING2) {
    after &= ~VISUAL_INFO_STRING2;
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
  }

  if ((cp1>='a' && cp1<='z') || (cp1>='A' && cp1<='Z') || (cp1>='0' && cp1<='9') || cp1=='_') {
    after |= VISUAL_INFO_WORD;
  } else {
    after &= ~VISUAL_INFO_WORD;
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
      *flags = file_type_keyword(cache, this->keywords, length);
    }

    if (*flags==0) {
      *length = 0;
    }
  }

  *visual_detail = after;
}
