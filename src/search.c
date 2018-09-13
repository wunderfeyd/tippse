// Tippse - Search - Compilation of text search algorithms

#include "search.h"

#include "encoding.h"
#include "encoding/utf8.h"
#include "encoding/native.h"
#include "file.h"
#include "filecache.h"
#include "list.h"
#include "misc.h"
#include "rangetree.h"
#include "unicode.h"

extern struct trie* unicode_transform_lower;
extern struct trie* unicode_transform_upper;
extern uint16_t unicode_letters_rle[];
extern uint16_t unicode_whitespace_rle[];
extern uint16_t unicode_digits_rle[];

#define SEARCH_DEBUG 0

// Build node for search tree
struct search_node* search_node_create(int type) {
  struct search_node* base = malloc(sizeof(struct search_node));
  base->type = type;
  base->size = 0;
  base->plain = NULL;
  list_create_inplace(&base->sub, sizeof(struct search_node*));
  list_create_inplace(&base->group_start, sizeof(size_t));
  list_create_inplace(&base->group_end, sizeof(size_t));
  base->stack = NULL;
  base->next = NULL;
  base->forward = NULL;
  base->set = NULL;
  return base;
}

// Remove node
void search_node_destroy(struct search_node* base) {
  search_node_empty(base);
  free(base);
}

// Free all node contents
void search_node_empty(struct search_node* base) {
  while (base->sub.first) {
    list_remove(&base->sub, base->sub.first);
  }
  list_destroy_inplace(&base->sub);

  while (base->group_start.first) {
    list_remove(&base->group_start, base->group_start.first);
  }
  list_destroy_inplace(&base->group_start);

  while (base->group_end.first) {
    list_remove(&base->group_end, base->group_end.first);
  }
  list_destroy_inplace(&base->group_end);

  free(base->plain);
  range_tree_destroy(base->set, NULL);
}

// Move node contents to another and free the source node
void search_node_move(struct search_node* base, struct search_node* copy) {
  search_node_empty(base);
  *base = *copy;
  free(copy);
}

// Append group descriptors from another node
void search_node_group_copy(struct list* to, struct list* from) {
  struct list_node* it = from->first;
  while (it) {
    list_insert(to, NULL, list_object(it));
    it = it->next;
  }
}

// Remove all nodes from the selected node onwards
void search_node_destroy_recursive(struct search_node* node) {
  while (node) {
    struct search_node* next = node->next;

    while (node->sub.first) {
      search_node_destroy_recursive(*(struct search_node**)list_object(node->sub.first));
      list_remove(&node->sub, node->sub.first);
    }

    search_node_destroy(node);

    node = next;
  }
}

// Return count of sub nodes
int search_node_count(struct search_node* node) {
  int count = 0;
  while (node) {
    struct list_node* subs = node->sub.first;
    while (subs) {
      count += search_node_count(*((struct search_node**)list_object(subs)));
      subs = subs->next;
    }

    count++;
    node = node->next;
  }

  return count;
}

// A huge set is needed, create it if necessary (via range_tree)
void search_node_set_build(struct search_node* node) {
  if (!node->set) {
    node->set = range_tree_static(node->set, SEARCH_NODE_SET_CODES, 0);
  }
}

// Append an index to the huge set
void search_node_set(struct search_node* node, size_t index) {
  search_node_set_build(node);
  node->set = range_tree_mark(node->set, index, 1, TIPPSE_INSERTER_MARK);
}

// Decode a huge set from choosen rle stream (usally to create character classes) and invert if needed
void search_node_set_decode_rle(struct search_node* node, int invert, uint16_t* rle) {
  file_offset_t codepoint = 0;
  while (1) {
    file_offset_t codes = (file_offset_t)*rle++;
    if (codes==0) {
      break;
    }

    int set = (int)(codes&1)^invert;
    codes >>= 1;
    if (set) {
      search_node_set_build(node);
      node->set = range_tree_mark(node->set, codepoint, codes, TIPPSE_INSERTER_MARK);
    }
    codepoint += codes;
  }
}

// Destroy the search object
void search_destroy(struct search* base) {
  search_node_destroy_recursive(base->root);
  while (base->stack.first) {
    list_remove(&base->stack, base->stack.first);
  }
  list_destroy_inplace(&base->stack);

  free(base->group_hits);
  free(base);
}

// Create search object
struct search* search_create(int reverse, struct encoding* output_encoding) {
  struct search* base = malloc(sizeof(struct search));
  base->reverse = reverse;
  base->groups = 0;
  base->group_hits = NULL;
  base->root = NULL;
  base->stack_size = 1024;
  list_create_inplace(&base->stack, sizeof(struct search_stack)*base->stack_size);
  list_insert_empty(&base->stack, NULL);
  base->encoding = output_encoding;
  return base;
}

// Create search object from plain text and encoding
struct search* search_create_plain(int ignore_case, int reverse, struct stream needle, struct encoding* needle_encoding, struct encoding* output_encoding) {
  //int64_t tick = tick_count();
  struct search* base = search_create(reverse, output_encoding);

  struct encoding_cache cache;
  encoding_cache_clear(&cache, needle_encoding, &needle);

  struct search_node* last = NULL;
  size_t offset = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(&cache, offset).cp;
    if (cp<=0) {
      break;
    }

    struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BRANCH);
    next->min = 1;
    next->max = 1;
    if (!last) {
      base->root = next;
    } else {
      last->next = next;
    }

    last = next;

    offset += search_append_unicode(last, ignore_case, &cache, offset, last, 0);
  }

  //int64_t tick2 = tick_count();
  if (base->root) {
    search_optimize(base, output_encoding);
  }
  //int64_t tick3 = tick_count();
  //fprintf(stderr, "\r\n%d %d %d\r\n", (int)(tick2-tick), (int)(tick3-tick2), (int)sizeof(struct search_node));
  return base;
}

// Create search object from regular expression string
struct search* search_create_regex(int ignore_case, int reverse, struct stream needle, struct encoding* needle_encoding, struct encoding* output_encoding) {
  struct search* base = search_create(reverse, output_encoding);

  base->root = search_node_create(SEARCH_NODE_TYPE_BRANCH);
  base->root->min = 1;
  base->root->max = 1;

  struct search_node* group[16];
  size_t group_index[16];
  size_t group_depth = 1;
  group[0] = base->root;
  group_index[0] = SIZE_T_MAX;

  struct encoding_cache cache;
  encoding_cache_clear(&cache, needle_encoding, &needle);

  struct search_node* last = search_node_create(SEARCH_NODE_TYPE_BRANCH);
  last->min = 1;
  last->max = 1;
  list_insert(&group[group_depth-1]->sub, NULL, &last);

  size_t offset = 0;
  int escape = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(&cache, offset).cp;
    if (cp<=0) {
      break;
    }

    if (!escape) {
      if (cp=='\\') {
        escape = 1;
        offset++;
        continue;
      }

      if (cp=='{') {
        offset++;
        size_t min = (size_t)decode_based_unsigned_offset(&cache, 10, &offset, SIZE_T_MAX);
        last->min = min;
        last->max = min;
        if (encoding_cache_find_codepoint(&cache, offset).cp==',') {
          offset++;
          size_t max = (size_t)decode_based_unsigned_offset(&cache, 10, &offset, SIZE_T_MAX);
          offset++;
          last->max = (max==0)?SIZE_T_MAX:max;
        } else {
          offset++;
          if (min==0) {
            last->min = 1;
            last->max = 1;
          }
        }

        if (last->min!=last->max) {
          last->type |= SEARCH_NODE_TYPE_MATCHING_TYPE;
        }
        continue;
      }

      if (cp=='|') {
        struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BRANCH);
        next->min = 1;
        next->max = 1;
        list_insert(&group[group_depth-1]->sub, NULL, &next);
        last = next;
        offset++;
        continue;
      }

