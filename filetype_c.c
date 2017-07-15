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

  // TODO: the reverse lookup should be replaced by the visual state
  struct range_tree_node* node_prev = stream.buffer;
  file_offset_t buffer_pos_prev = stream.buffer_pos;
  while (buffer_pos_prev==0) {
    node_prev = range_tree_prev(node_prev);
    if (!node_prev) {
      break;
    }

    buffer_pos_prev = node_prev->length;
  }
  buffer_pos_prev--;

  int cp0 = node_prev?*(node_prev->buffer->buffer+node_prev->offset+buffer_pos_prev):'\0';

  size_t cp_length = 0;
  int cp1 = (*encoding->decode)(encoding, &stream, ~0, &cp_length);
  encoding_stream_forward(&stream, cp_length);
  int cp2 = (*encoding->decode)(encoding, &stream, ~0, &cp_length);

  *length = 1;
  int before = *visual_detail;
  int before_masked = before&(~VISUAL_INFO_INDENTATION);
  if ((*visual_detail)&VISUAL_INFO_STRINGESCAPE) {
    *visual_detail &= ~VISUAL_INFO_STRINGESCAPE;
  } else if (cp1=='/' && cp2=='*' && before_masked==0) {
    *length = 2;
    *visual_detail = VISUAL_INFO_COMMENT0;
  } else if (cp1=='*' && cp2=='/' && before_masked==VISUAL_INFO_COMMENT0) {
    *length = 2;
    *visual_detail = 0;
  } else if (cp1=='/' && cp2=='/' && before_masked==0) {
    *length = 2;
    *visual_detail = VISUAL_INFO_COMMENT1;
  } else if (cp1=='\n' && before_masked==VISUAL_INFO_COMMENT1) {
    *visual_detail = 0;
  } else if (cp1=='\\' && (before_masked==VISUAL_INFO_STRING0 || before_masked==VISUAL_INFO_STRING1)) {
    *visual_detail |= VISUAL_INFO_STRINGESCAPE;
  } else if (cp1=='"' && before_masked==0) {
    *visual_detail = VISUAL_INFO_STRING0;
  } else if ((cp1=='"' || cp1=='\n') && before_masked==VISUAL_INFO_STRING0) {
    *visual_detail = 0;
  } else if (cp1=='\'' && before_masked==0) {
    *visual_detail = VISUAL_INFO_STRING1;
  } else if ((cp1=='\'' || cp1=='\n') && before_masked==VISUAL_INFO_STRING1) {
    *visual_detail = 0;
  } else if (cp1!='\t' && cp1!=' ' && before_masked==0) {
    *visual_detail = 0;
  } else if ((cp0=='\0' || cp0=='\n') && before==0) {
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
    *length = 0;
    *flags = 0;
  } else {
    if (same_line && (cp0<'a' || cp0>'z') && (cp0<'A' || cp0>'Z') && (cp0<'0' || cp0>'9') && cp0!='_') {
      *length = 0;
      *flags = file_type_keyword(encoding, copy, this->keywords, length);
    }

    if (*flags==0) {
      *length = 0;
    }
  }
}
