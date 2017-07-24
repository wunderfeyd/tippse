/* Tippse - File type - XML */

#include "xml.h"

struct trie_static keywords_language_xml[] = {
  {NULL, 0}
};

struct file_type* file_type_xml_create() {
  struct file_type_xml* this = malloc(sizeof(struct file_type_xml));
  this->vtbl.create = file_type_xml_create;
  this->vtbl.destroy = file_type_xml_destroy;
  this->vtbl.name = file_type_xml_name;
  this->vtbl.mark = file_type_xml_mark;

  this->keywords = trie_create();
  trie_load_array(this->keywords, &keywords_language_xml[0]);

  return (struct file_type*)this;
}

void file_type_xml_destroy(struct file_type* base) {
  struct file_type_xml* this = (struct file_type_xml*)base;
  trie_destroy(this->keywords);
  free(this);
}

const char* file_type_xml_name() {
  return "XML";
}

void file_type_xml_mark(struct file_type* base, int* visual_detail, struct encoding_cache* cache, int same_line, int* length, int* flags) {
  struct file_type_xml* this = (struct file_type_xml*)base;

  int cp1 = encoding_cache_find_codepoint(cache, 0);
  int cp2 = encoding_cache_find_codepoint(cache, 1);

  *length = 1;
  int before = *visual_detail;
  int before_masked = before&(~(VISUAL_INFO_INDENTATION|VISUAL_INFO_NEWLINE|VISUAL_INFO_WORD|VISUAL_INFO_WHITESPACED_COMPLETE|VISUAL_INFO_WHITESPACED_START));
  int after = before;

  if (before_masked&VISUAL_INFO_STRINGESCAPE) {
    after &= ~VISUAL_INFO_STRINGESCAPE;
  } else {
    if (cp1=='<') {
      if (before_masked==0) {
        after |= VISUAL_INFO_COMMENT2;
      }
    } else if (cp1=='>') {
      if (before_masked==VISUAL_INFO_COMMENT2) {
        after &= ~VISUAL_INFO_COMMENT2;
      }
    } else if (cp1=='-') {
      if (cp2=='-') {
        if (before_masked==VISUAL_INFO_COMMENT2) {
          *length = 2;
          after |= VISUAL_INFO_COMMENT0;
        } else if (before_masked==(VISUAL_INFO_COMMENT2|VISUAL_INFO_COMMENT0)) {
          *length = 2;
          after &= ~VISUAL_INFO_COMMENT0;
        }
      }
    } else if (cp1=='"') {
      if (before_masked&VISUAL_INFO_STRING0) {
        after &= ~VISUAL_INFO_STRING0;
      } else if (before_masked==VISUAL_INFO_COMMENT2) {
        after |= VISUAL_INFO_STRING0;
      }
    } else if (cp1=='\'') {
      if (before_masked&VISUAL_INFO_STRING1) {
        after &= ~VISUAL_INFO_STRING1;
      } else if (before_masked==VISUAL_INFO_COMMENT2) {
        after |= VISUAL_INFO_STRING1;
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
    after &= ~(VISUAL_INFO_STRING0|VISUAL_INFO_STRING1);
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
  } else if ((before|after)&(VISUAL_INFO_COMMENT2)) {
    *flags = VISUAL_FLAG_COLOR_TYPE;
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