      if (cp=='(') {
        if (group_depth<16) {
          struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BRANCH);
          next->min = 1;
          next->max = 1;
          last->next = next;
          last = next;
          group[group_depth] = last;
          group_index[group_depth] = base->groups;
          group_depth++;
          list_insert(&last->group_start, NULL, &base->groups);
          list_insert(&last->group_end, NULL, &base->groups);
          base->groups++;
        }

        struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BRANCH);
        next->min = 1;
        next->max = 1;
        list_insert(&group[group_depth-1]->sub, NULL, &next);
        last = next;
        offset++;
        continue;
      }

      if (cp==')') {
        if (group_depth>1) {
          group_depth--;
          last = group[group_depth];
        }
        offset++;
        continue;
      }

      if (cp=='?') {
        if (!(last->type&SEARCH_NODE_TYPE_MATCHING_TYPE)) {
          last->min = 0;
          last->max = 1;
          last->type |= SEARCH_NODE_TYPE_GREEDY|SEARCH_NODE_TYPE_MATCHING_TYPE;
        } else {
          last->type &= ~(SEARCH_NODE_TYPE_GREEDY|SEARCH_NODE_TYPE_MATCHING_TYPE);
        }
        offset++;
        continue;
      }

      if (cp=='*') {
        last->min = 0;
        last->max = SIZE_T_MAX;
        last->type |= SEARCH_NODE_TYPE_GREEDY|SEARCH_NODE_TYPE_MATCHING_TYPE;
        offset++;
        continue;
      }

      if (cp=='+') {
        if (!(last->type&SEARCH_NODE_TYPE_MATCHING_TYPE)) {
          last->min = 1;
          last->max = SIZE_T_MAX;
          last->type |= SEARCH_NODE_TYPE_GREEDY|SEARCH_NODE_TYPE_MATCHING_TYPE;
        } else {
          last->type &= ~(SEARCH_NODE_TYPE_GREEDY|SEARCH_NODE_TYPE_MATCHING_TYPE);
          last->type |= SEARCH_NODE_TYPE_POSSESSIVE;
        }
        offset++;
        continue;
      }

      if (cp=='.') {
        struct search_node* next = search_node_create(SEARCH_NODE_TYPE_SET);
        next->min = 1;
        next->max = 1;
        search_node_set_build(next);
        next->set = range_tree_mark(next->set, 0, SEARCH_NODE_SET_CODES, TIPPSE_INSERTER_MARK);
        last->next = next;
        last = next;
        offset++;
        continue;
      }

      if (cp=='[') {
        struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BRANCH);
        next->min = 1;
        next->max = 1;
        last->next = next;
        last = next;
        offset++;
        offset += search_append_set(last, ignore_case, &cache, offset);
        continue;
      }

      if (cp=='^') {
        struct search_node* next = search_node_create(SEARCH_NODE_TYPE_START_POSITION);
        next->min = 1;
        next->max = 1;
        last->next = next;
        last = next;
        offset++;
        continue;
      }

      if (cp=='$') {
        struct search_node* next = search_node_create(SEARCH_NODE_TYPE_END_POSITION);
        next->min = 1;
        next->max = 1;
        last->next = next;
        last = next;
        offset++;
        continue;
      }
    } else {
      escape = 0;
      struct search_node* next = search_append_class(last, cp, 1);
      if (next) {
        last->next = next;
        last = next;
        offset++;
        continue;
      }

      if (cp=='x' || cp=='u' || cp=='U') {
        offset++;
        size_t index = (size_t)decode_based_unsigned_offset(&cache, 16, &offset, (size_t)((cp=='x')?2:((cp=='u')?4:8)));
        if (index<SEARCH_NODE_SET_CODES) {
          struct search_node* next = search_node_create(SEARCH_NODE_TYPE_SET);
          next->min = 1;
          next->max = 1;
          search_node_set_build(next);
          search_node_set(next, index);
          last->next = next;
          last = next;
        }
        continue;
      }

      if (cp>='1' && cp<='9') {
        size_t group = (size_t)(cp-'1');
        size_t n;
        for (n = 0; n<group_depth; n++) {
          if (group_index[n]==group) {
            break;
          }
        }
        if (n==group_depth && group<base->groups) {
          struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BACKREFERENCE);
          next->min = 1;
          next->max = 1;
          next->group = group;
          last->next = next;
          last = next;
          offset++;
          continue;
        }
      }
    }

    struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BRANCH);
    next->min = 1;
    next->max = 1;
    last->next = next;
    last = next;

    offset += search_append_unicode(last, ignore_case, &cache, offset, last, 0);
  }

  if (base->groups>0) {
    base->group_hits = (struct search_group*)malloc(sizeof(struct search_group)*base->groups);
    for (size_t n = 0; n<base->groups; n++) {
      base->group_hits[n].node_start = NULL;
      base->group_hits[n].node_end = NULL;
    }
  }

  if (base->root) {
    search_optimize(base, output_encoding);
  }
  return base;
}

// Build replacement
struct range_tree_node* search_replacement(struct search* base, struct range_tree_node* replacement_root, struct encoding* replacement_encoding, struct range_tree_node* document_root) {
  if (!replacement_root) {
    return replacement_root;
  }

  struct range_tree_node* output = NULL;

  struct stream replacement_stream;
  stream_from_page(&replacement_stream, range_tree_first(replacement_root), 0);

  struct encoding_cache cache;
  encoding_cache_clear(&cache, replacement_encoding, &replacement_stream);

  uint8_t coded[TREE_BLOCK_LENGTH_MIN+1024];
  size_t size = 0;

  size_t offset = 0;
  int escape = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(&cache, offset).cp;
    if (size>TREE_BLOCK_LENGTH_MIN-8 || cp<=0) {
      output = range_tree_insert_split(output, range_tree_length(output), &coded[0], size, 0);
      size = 0;
    }

    if (cp<=0) {
      break;
    }

    if (!escape) {
      if (cp=='\\') {
        escape = 1;
        offset++;
        continue;
      }
    } else {
      escape = 0;
      if (cp=='r') {
        cp = '\r';
      } else if (cp=='n') {
        cp = '\n';
      } else if (cp=='t') {
        cp = '\t';
      } else if (cp=='b') {
        cp = '\b';
      } else if (cp=='0') {
        cp = '\0';
      } else if (cp>='1' && cp<='9') {
        output = range_tree_insert_split(output, range_tree_length(output), &coded[0], size, 0);
        size = 0;
        size_t group = (size_t)(cp-'1');
        if (group<base->groups) {
          file_offset_t start = stream_offset_page(&base->group_hits[group].start);
          file_offset_t end = stream_offset_page(&base->group_hits[group].end);
          if (start<end) {
            // TODO: this looks inefficient (copy->paste->delete ... use direct copy)
            struct range_tree_node* copy = range_tree_copy(document_root, start, end-start);
            output = range_tree_paste(output, copy, range_tree_length(output), NULL);
            range_tree_destroy(copy, NULL);
          }
        }
        offset++;
        continue;
      }
    }

