/* Tippse - File type - Interface and helper functions for specialized document displaying and modification */

#include "filetype.h"

int file_type_keyword(struct encoding* encoding, struct encoding_stream stream, struct trie* trie, int* keyword_length) {
  struct trie_node* parent = NULL;
  while (stream.buffer) {
    size_t length = 0;
    int cp = (*encoding->decode)(encoding, &stream, ~0, &length);
    parent = trie_find_codepoint(trie, parent, cp);

    (*keyword_length)++;

    if (!parent) {
      return 0;
    }

    if (parent->type!=0) {
      encoding_stream_forward(&stream, length);
      cp = (*encoding->decode)(encoding, &stream, ~0, &length);
      if ((cp<'a' || cp>'z') && (cp<'A' || cp>'Z') && (cp<'0' || cp>'9') && cp!='_') {
        return parent->type;
      }

      continue;
    }

    encoding_stream_forward(&stream, length);
  }

  return 0;
}

