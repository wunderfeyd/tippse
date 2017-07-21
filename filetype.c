/* Tippse - File type - Interface and helper functions for specialized document displaying and modification */

#include "filetype.h"

int file_type_keyword(struct encoding_cache* cache, struct trie* trie, int* keyword_length) {
  struct trie_node* parent = NULL;
  size_t pos = 0;
  while (1) {
    int cp = encoding_cache_find_codepoint(cache, pos++);
    parent = trie_find_codepoint(trie, parent, cp);

    (*keyword_length)++;

    if (!parent) {
      return 0;
    }

    if (parent->type!=0) {
      int cp = encoding_cache_find_codepoint(cache, pos);
      if ((cp<'a' || cp>'z') && (cp<'A' || cp>'Z') && (cp<'0' || cp>'9') && cp!='_') {
        return parent->type;
      }

      continue;
    }
  }

  return 0;
}