    size += base->encoding->encode(base->encoding, cp, &coded[size], 8);
    offset++;
  }

  stream_destroy(&replacement_stream);

  return output;
}

// Append/assign a character class to the current selected node
struct search_node* search_append_class(struct search_node* last, codepoint_t cp, int create) {
  if (cp=='r' || cp=='n' || cp=='t' || cp=='b' || cp=='0' || cp=='d' || cp=='D' || cp=='w' || cp=='W' || cp=='s' || cp=='S') {
    struct search_node* next;
    if (create) {
      next = search_node_create(SEARCH_NODE_TYPE_SET);
      next->min = 1;
      next->max = 1;
    } else {
      next = last;
    }

    if (cp=='r') {
      search_node_set(next, (size_t)'\r');
    } else if (cp=='n') {
      search_node_set(next, (size_t)'\n');
    } else if (cp=='t') {
      search_node_set(next, (size_t)'\t');
    } else if (cp=='b') {
      search_node_set(next, (size_t)'\b');
    } else if (cp=='0') {
      search_node_set(next, (size_t)0);
    } else if (cp=='d') {
      search_node_set_decode_rle(next, 0, &unicode_digits_rle[0]);
    } else if (cp=='D') {
      search_node_set_decode_rle(next, 1, &unicode_digits_rle[0]);
    } else if (cp=='w') {
      search_node_set_decode_rle(next, 0, &unicode_letters_rle[0]);
    } else if (cp=='W') {
      search_node_set_decode_rle(next, 1, &unicode_letters_rle[0]);
    } else if (cp=='s') {
      search_node_set_decode_rle(next, 0, &unicode_whitespace_rle[0]);
    } else if (cp=='S') {
      search_node_set_decode_rle(next, 1, &unicode_whitespace_rle[0]);
    }
    return next;
  }

  return NULL;
}

// Interpret regular expression user character classes for exmaple [^abc]
size_t search_append_set(struct search_node* last, int ignore_case, struct encoding_cache* cache, size_t offset) {
  struct search_node* check = search_node_create(SEARCH_NODE_TYPE_SET);
  check->min = 1;
  check->max = 1;
  search_node_set_build(check);
  list_insert(&last->sub, NULL, &check);

  codepoint_t from = 0;
  int escape = 0;
  int invert = 0;
  size_t advance = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(cache, offset+advance).cp;
    if (cp<=0) {
      break;
    }

    if (!escape) {
      if (cp=='\\') {
        escape = 1;
        advance++;
        continue;
      }

      if (cp==']') {
        advance++;
        break;
      }

      if (cp=='^' && advance==0) {
        invert = 1;
        advance++;
        continue;
      }
    } else {
      from = 0;
      escape = 0;

      if (search_append_class(check, cp, 0)) {
        advance++;
        continue;
      }

      if (cp=='x' || cp=='u' || cp=='U') {
        size_t reloc = advance+offset+1;
        size_t index = (size_t)decode_based_unsigned_offset(cache, 16, &reloc, (size_t)((cp=='x')?2:((cp=='u')?4:8)));
        advance = reloc-offset;
        search_node_set(check, index);
        continue;
      }
    }

    if (encoding_cache_find_codepoint(cache, offset+advance+1).cp=='-' && from<=0) {
      from = encoding_cache_find_codepoint(cache, offset+advance).cp;
      advance += 2;
    } else {
      if (from>0) {
        codepoint_t to = encoding_cache_find_codepoint(cache, offset+advance).cp;
        advance++;
        while (from<=to) {
          search_node_set(check, (size_t)from);
          from++;
        }
      } else {
        advance += search_append_unicode(last, ignore_case, cache, offset+advance, check, 0);
      }
    }
  }

  if (ignore_case) {
    // TODO: Ummm... very hacky ... we transform from pure codepoints ... what about multi codepoint transformations?
    // TODO: only check codepoints that are actually transform instead of bruteforce all codepoints (speed improvement)
    struct range_tree_node* source = check->set;
    check->set = range_tree_copy(check->set, 0, check->set->length);
    struct encoding* native_encoding = encoding_native_create();
    struct range_tree_node* range = range_tree_first(source);
    size_t codepoint = 0;
    while (range) {
      if (range->inserter&TIPPSE_INSERTER_MARK) {
        for (size_t n = codepoint; n<codepoint+range->length; n++) {
          codepoint_t from = (codepoint_t)n;
          struct stream native_stream;
          stream_from_plain(&native_stream, (uint8_t*)&from, sizeof(codepoint_t));

          struct encoding_cache native_cache;
          encoding_cache_clear(&native_cache, native_encoding, &native_stream);

          search_append_unicode(last, ignore_case, &native_cache, 0, check, 2);

          stream_destroy(&native_stream);
        }
      }
      codepoint += range->length;
      range = range_tree_next(range);
    }
    native_encoding->destroy(native_encoding);
    range_tree_destroy(source, NULL);
  }

  if (invert) {
    check->set = range_tree_invert_mark(check->set, TIPPSE_INSERTER_MARK);
  }

  return advance;
}

// Try to append unicode character to the current set of the node, if a transformation into multiple characters has been made add a branch
size_t search_append_unicode(struct search_node* last, int ignore_case, struct encoding_cache* cache, size_t offset, struct search_node* shorten, size_t min) {
  size_t advance = 1;
  if (ignore_case) {
    advance = 0;
    size_t length = 0;
    struct unicode_transform_node* transform = unicode_transform(unicode_transform_upper, cache, offset, &advance, &length);
    if (!transform)
      transform = unicode_transform(unicode_transform_lower, cache, offset, &advance, &length);

    if (transform) {
      if (transform->length==1 && shorten && (shorten!=last || advance==1)) {
        shorten->type &= ~SEARCH_NODE_TYPE_BRANCH;
        shorten->type |= SEARCH_NODE_TYPE_SET;
        search_node_set(shorten, (size_t)transform->cp[0]);
      } else {
        search_append_next_codepoint(last, &transform->cp[0], transform->length);
      }
    } else {
      advance = 1;
    }
  }

  if (advance>=min) {
    if (advance==1 && shorten) {
      shorten->type &= ~SEARCH_NODE_TYPE_BRANCH;
      shorten->type |= SEARCH_NODE_TYPE_SET;
      search_node_set(shorten, (size_t)encoding_cache_find_codepoint(cache, offset).cp);
    } else {
      codepoint_t cp[8];
      for (size_t n = 0; n<advance; n++) {
        cp[n] = encoding_cache_find_codepoint(cache, offset+n).cp;
      }
      search_append_next_codepoint(last, &cp[0], advance);
    }
  }
  return advance;
}

// Helper for adding multi character strings. Append set nodes as needed to the current node and eventually create a branch.
struct search_node* search_append_next_index(struct search_node* last, size_t index, int type) {
  if (!(last->type&SEARCH_NODE_TYPE_BRANCH)) {
    if (!last->next || !(last->next->type&SEARCH_NODE_TYPE_BRANCH)) {
      struct search_node* branch = search_node_create(SEARCH_NODE_TYPE_BRANCH);
      branch->min = 1;
      branch->max = 1;
      branch->next = last->next;
      last->next = branch;
    }

    last = last->next;
  }

  struct search_node* check = search_node_create(type);
  check->min = 1;
  check->max = 1;
  search_node_set(check, index);
  list_insert(&last->sub, NULL, &check);
  return check;
}

