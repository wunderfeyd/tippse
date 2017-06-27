/* Tippse - File type - C language */

#include "filetype_c.h"

struct trie_static keywords_language_c[] = {
  {"int", SYNTAX_COLOR_TYPE},
  {"unsigned", SYNTAX_COLOR_TYPE},
  {"signed", SYNTAX_COLOR_TYPE},
  {"char", SYNTAX_COLOR_TYPE},
  {"short", SYNTAX_COLOR_TYPE},
  {"long", SYNTAX_COLOR_TYPE},
  {"void", SYNTAX_COLOR_TYPE},
  {"bool", SYNTAX_COLOR_TYPE},
  {"size_t", SYNTAX_COLOR_TYPE},
  {"int8_t", SYNTAX_COLOR_TYPE},
  {"uint8_t", SYNTAX_COLOR_TYPE},
  {"int16_t", SYNTAX_COLOR_TYPE},
  {"uint16_t", SYNTAX_COLOR_TYPE},
  {"int32_t", SYNTAX_COLOR_TYPE},
  {"uint32_t", SYNTAX_COLOR_TYPE},
  {"int64_t", SYNTAX_COLOR_TYPE},
  {"uint64_t", SYNTAX_COLOR_TYPE},
  {"nullptr", SYNTAX_COLOR_TYPE},
  {"const", SYNTAX_COLOR_TYPE},
  {"struct", SYNTAX_COLOR_TYPE},
  {"class", SYNTAX_COLOR_TYPE},
  {"public", SYNTAX_COLOR_TYPE},
  {"private", SYNTAX_COLOR_TYPE},
  {"protected", SYNTAX_COLOR_TYPE},
  {"virtual", SYNTAX_COLOR_TYPE},
  {"template", SYNTAX_COLOR_TYPE},
  {"static", SYNTAX_COLOR_TYPE},
  {"inline", SYNTAX_COLOR_TYPE},
  {"for", SYNTAX_COLOR_KEYWORD},
  {"do", SYNTAX_COLOR_KEYWORD},
  {"while", SYNTAX_COLOR_KEYWORD},
  {"if", SYNTAX_COLOR_KEYWORD},
  {"else", SYNTAX_COLOR_KEYWORD},
  {"return", SYNTAX_COLOR_KEYWORD},
  {"break", SYNTAX_COLOR_KEYWORD},
  {"continue", SYNTAX_COLOR_KEYWORD},
  {"using", SYNTAX_COLOR_KEYWORD},
  {"namespace", SYNTAX_COLOR_KEYWORD},
  {"new", SYNTAX_COLOR_KEYWORD},
  {"delete", SYNTAX_COLOR_KEYWORD},
  {"//", SYNTAX_COLOR_KEYWORD|SYNTAX_LINECOMMENT},
  {"#include", SYNTAX_COLOR_PREPROCESSOR},
  {NULL, 0}
};

struct file_type* file_type_c_create() {
  struct file_type_c* this = malloc(sizeof(struct file_type_c));
  this->vtbl.create = file_type_c_create;
  this->vtbl.destroy = file_type_c_destroy;
  this->vtbl.keywords = file_type_c_keywords;

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
