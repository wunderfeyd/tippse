/* Tippse - File type - Interface and helper functions for specialized document displaying and modification */

#include "filetype.h"

int file_type_keyword(struct encoding_cache* cache, struct trie* trie, int* keyword_length) {
  struct trie_node* parent = NULL;
  size_t pos = 0;
  while (1) {
    int cp = encoding_cache_find_codepoint(cache, pos++);
    parent = trie_find_codepoint(trie, parent, cp);

    if (!parent) {
      break;
    }

    if (parent->type!=0) {
      int cp = encoding_cache_find_codepoint(cache, pos);
      if ((cp<'a' || cp>'z') && (cp<'A' || cp>'Z') && (cp<'0' || cp>'9') && cp!='_') {
        *keyword_length = (int)pos;
        return (int)parent->type;
      }
    }
  }

  return 0;
}

int file_type_bracket_match(int visual_detail, int* cp, size_t length) {
  if ((visual_detail&(VISUAL_INFO_STRING0|VISUAL_INFO_STRING1|VISUAL_INFO_COMMENT0|VISUAL_INFO_COMMENT1))==0) {
    int cp1 = *cp;
    if (cp1=='{') {
      return 0|VISUAL_BRACKET_OPEN;
    } else if (cp1=='[') {
      return 1|VISUAL_BRACKET_OPEN;
    } else if (cp1=='(') {
      return 2|VISUAL_BRACKET_OPEN;
    } else if (cp1=='}') {
      return 0|VISUAL_BRACKET_CLOSE;
    } else if (cp1==']') {
      return 1|VISUAL_BRACKET_CLOSE;
    } else if (cp1==')') {
      return 2|VISUAL_BRACKET_CLOSE;
    }
  }

  return 0;
}