// Append a multi character string from codepoint array
void search_append_next_codepoint(struct search_node* last, codepoint_t* buffer, size_t size) {
  while (size>0) {
    last = search_append_next_index(last, (size_t)*buffer, SEARCH_NODE_TYPE_SET);
    buffer++;
    size--;
  }
}

// Append a multi character string from byte array
void search_append_next_byte(struct search_node* last, uint8_t* buffer, size_t size) {
  while (size>0) {
    last = search_append_next_index(last, (size_t)*buffer, SEARCH_NODE_TYPE_SET|SEARCH_NODE_TYPE_BYTE);
    buffer++;
    size--;
  }
}

// Build debug output (TODO: get rid of the static helper arrays if printing to the debug console soon)
int search_debug_lengths[128];
int search_debug_stops[128];
void search_debug_indent(size_t depth) {
  for (size_t n = 0; n<depth; n++) {
    if (!search_debug_stops[n]) {
      fprintf(stderr, "|");
    } else {
      fprintf(stderr, " ");
    }

    for (int l = 0; l<search_debug_lengths[n]-1; l++) {
      fprintf(stderr, " ");
    }
  }
}

void search_debug_tree(struct search* base, struct search_node* node, size_t depth, int length, int stop) {
  if (depth>0) {
    search_debug_lengths[depth-1] = length;
    search_debug_stops[depth-1] = stop;
  }
  length = 0;
  if (node->group_start.first) {
    fprintf(stderr, "{");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_GROUP_START) {
    fprintf(stderr, "(");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_BRANCH) {
    fprintf(stderr, "B");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_GREEDY) {
    fprintf(stderr, "*");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_POSSESSIVE) {
    fprintf(stderr, "+");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_MATCHING_TYPE) {
    fprintf(stderr, "M");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_BYTE) {
    fprintf(stderr, "8");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_BACKREFERENCE) {
    fprintf(stderr, "R");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_START_POSITION) {
    fprintf(stderr, "<");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_END_POSITION) {
    fprintf(stderr, ">");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_GROUP_END) {
    fprintf(stderr, ")");
    length++;
  }

  if (node->group_end.first) {
    fprintf(stderr, "}");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_SET) {
    if (node->size==0) {
      fprintf(stderr, "[");
      int count = 0;
      struct range_tree_node* range = range_tree_first(node->set);
      size_t codepoint = 0;
      while (range && count<8) {
        if (range->inserter&TIPPSE_INSERTER_MARK) {
          for (size_t n = 0; n<range->length && count<8; n++) {
            count++;
            size_t c = n+codepoint;
            if (c<0x20 || c>0x7f) {
              if (node->type&SEARCH_NODE_TYPE_BYTE) {
                fprintf(stderr, "\\%02x", (int)c);
                length+=3;
              } else {
                fprintf(stderr, "\\%06x", (int)c);
                length+=7;
              }
            } else {
              fprintf(stderr, "%c", (int)c);
              length++;
            }
          }
        }
        codepoint += range->length;
        range = range_tree_next(range);
      }

      if (count==8) {
        fprintf(stderr, "..");
        length+=2;
      }
      fprintf(stderr, "]");
      length+=2;
    }

    if (node->size>0) {
      fprintf(stderr, "\"");
      for (size_t n = 0; n<node->size; n++) {
        uint8_t c = node->plain[n];
        if (c<0x20 || c>0x7f) {
          if (node->type&SEARCH_NODE_TYPE_BYTE) {
            fprintf(stderr, "\\%02x", c);
            length+=3;
          } else {
            fprintf(stderr, "\\%06x", c);
            length+=7;
          }
        } else {
          fprintf(stderr, "%c", c);
          length++;
        }
      }
      fprintf(stderr, "\"");
      length+=2;
    }
  }

  if (node->min!=1 || node->max!=1) {
    char out[1024];
    length += sprintf(&out[0], "{%d,%d}", (int)node->min, (int)node->max);
    fprintf(stderr, "%s", &out[0]);
  }

  if (node->next) {
    fprintf(stderr, " ");
    length+=1;
    search_debug_tree(base, node->next, depth+1, length, node->sub.first?0:1);
  } else {
    fprintf(stderr, "\r\n");
  }

  struct list_node* subs = node->sub.first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    search_debug_indent(depth);
    fprintf(stderr, "`- ");
    subs = subs->next;
    search_debug_tree(base, check, depth+1, 3, subs?0:1);
  }
}

// After creation of the search the tree is not optimized. Reduce branches, merge nodes or change set sizes. Afterwards prepare the tree for usage from within the search loop.
void search_optimize(struct search* base, struct encoding* encoding) {
  int again;
  do {
    if (SEARCH_DEBUG) {
      fprintf(stderr, "loop codepoints.. %d\r\n", search_node_count(base->root));
      search_debug_tree(base, base->root, 0, 0, 0);
    }
    again = search_optimize_combine_branch(base->root);
    again |= search_optimize_reduce_branch(base->root);
  } while(again);
  search_optimize_native(encoding, base->root);
  do {
    if (SEARCH_DEBUG) {
      fprintf(stderr, "loop native.. %d\r\n", search_node_count(base->root));
      search_debug_tree(base, base->root, 0, 0, 0);
    }
    again = search_optimize_combine_branch(base->root);
    again |= search_optimize_reduce_branch(base->root);
  } while(again);
  search_optimize_plain(base->root);
  search_prepare(base, base->root, NULL);
  search_prepare_skip(base, base->root);
}

// Remove empty branches or merge branch nodes into the current segment.
int search_optimize_reduce_branch(struct search_node* node) {
  int again = 0;
  if (node->sub.count==1) {
    struct search_node* check = *((struct search_node**)list_object(node->sub.first));
    struct search_node* crawl = check;
    struct search_node* last = check;
    size_t count = 0;
    while (crawl) {
      last = crawl;
      crawl = crawl->next;
      count++;
    }

    if ((node->min==1 && node->max==1) || (count==1 && ((last->min==last->max) || (node->min==node->max)))) {
      last->next = node->next;
      if (count==1) {
        last->min *= node->min;
        last->max *= node->max;
      }
      search_node_group_copy(&check->group_start, &node->group_start);
      search_node_group_copy(&last->group_end, &node->group_end);
      check->type |= node->type&~SEARCH_NODE_TYPE_BRANCH;
      search_node_move(node, check);
      again = 1;
    }
  }

  if (node->sub.count==0 && (node->type&SEARCH_NODE_TYPE_BRANCH) && node->next) {
    search_node_group_copy(&node->next->group_start, &node->group_start);
    search_node_group_copy(&node->next->group_end, &node->group_end);
    search_node_move(node, node->next);
  }

  if (node->next) {
    again |= search_optimize_reduce_branch(node->next);
  }

  struct list_node* subs = node->sub.first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    again |= search_optimize_reduce_branch(check);
    subs = subs->next;
  }

  return again;
}

