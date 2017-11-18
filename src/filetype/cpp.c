// Tippse - File type - C++ language

#include "cpp.h"

struct file_type* file_type_cpp_create(struct config* config) {
  struct file_type_cpp* this = malloc(sizeof(struct file_type_cpp));
  this->vtbl.config = config;
  this->vtbl.create = file_type_cpp_create;
  this->vtbl.destroy = file_type_cpp_destroy;
  this->vtbl.name = file_type_cpp_name;
  this->vtbl.mark = file_type_c_mark;
  this->vtbl.bracket_match = file_type_bracket_match;

  this->keywords = file_type_config_base((struct file_type*)this, "colors/keywords");
  this->keywords_preprocessor = file_type_config_base((struct file_type*)this, "colors/preprocessor");

  return (struct file_type*)this;
}

void file_type_cpp_destroy(struct file_type* base) {
  struct file_type_cpp* this = (struct file_type_cpp*)base;
  free(this);
}

const char* file_type_cpp_name(void) {
  return "C++";
}
