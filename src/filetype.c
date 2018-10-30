// Tippse - File type - Interface and helper functions for specialized document displaying and modification

#include "filetype.h"

#include "config.h"
#include "document_text.h"
#include "encoding.h"
#include "rangetree.h"
#include "trie.h"
#include "visualinfo.h"

extern struct config_cache visual_color_codes[VISUAL_FLAG_COLOR_MAX+1];

// Return entry point to specific file type configuration location
struct trie_node* file_type_config_base(struct file_type* base, const char* suffix) {
  struct trie_node* node = config_find_ascii(base->config, "/filetypes/");
  if (!node) {
    return NULL;
  }

  node = config_advance_ascii(base->config, node, base->file_type);
  if (!node) {
    return NULL;
  }

  node = config_advance_ascii(base->config, node, "/");
  if (!node) {
    return NULL;
  }

  node = config_advance_ascii(base->config, node, suffix);
  if (!node) {
    return NULL;
  }

  node = config_advance_ascii(base->config, node, "/");
  if (!node) {
    return NULL;
  }

  return node;
}

int file_type_keyword_config(struct file_type* base, struct encoding_cache* cache, struct trie_node* parent, int* keyword_length, int nocase) {
  if (!parent) {
    return 0;
  }

  struct trie_node* last = NULL;
  size_t pos = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(cache, pos++).cp;
    if (nocase) {
      cp = tolower(cp);
    }

    parent = trie_find_codepoint(base->config->keywords, parent, cp);

    if (!parent) {
      break;
    }

    if (parent->end) {
      codepoint_t cp = encoding_cache_find_codepoint(cache, pos).cp;
      if ((cp<'a' || cp>'z') && (cp<'A' || cp>'Z') && (cp<'0' || cp>'9') && cp!='_') {
        last = parent;
        *keyword_length = (int)pos;
      }
    }
  }

  if (!last) {
    return 0;
  }

  return (int)config_convert_int64_cache(last, &visual_color_codes[0]);
}

size_t file_type_bracket_match(const struct document_text_render_info* render_info) {
  if ((render_info->visual_detail&(VISUAL_DETAIL_STRING0|VISUAL_DETAIL_STRING1|VISUAL_DETAIL_COMMENT0|VISUAL_DETAIL_COMMENT1))==0) {
    codepoint_t cp = render_info->sequence.cp[0];
    if (cp=='{') {
      return 0|VISUAL_BRACKET_OPEN;
    } else if (cp=='[') {
      return 1|VISUAL_BRACKET_OPEN;
    } else if (cp=='(') {
      return 2|VISUAL_BRACKET_OPEN;
    } else if (cp=='}') {
      return 0|VISUAL_BRACKET_CLOSE;
    } else if (cp==']') {
      return 1|VISUAL_BRACKET_CLOSE;
    } else if (cp==')') {
      return 2|VISUAL_BRACKET_CLOSE;
    }
  }

  return 0;
}

// Return the file type the parser was based on
const char* file_type_file_type(struct file_type* base) {
  return base->file_type;
}