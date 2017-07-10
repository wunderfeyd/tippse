#include "filetype.h"

int file_type_keyword(struct range_tree_node* buffer, file_offset_t buffer_pos, struct trie* trie, int* keyword_length) {
  struct trie_node* parent = NULL;
  while (buffer) {
    if (buffer_pos>=buffer->length || !buffer->buffer) {
      buffer = range_tree_next(buffer);
      buffer_pos = 0;
      continue;
    }

    const char* text = buffer->buffer->buffer+buffer->offset+buffer_pos;
    file_offset_t max = buffer->length-buffer_pos;

    while (max>0 && (!parent || parent->type==0)) {
      parent = trie_find_codepoint(trie, parent, *text);

      (*keyword_length)++;

      if (!parent) {
        return 0;
      }

      text++;
      max--;

      if (parent->type!=0) {
        break;
      }
    }

    while (max>0 && (parent && parent->type!=0)) {
      if ((*text<'a' || *text>'z') && (*text<'A' || *text>'Z') && (*text<'0' || *text>'9') && *text!='_') {
        return parent->type;
      } else {
        return 0;
      }

      text++;
      max--;
    }

    buffer_pos += max;
  }

  return 0;
}