// Merge sub segments of a branch. Eventually merge sets or move branch behind equal segment starts.
int search_optimize_combine_branch(struct search_node* node) {
  int again = 0;
  struct list_node* subs1 = node->sub.first;
  while (subs1) {
    struct search_node* check1 = *((struct search_node**)list_object(subs1));
    struct list_node* subs2 = subs1->next;
    while (subs2) {
      struct search_node* check2 = *((struct search_node**)list_object(subs2));
      if (check1->type==check2->type && check1->min==check2->min && check1->max==1 && check2->max==1 && (check1->type&SEARCH_NODE_TYPE_SET)) {
        if (check1->next==NULL && check2->next==NULL) { // Merge only if both paths are equal (or if there are single nodes)
          struct range_tree_node* range = range_tree_first(check2->set);
          size_t codepoint = 0;
          while (range) {
            if (range->inserter&TIPPSE_INSERTER_MARK) {
              search_node_set_build(check1);
              check1->set = range_tree_mark(check1->set, codepoint, range->length, TIPPSE_INSERTER_MARK);
            }
            codepoint += range->length;
            range = range_tree_next(range);
          }
          search_node_group_copy(&check1->group_start, &check2->group_start);
          search_node_group_copy(&check1->group_end, &check2->group_end);
          search_node_destroy_recursive(check2);
          list_remove(&node->sub, subs2);
          again = 1;
          break;
        }

        struct range_tree_node* range1 = range_tree_first(check1->set);
        struct range_tree_node* range2 = range_tree_first(check2->set);
        while (range1 && range2) {
          if (range1->inserter!=range2->inserter || range1->length!=range2->length) {
            break;
          }
          range1 = range_tree_next(range1);
          range2 = range_tree_next(range2);
        }

        if (!range1 && !range2) { // Merge if both nodes are equal
          int next1 = (check1->next && (!(check1->next->type&SEARCH_NODE_TYPE_BRANCH) || (check1->next->sub.count!=0)))?1:0;
          int next2 = (check2->next && (!(check2->next->type&SEARCH_NODE_TYPE_BRANCH) || (check2->next->sub.count!=0)))?1:0;
          if ((next1 && next2) || (!next1 && !next2)) {
            if (!check1->next || !(check1->next->type&SEARCH_NODE_TYPE_BRANCH) || check1->next->min!=1 || check1->next->max!=1) {
              struct search_node* branch = search_node_create(SEARCH_NODE_TYPE_BRANCH);
              branch->min = 1;
              branch->max = 1;
              branch->next = NULL;
              if (check1->next) {
                list_insert(&branch->sub, NULL, &check1->next);
              }
              check1->next = branch;
            }

            if (check2->next) {
              if ((check2->next->type&SEARCH_NODE_TYPE_BRANCH) && check2->next->min==1 && check2->next->max==1) {
                struct list_node* subs = check2->next->sub.first;
                while (subs) {
                  list_insert(&check1->next->sub, NULL, list_object(subs));
                  subs = subs->next;
                }

                if (check2->next->next) {
                  list_insert(&check1->next->sub, NULL, &check2->next->next);
                }
                search_node_group_copy(&check1->next->group_start, &check2->next->group_start);
                search_node_group_copy(&check1->next->group_end, &check2->next->group_end);
                search_node_destroy(check2->next);
              } else {
                list_insert(&check1->next->sub, NULL, &check2->next);
              }
            }
            search_node_group_copy(&check1->next->group_start, &check2->group_start);
            search_node_group_copy(&check1->next->group_end, &check2->group_end);
            search_node_destroy(check2);
            list_remove(&node->sub, subs2);
            again = 1;
            break;
          }
        }
      }
      subs2 = subs2->next;
    }
    subs1 = subs1->next;
  }

  if (node->next) {
    again |= search_optimize_combine_branch(node->next);
  }

  struct list_node* subs = node->sub.first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    again |= search_optimize_combine_branch(check);
    subs = subs->next;
  }

  return again;
}

// If the set is small try to translate the huge unicode set into a small byte set and encode the unicode code point into its output reprensentation
void search_optimize_native(struct encoding* encoding, struct search_node* node) {
  if (node->next) {
    search_optimize_native(encoding, node->next);
  }

  struct list_node* subs = node->sub.first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    search_optimize_native(encoding, check);
    subs = subs->next;
  }

  size_t count = 0;
  struct range_tree_node* range = range_tree_first(node->set);
  while (range && count<=SEARCH_NODE_TYPE_NATIVE_COUNT) {
    if (range->inserter&TIPPSE_INSERTER_MARK) {
      count += range->length;
    }
    range = range_tree_next(range);
  }

  if ((node->type&SEARCH_NODE_TYPE_SET) && !(node->type&SEARCH_NODE_TYPE_BYTE) && count<=SEARCH_NODE_TYPE_NATIVE_COUNT) {
    node->type &= ~SEARCH_NODE_TYPE_SET;
    node->type |= SEARCH_NODE_TYPE_BRANCH;

    size_t codepoint = 0;
    struct range_tree_node* range = range_tree_first(node->set);
    while (range) {
      if (range->inserter&TIPPSE_INSERTER_MARK) {
        for (size_t n = 0; n<range->length; n++) {
          uint8_t coded[1024];
          size_t size = encoding->encode(encoding, (codepoint_t)(codepoint+n), &coded[0], 8);
          if (size>0) {
            search_append_next_byte(node, &coded[0], size);
          }
        }
      }
      codepoint += range->length;
      range = range_tree_next(range);
    }
  }
}

// Sets with a single byte aren't really useful, convert them to a plain byte string. Stick together multiple plain byte strings if nodes are neighbours.
void search_optimize_plain(struct search_node* node) {
  if ((node->type&SEARCH_NODE_TYPE_BYTE) && node->size==0 && node->sub.count==0 && node->min==node->max && node->max<16) {
    size_t codepoint = 0;
    size_t set = SIZE_T_MAX;
    struct range_tree_node* range = range_tree_first(node->set);
    while (range) {
      if (range->inserter&TIPPSE_INSERTER_MARK) {
        if (range->length!=1) {
          set = SIZE_T_MAX;
          break;
        }
        if (set!=SIZE_T_MAX) {
          set = SIZE_T_MAX;
          break;
        }
        set = codepoint;
      }
      codepoint += range->length;
      range = range_tree_next(range);
    }
    if (set!=SIZE_T_MAX) {
      node->size = node->max;
      node->plain = malloc(sizeof(uint8_t)*node->size);
      node->min = 1;
      node->max = 1;
      for (size_t n = 0; n<node->size; n++) {
        node->plain[n] = (uint8_t)set;
      }
    }
  }

  if (node->next) {
    search_optimize_plain(node->next);
  }

  struct list_node* subs = node->sub.first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    search_optimize_plain(check);
    subs = subs->next;
  }

  if (node->next && node->size>0 && node->next->size>0 && !node->next->group_start.first && !node->group_end.first) {
    size_t size = node->size+node->next->size;
    uint8_t* plain = malloc(sizeof(uint8_t)*size);
    memcpy(plain, node->plain, node->size);
    memcpy(plain+node->size, node->next->plain, node->next->size);
    free(node->plain);
    node->plain = plain;
    node->size = size;
    search_node_group_copy(&node->group_end, &node->next->group_end);
    struct search_node* remove = node->next;
    node->next = node->next->next;
    search_node_destroy(remove);
  }
}

