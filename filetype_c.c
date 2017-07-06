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

void file_type_c_mark(struct file_type* base, int* visual_detail, struct range_tree_node* node, file_offset_t buffer_pos, int same_line, int* length, int* flags) {
  struct file_type_c* this = (struct file_type_c*)base;

  struct range_tree_node* node_prev = node;
  file_offset_t buffer_pos_prev = buffer_pos;
  while (buffer_pos_prev==0) {
    node_prev = range_tree_prev(node_prev);
    if (!node_prev) {
      break;
    }

    buffer_pos_prev = node_prev->length;
  }
  buffer_pos_prev--;

  const char* text0 = node_prev?node_prev->buffer->buffer+node_prev->offset+buffer_pos_prev:"\0";

  const char* text1 = node->buffer->buffer+node->offset+buffer_pos;

  struct range_tree_node* node_next = node;
  file_offset_t buffer_pos_next = buffer_pos+1;
  if (buffer_pos_next==node_next->length) {
    buffer_pos_next = 0;
    node_next = range_tree_next(node_next);
  }

  const char* text2 = node_next?node_next->buffer->buffer+node_next->offset+buffer_pos_next:" ";

  *length = 1;
  int before = *visual_detail;
  int before_masked = before&(~VISUAL_INFO_INDENTATION);
  if ((*visual_detail)&VISUAL_INFO_STRINGESCAPE) {
    *visual_detail &= ~VISUAL_INFO_STRINGESCAPE;
  } else if (*text1=='/' && *text2=='*' && before_masked==0) {
    *length = 2;
    *visual_detail = VISUAL_INFO_COMMENT0;
  } else if (*text1=='*' && *text2=='/' && before_masked==VISUAL_INFO_COMMENT0) {
    *length = 2;
    *visual_detail = 0;
  } else if (*text1=='/' && *text2=='/' && before_masked==0) {
    *length = 2;
    *visual_detail = VISUAL_INFO_COMMENT1;
  } else if (*text1=='\n' && before_masked==VISUAL_INFO_COMMENT1) {
    *visual_detail = 0;
  } else if (*text1=='\\' && (before_masked==VISUAL_INFO_STRING0 || before_masked==VISUAL_INFO_STRING1)) {
    *visual_detail |= VISUAL_INFO_STRINGESCAPE;
  } else if (*text1=='"' && before_masked==0) {
    *visual_detail = VISUAL_INFO_STRING0;
  } else if (*text1=='"' && before_masked==VISUAL_INFO_STRING0) {
    *visual_detail = 0;
  } else if (*text1=='\'' && before_masked==0) {
    *visual_detail = VISUAL_INFO_STRING1;
  } else if (*text1=='\'' && before_masked==VISUAL_INFO_STRING1) {
    *visual_detail = 0;
  } else if (*text1!='\t' && *text1!=' ' && before_masked==0) {
    *visual_detail = 0;
  } else if ((*text0=='\0' || *text0=='\n') && before==0) {
    *visual_detail = VISUAL_INFO_INDENTATION;
  }
  int after = *visual_detail;
  if ((before|after)&(VISUAL_INFO_STRING0|VISUAL_INFO_STRING1)) {
    *flags = VISUAL_FLAG_COLOR_STRING;
  } else if ((before|after)&(VISUAL_INFO_COMMENT0)) {
    *flags = VISUAL_FLAG_COLOR_BLOCKCOMMENT;
  } else if ((before|after)&(VISUAL_INFO_COMMENT1)) {
    *flags = VISUAL_FLAG_COLOR_LINECOMMENT;
  } else if ((after)&(VISUAL_INFO_INDENTATION)) {
    *flags = 0;
  } else {
    int cp = *text0;
    if (same_line && (cp<'a' || cp>'z') && (cp<'A' || cp>'Z') && (cp<'0' || cp>'9') && cp!='_') {
      *flags = file_type_keyword(node, buffer_pos, this->keywords, length);
    }
    if (*flags==0) {
      *length = 0;
    }
  }
}
