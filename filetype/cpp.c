/* Tippse - File type - C++ language */

#include "cpp.h"

struct trie_static keywords_language_cpp[] = {
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
  {"sizeof", VISUAL_FLAG_COLOR_KEYWORD},
  {NULL, 0}
};

extern struct trie_static keywords_preprocessor_language_c[];

struct file_type* file_type_cpp_create() {
  struct file_type_cpp* this = malloc(sizeof(struct file_type_cpp));
  this->vtbl.create = file_type_cpp_create;
  this->vtbl.destroy = file_type_cpp_destroy;
  this->vtbl.name = file_type_cpp_name;
  this->vtbl.mark = file_type_c_mark;
  this->vtbl.bracket_match = file_type_bracket_match;

  this->keywords = trie_create();
  this->keywords_preprocessor = trie_create();
  trie_load_array(this->keywords, &keywords_language_cpp[0]);
  trie_load_array(this->keywords_preprocessor, &keywords_preprocessor_language_c[0]);

  return (struct file_type*)this;
}

void file_type_cpp_destroy(struct file_type* base) {
  struct file_type_cpp* this = (struct file_type_cpp*)base;
  trie_destroy(this->keywords_preprocessor);
  trie_destroy(this->keywords);
  free(this);
}

const char* file_type_cpp_name() {
  return "C++";
}