// Update small sets, group informations and node followers if the node had a hit (back to the previous branch)
void search_prepare(struct search* base, struct search_node* node, struct search_node* prev) {
  if (node->type&SEARCH_NODE_TYPE_BYTE) {
    memset(&node->bitset, 0, sizeof(node->bitset));
    size_t codepoint = 0;
    struct range_tree_node* range = range_tree_first(node->set);
    while (range) {
      if (range->inserter&TIPPSE_INSERTER_MARK) {
        for (size_t n = codepoint; n<codepoint+range->length; n++) {
          node->bitset[n/SEARCH_NODE_SET_BUCKET] |= ((uint32_t)1)<<n%SEARCH_NODE_SET_BUCKET;
        }
      }
      codepoint += range->length;
      range = range_tree_next(range);
    }
  }

  if (node->group_start.first) {
    node->type |= SEARCH_NODE_TYPE_GROUP_START;
    struct list_node* groups = node->group_start.first;
    while (groups) {
      base->group_hits[*(size_t*)list_object(groups)].node_start = node;
      groups = groups->next;
    }
  }
  if (node->group_end.first) {
    node->type |= SEARCH_NODE_TYPE_GROUP_END;
    struct list_node* groups = node->group_end.first;
    while (groups) {
      base->group_hits[*(size_t*)list_object(groups)].node_end = node;
      groups = groups->next;
    }
  }

  if (node->next) {
    node->forward = node->next;
    search_prepare(base, node->next, prev);
  } else {
    node->forward = prev;
  }

  struct list_node* subs = node->sub.first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    search_prepare(base, check, node);
    subs = subs->next;
  }
}

// Build a skip tree (the search starts at the needle end, if no character match is found skip at the whole needle length otherwise skip to the possible needle end match)
void search_prepare_skip(struct search* base, struct search_node* node) {
  struct search_skip_node references[SEARCH_SKIP_NODES];
  size_t nodes = 0;
  while (node && nodes<SEARCH_SKIP_NODES) {
    if (!(node->type&SEARCH_NODE_TYPE_SET) || !(node->type&SEARCH_NODE_TYPE_BYTE) || node->min!=node->max) {
      break;
    }

    if (node->plain) {
      for (size_t n = 0; n<node->size && nodes<SEARCH_SKIP_NODES; n++) {
        for (size_t index = 0; index<256; index++) {
          references[nodes].index[index] = (size_t)((node->plain[n]==(uint8_t)index)?1:0);
        }
        nodes++;
      }
    } else {
      for (size_t n = 0; n<node->min && nodes<SEARCH_SKIP_NODES; n++) {
        for (size_t index = 0; index<256; index++) {
          references[nodes].index[index] = 0;
        }
        size_t codepoint = 0;
        struct range_tree_node* range = range_tree_first(node->set);
        while (range) {
          if (range->inserter&TIPPSE_INSERTER_MARK) {
            for (size_t index = codepoint; index<codepoint+range->length; index++) {
              references[nodes].index[index] = 1;
            }
          }
          codepoint += range->length;
          range = range_tree_next(range);
        }

        nodes++;
      }
    }

    node = node->next;
  }

  base->skip_length = 0;
  if (nodes>0 && !base->reverse) {
    base->skip_rescan = (node || base->groups>0)?1:0;
    base->skip_length = nodes;
    for (size_t pos = 0; pos<nodes; pos++) {
      size_t current = nodes-pos-1;
      for (size_t index = 0; index<256; index++) {
        size_t skip_min = nodes+1;
        size_t skip_rel = 0;
        size_t last = SIZE_T_MAX;
        for (size_t check = 0; check<current; check++) {
          if (references[check].index[index]) {
            size_t skip = (nodes-check);
            if (skip<skip_min) {
              skip_min = skip;
              skip_rel = 0;
            }
          }
        }
        for (size_t check = 0; check<current; check++) {
          if (references[check].index[index]) {
            if (last!=SIZE_T_MAX) {
              size_t skip = check-last+1;
              if (skip<skip_min) {
                skip_min = skip;
                skip_rel = pos;
              }
            }
            last = check;
          }
        }
        base->skip[pos].index[index] = references[current].index[index]?0:(skip_min+skip_rel);
      }
    }
  }
  //fprintf(stderr, "Nodes: %d\r\n", (int)nodes);
}

// Find next occurence of the compiled pattern until the stream ends or "left" has been count down
int search_find(struct search* base, struct stream* text, file_offset_t* left) {
  if (!base->root) {
    if (left) {
      *left = 0;
    }
    return 0;
  }

  for (size_t n = 0; n<base->groups; n++) {
    base->group_hits[n].start = *text;
    base->group_hits[n].end = *text;
    base->group_hits[n].node_start->start = *text;
    base->group_hits[n].node_end->end = *text;
  }

  file_offset_t count = left?*left:FILE_OFFSET_T_MAX;

  if (base->skip_length>0) {
    stream_forward(text, base->skip_length);
    size_t hit = 0;
    if (count<FILE_OFFSET_T_MAX-base->skip_length) {
      count += base->skip_length;
    }

    while (text->displacement<=text->cache_length || !stream_end(text)) {
      size_t size = 0;
      while (1) {
        uint8_t index = stream_read_reverse(text);
        // fprintf(stderr, "%02x '%c' %d/%d @ %d\r\n", index, index, (int)size, (int)base->skip[size].index[index], (int)stream_offset(text));
        hit = base->skip[size++].index[index];
        if (hit) {
          break;
        }

        if (size==base->skip_length) {
          if (!base->skip_rescan || search_find_loop(base, base->root, text)) {
            base->hit_start = *text;
            if (!base->skip_rescan) {
              stream_forward(text, base->skip_length);
              base->hit_end = *text;
            }
            if (left) {
              if (count<base->skip_length) {
                stream_reverse(text, base->skip_length-count);
                *left = 0;
                return 0;
              } else {
                *left = count-base->skip_length;
              }
            }
            return 1;
          }
          hit = size+1;
          break;
        }
      }
      if (count<hit-size) {
        count = 0;
        break;
      }
      count-=hit-size;
      stream_forward(text, hit);
    }

    if (count<base->skip_length) {
      stream_reverse(text, base->skip_length-count);
      count = base->skip_length;
    }

    count -= base->skip_length;
  } else {
    if (!base->reverse) {
      while (!stream_end(text) && count>0) {
        count--;
        if (search_find_loop(base, base->root, text)) {
          base->hit_start = *text;
          stream_forward(text, 1);
          return 1;
        }

        stream_forward(text, 1);
      }
    } else {
      while (count>0) {
        count--;
        if (search_find_loop(base, base->root, text)) {
          base->hit_start = *text;
          stream_reverse(text, 1);
          return 1;
        }

        if (stream_start(text)) {
          break;
        }
        stream_reverse(text, 1);
      }
    }
  }

  if (left) {
    *left = count;
  }

  return 0;
}

// Check a single location
int search_find_check(struct search* base, struct stream* text) {
  for (size_t n = 0; n<base->groups; n++) {
    base->group_hits[n].start = *text;
    base->group_hits[n].end = *text;
    base->group_hits[n].node_start->start = *text;
    base->group_hits[n].node_end->end = *text;
  }

  if (search_find_loop(base, base->root, text)) {
    base->hit_start = *text;
    return 1;
  }

  return 0;
}

// Loop helper to find the bit in the byte set accordingly to the index
TIPPSE_INLINE int search_node_bitset_check(struct search_node* node, uint8_t index) {
  return (int)((node->bitset[index/SEARCH_NODE_SET_BUCKET]>>(index%SEARCH_NODE_SET_BUCKET))&1);
}

// Loop helper to find the code point in the code point set accordingly to the index
TIPPSE_INLINE int search_node_set_check(struct search_node* node, codepoint_t index) {
  return index>=0 && range_tree_marked(node->set, (file_offset_t)index, 1, TIPPSE_INSERTER_MARK);
}

// Next stack entry, create new stack and frame if needed
TIPPSE_INLINE void search_find_loop_enter(struct search* base, struct search_stack** load, struct search_stack** start, struct search_stack** end, struct list_node** frame) {
  *load = (*load)+1;
  if (*load!=*end) {
    return;
  }

  if (!(*frame)->next) {
    list_insert_empty(&base->stack, *frame);
  }

  *frame = (*frame)->next;
  *start = (struct search_stack*)list_object(*frame);
  *end = (*start)+base->stack_size;
  *load = *start;
}

// Previous stack entry
TIPPSE_INLINE void search_find_loop_leave(struct search* base, struct search_stack** load, struct search_stack** start, struct search_stack** end, struct list_node** frame) {
  if (*load!=*start) {
    *load = (*load)-1;
    return;
  }

  *frame = (*frame)->prev;
  *start = (struct search_stack*)list_object(*frame);
  *end = (*start)+base->stack_size;
  *load = (*end)-1;
}

// The hot loop of the search, matching and branching is done here. Non recursive version, but uses recursive style with a simulated stack. This version has replaced a recursive version to halt and continue the search process (in future)
int search_find_loop(struct search* base, struct search_node* node, struct stream* text) {
  struct stream stream;
  stream_clone(&stream, text);
  struct list_node* frame = base->stack.first;
  struct search_stack* start = (struct search_stack*)list_object(frame);
  struct search_stack* end = start+base->stack_size;
  struct search_stack* load = start;

  file_offset_t longest = 0;
  load->node = NULL;
  int enter = 1;
  int hit = 0;
  int hits = 0;
  while (1) {
    if (enter==1 && node->type&SEARCH_NODE_TYPE_GROUP_START) {
      node->start = stream;
    }

    if (node->type&SEARCH_NODE_TYPE_SET) {
      if (node->plain) {
        enter = 1;
        for (size_t n = 0; n<node->size; n++) {
          if (stream_end(&stream) || node->plain[n]!=stream_read_forward(&stream)) {
            enter = 0;
            break;
          }
        }
      } else {
        if (enter) {
          if (!(node->type&SEARCH_NODE_TYPE_POSSESSIVE)) {
            if ((node->type&SEARCH_NODE_TYPE_BYTE)) {
              for (size_t n = 0; n<node->min; n++) {
                if (stream_end(&stream) || !search_node_bitset_check(node, stream_read_forward(&stream))) {
                  enter = 0;
                  break;
                }
              }
            } else {
              for (size_t n = 0; n<node->min; n++) {
                size_t length;
                if (stream_end(&stream) || !search_node_set_check(node, base->encoding->decode(base->encoding, &stream, &length))) {
                  enter = 0;
                  break;
                }
              }
            }

            if (enter && node->min<node->max) {
              search_find_loop_enter(base, &load, &start, &end, &frame);
              load->node = node;
              load->previous = node->stack;
              load->loop = node->min;
              load->hits = hits;
              node->stack = load;
              stream_clone(&load->stream, &stream);
            }
          } else {
            if ((node->type&SEARCH_NODE_TYPE_BYTE)) {
              for (size_t n = 0; n<node->max; n++) {
                if (stream_end(&stream) || !search_node_bitset_check(node, stream_read_forward(&stream))) {
                  stream_reverse(&stream, 1);
                  enter = (n>=node->min)?1:0;
                  break;
                }
              }
            } else {
              for (size_t n = 0; n<node->max; n++) {
                size_t length = 0;
                if (stream_end(&stream) || !search_node_set_check(node, base->encoding->decode(base->encoding, &stream, &length))) {
                  stream_reverse(&stream, length);
                  enter = (n>=node->min)?1:0;
                  break;
                }
              }
            }
          }
        } else {
          struct search_stack* index = node->stack;
          enter = (index->loop<=node->max && ((node->type&SEARCH_NODE_TYPE_GREEDY) || load->hits==hits))?1:0;
          if (index->loop<node->max) {
            if ((node->type&SEARCH_NODE_TYPE_BYTE)) {
              if (stream_end(&load->stream) || !search_node_bitset_check(node, stream_read_forward(&load->stream))) {
                enter = 0;
              }
            } else {
              size_t length;
              if (stream_end(&load->stream) || !search_node_set_check(node, base->encoding->decode(base->encoding, &load->stream, &length))) {
                enter = 0;
              }
            }
          }

          if (!enter) {
            node->stack = load->previous;
            stream_destroy(&load->stream);
            search_find_loop_leave(base, &load, &start, &end, &frame);
          } else {
            index->loop++;
            stream_destroy(&stream);
            stream_clone(&stream, &load->stream);
          }
        }
      }
    } else {
      if (node->type&SEARCH_NODE_TYPE_BRANCH) {
        if (enter) {
          search_find_loop_enter(base, &load, &start, &end, &frame);
          load->node = node;
          load->previous = node->stack;
          load->loop = (enter==2)?node->stack->loop+1:0;
          load->sub = node->sub.first;
          stream_clone(&load->stream, &stream);
          load->hits = hits;
          node->stack = load;
          enter = 1;
        } else {
          stream_destroy(&stream);
          stream_clone(&stream, &load->stream);
        }

        struct search_stack* index = node->stack;
        if (index->loop<node->max) {
          if (enter==0 || index->loop<node->min || (node->type&SEARCH_NODE_TYPE_POSSESSIVE)) {
            if (index->sub) {
              if ((node->type&(SEARCH_NODE_TYPE_GREEDY|SEARCH_NODE_TYPE_POSSESSIVE)) || index->loop<node->min || load->sub!=node->sub.first || load->hits==hits) {
                enter = 1;
                struct search_node* next = *((struct search_node**)list_object(index->sub));
                index->sub = index->sub->next;
                node = next;
                continue;
              }
            } else {
              if (node->type&SEARCH_NODE_TYPE_POSSESSIVE) {
                node->stack = load->previous;
                stream_destroy(&load->stream);
                search_find_loop_leave(base, &load, &start, &end, &frame);
                enter = 1;
              }
            }
          }
        }
        if (!enter) {
          node->stack = load->previous;
          stream_destroy(&load->stream);
          search_find_loop_leave(base, &load, &start, &end, &frame);
        }
      } else {
        if (node->type&SEARCH_NODE_TYPE_START_POSITION) {
          enter = stream_start(&stream);
          if (enter==0) { // TODO: we need to know the line ending style here in future
            uint8_t index = stream_read_reverse(&stream);
            stream_forward(&stream, 1);
            enter = (index=='\r' || index=='\n')?1:0;
          }
        } else if (node->type&SEARCH_NODE_TYPE_END_POSITION) {
          enter = stream_end(&stream);
          if (enter==0) { // TODO: we need to know the line ending style here in future
            uint8_t index = stream_read_forward(&stream);
            stream_reverse(&stream, 1);
            enter = (index=='\r' || index=='\n')?1:0;
          }
        } else if (node->type&SEARCH_NODE_TYPE_BACKREFERENCE) {
          // TODO: are min/max or modifiers needed here?
          struct stream source;
          stream_clone(&source, &base->group_hits[node->group].node_start->start);
          file_offset_t from = stream_offset(&source);
          file_offset_t to = stream_offset(&base->group_hits[node->group].node_start->end);
          enter = 1;
          while (from<to) {
            if (stream_read_forward(&stream)!=stream_read_forward(&source)) {
              enter = 0;
              break;
            }
            from++;
          }
          stream_destroy(&source);
        }
      }
    }

    if (enter) {
      if (node->type&SEARCH_NODE_TYPE_GROUP_END) {
        node->end = stream;
      }
      if (!node->next) {
        enter = 2;
      }
      node = node->forward;
      if (node) {
        continue;
      }

      file_offset_t distance = stream_offset(&stream)-stream_offset(text);
      if (distance>longest || hit==0) {
        longest = distance;
        for (size_t n = 0; n<base->groups; n++) {
          base->group_hits[n].start = base->group_hits[n].node_start->start;
          base->group_hits[n].end = base->group_hits[n].node_end->end;
        }
        base->hit_end = stream;
        hit = 1;
      }
      hits++;
      enter = 0;
    }

    node = load->node;
    if (!node) {
      break;
    }
  }

  stream_destroy(&stream);
  return hit;
}

// Some small manual unittests (TODO: remove or expand as soon the search is feature complete and optimized)
void search_test(void) {
  //const char* needle_text = "[^\n]*";
  //const char* needle_text = "remove_ticket_number";
  const char* needle_text = "sIBM";
  //const char* needle_text = "Recently, Standard Japanese has ";
  //const char* needle_text = "Hallo äÖÜ ß Test Ú ń ǹ";
  //const char* needle_text = "(haLlO|(Teßt){1,2}?)";
  //const char* needle_text = "(\\S+)\\s(sch)*(schnappi|schnapp|schlappi).*?[A-C][^\\d]{2}";
  //const char* needle_text = "a|ab|abc";
  //const char* needle_text = "#(\\S+)";
  //const char* needle_text = "(schnappi|schnapp|schlappi){2}";
  //const char* needle_text = "make_";
  //const char* needle_text = "schnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappi";
  struct stream needle_stream;
  stream_from_plain(&needle_stream, (uint8_t*)needle_text, strlen(needle_text));
  struct encoding* needle_encoding = encoding_utf8_create();
  struct encoding* output_encoding = encoding_utf8_create();
  struct search* search = search_create_regex(1, 0, needle_stream, needle_encoding, output_encoding);
  search_debug_tree(search, search->root, 0, 0, 0);

  struct stream text;
  const char* text_text = "Dies ist ein l TesstTeßt, HaLLo schschschSchlappiSchlappi ppi! 999 make_debug.sh abc #include bla"; //"void et_container::remove_ticket_number(";
  stream_from_plain(&text, (uint8_t*)text_text, strlen(text_text));
  int found = search_find(search, &text, NULL);
  fprintf(stderr, "%d\r\n", found);
  if (found) {
    fprintf(stderr, "\"");
    for (const uint8_t* plain = search->hit_start.plain+search->hit_start.displacement; plain!=search->hit_end.plain+search->hit_end.displacement; plain++) {
      fprintf(stderr, "%c", *plain);
    }
    fprintf(stderr, "\"\r\n");
    for (size_t n = 0; n<search->groups; n++) {
      fprintf(stderr, "%d: \"", (int)n);
      for (const uint8_t* plain = search->group_hits[n].start.plain+search->group_hits[n].start.displacement; plain!=search->group_hits[n].end.plain+search->group_hits[n].end.displacement; plain++) {
        fprintf(stderr, "%c", *plain);
      }
      fprintf(stderr, "\"\r\n");
    }
  }
  search_destroy(search);

  /*for (size_t n = 0; n<100; n++) {
    struct search* search = search_create_plain(1, 0, needle_stream, needle_encoding, output_encoding);
    search_destroy(search);
  }*/

  struct file* f = file_create("enwik8", TIPPSE_FILE_READ);
  if (f) {
    size_t length = 100*1000*1000;
    uint8_t* buffer = (uint8_t*)malloc(length);
    UNUSED(file_read(f, buffer, length));
    file_destroy(f);

    fprintf(stderr, "hash search...\r\n");
    struct stream buffered;
    struct search* search = search_create_plain(1, 0, needle_stream, needle_encoding, output_encoding);
    search_debug_tree(search, search->root, 0, 0, 0);
    int64_t tick = tick_count();
    for (size_t n = 0; n<5; n++) {
      stream_from_plain(&buffered, buffer, length);
      int found = search_find(search, &buffered, NULL);
      fprintf(stderr, "%d %d us %d\r\n", (int)n, (int)((tick_count()-tick)/(int64_t)(n+1)), found);
      if (found) {
        fprintf(stderr, "%d \"", (int)stream_offset_plain(&search->hit_start));
        for (const uint8_t* plain = search->hit_start.plain+search->hit_start.displacement; plain!=search->hit_end.plain+search->hit_end.displacement; plain++) {
          fprintf(stderr, "%c", *plain);
        }
        fprintf(stderr, "\"\r\n");
      }
      fflush(stdout);
    }
    search_destroy(search);
    free(buffer);
  }

  {
    struct file_cache* file = file_cache_create("enwik8");
    fprintf(stderr, "hash search...\r\n");
    struct stream buffered;
    struct search* search = search_create_plain(1, 0, needle_stream, needle_encoding, output_encoding);
    search_debug_tree(search, search->root, 0, 0, 0);
    int64_t tick = tick_count();
    for (size_t n = 0; n<5; n++) {
      stream_from_file(&buffered, file, 0);
      int found = search_find(search, &buffered, NULL);
      fprintf(stderr, "%d %d us %d\r\n", (int)n, (int)((tick_count()-tick)/(int64_t)(n+1)), found);
      if (found) {
        fprintf(stderr, "%d \"", (int)stream_offset_file(&search->hit_start));
        for (const uint8_t* plain = search->hit_start.plain+search->hit_start.displacement; plain!=search->hit_end.plain+search->hit_end.displacement; plain++) {
          fprintf(stderr, "%c", *plain);
        }
        fprintf(stderr, "\"\r\n");
      }
      fflush(stdout);
    }
    search_destroy(search);
  }

  for (size_t fuzz = 0; fuzz<1000000; fuzz++) {
    uint8_t random[8192];
    size_t length = (size_t)(rand()%32);
    for (size_t n = 0; n<length; n++) {
      random[n] = rand()&0x7f;
    }

    uint8_t output[8192];
    size_t raw = (size_t)(rand()%8192);
    for (size_t n = 0; n<raw; n++) {
      output[n] = rand()&0xff;
    }

    struct stream needle_stream;
    stream_from_plain(&needle_stream, &random[0], length);

    struct stream output_stream;
    stream_from_plain(&output_stream, &output[0], raw);

    struct search* search = search_create_regex(1, 0, needle_stream, needle_encoding, output_encoding);
    if ((fuzz&0)==0) {
      fprintf(stderr, ">> %d || ", (int)fuzz);
      for (size_t n = 0; n<length; n++) {
        if (random[n]>=32 && random[n]<127) {
          fprintf(stderr, "%c", random[n]);
        } else {
          fprintf(stderr, ".");
        }
      }
      fprintf(stderr, "\r\n");
      search_debug_tree(search, search->root, 0, 0, 0);
    }
    int found = search_find(search, &output_stream, NULL);
    fprintf(stderr, "<< %d\r\n", found);

    search_destroy(search);
  }

  needle_encoding->destroy(needle_encoding);
  output_encoding->destroy(output_encoding);
}
