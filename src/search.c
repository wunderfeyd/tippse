// Tippse - Search - Compilation of text search algorithms

#include "search.h"

extern struct trie* unicode_transform_lower;
extern struct trie* unicode_transform_upper;
extern uint16_t unicode_letters_rle[];
extern uint16_t unicode_whitespace_rle[];
extern uint16_t unicode_digits_rle[];

#define SEARCH_DEBUG 0

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

void search_node_destroy(struct search_node* base) {
  search_node_empty(base);
  free(base);
}

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

void search_node_move(struct search_node* base, struct search_node* copy) {
  search_node_empty(base);
  *base = *copy;
  free(copy);
}

void search_node_group_copy(struct list* to, struct list* from) {
  struct list_node* it = from->first;
  while (it) {
    list_insert(to, NULL, list_object(it));
    it = it->next;
  }
}

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

void search_node_set_build(struct search_node* node) {
  if (!node->set) {
    node->set = range_tree_static(node->set, SEARCH_NODE_SET_CODES, 0);
  }
}

void search_node_set(struct search_node* node, size_t index) {
  search_node_set_build(node);
  node->set = range_tree_mark(node->set, index, 1, TIPPSE_INSERTER_MARK);
}

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



void search_destroy(struct search* base) {
  search_node_destroy_recursive(base->root);
  free(base->group_hits);
  free(base->stack);
  free(base);
}

struct search* search_create_plain(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding) {
  //int64_t tick = tick_count();
  struct search* base = malloc(sizeof(struct search));
  base->reverse = reverse;
  base->groups = 0;
  base->group_hits = NULL;
  base->root = NULL;
  base->stack_size = 1024;
  base->stack = NULL;
  base->encoding = output_encoding;

  struct encoding_cache cache;
  encoding_cache_clear(&cache, needle_encoding, &needle);

  struct search_node* last = NULL;
  size_t offset = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(&cache, offset);
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
  //printf("\r\n%d %d %d\r\n", (int)(tick2-tick), (int)(tick3-tick2), (int)sizeof(struct search_node));
  return base;
}

struct search* search_create_regex(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding) {
  struct search* base = malloc(sizeof(struct search));
  base->reverse = reverse;
  base->groups = 0;
  base->group_hits = NULL;
  base->stack_size = 1024;
  base->stack = NULL;
  base->encoding = output_encoding;

  base->root = search_node_create(SEARCH_NODE_TYPE_BRANCH);
  base->root->min = 1;
  base->root->max = 1;

  struct search_node* group[16];
  size_t group_depth = 1;
  group[0] = base->root;

  struct encoding_cache cache;
  encoding_cache_clear(&cache, needle_encoding, &needle);

  struct search_node* last = search_node_create(SEARCH_NODE_TYPE_BRANCH);
  last->min = 1;
  last->max = 1;
  list_insert(&group[group_depth-1]->sub, NULL, &last);

  size_t offset = 0;
  int escape = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(&cache, offset);
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
        size_t min = (size_t)decode_based_unsigned_offset(&cache, 10, &offset);
        last->min = min;
        last->max = min;
        if (encoding_cache_find_codepoint(&cache, offset)==',') {
          offset++;
          size_t max = (size_t)decode_based_unsigned_offset(&cache, 10, &offset);
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
    } else {
      escape = 0;
      struct search_node* next = search_append_class(last, cp, 1);
      if (next) {
        last->next = next;
        last = next;
        offset++;
        continue;
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

size_t search_append_set(struct search_node* last, int ignore_case, struct encoding_cache* cache, size_t offset) {
  struct search_node* check = search_node_create(SEARCH_NODE_TYPE_SET);
  check->min = 1;
  check->max = 1;
  list_insert(&last->sub, NULL, &check);

  codepoint_t from = 0;
  int escape = 0;
  int invert = 0;
  size_t advance = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(cache, offset+advance);
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
      }
    } else {
      from = 0;
      escape = 0;

      if (search_append_class(check, cp, 0)) {
        advance++;
        continue;
      }
    }

    if (encoding_cache_find_codepoint(cache, offset+advance+1)=='-' && from<=0) {
      from = encoding_cache_find_codepoint(cache, offset+advance);
      advance += 2;
    } else {
      if (from>0) {
        codepoint_t to = encoding_cache_find_codepoint(cache, offset+advance);
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

  if (invert) {
    check->set = range_tree_invert_mark(check->set, TIPPSE_INSERTER_MARK);
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
          struct encoding_stream native_stream;
          encoding_stream_from_plain(&native_stream, (uint8_t*)&from, sizeof(codepoint_t));

          struct encoding_cache native_cache;
          encoding_cache_clear(&native_cache, native_encoding, &native_stream);

          search_append_unicode(last, ignore_case, &native_cache, 0, check, 2);
        }
      }
      codepoint += range->length;
      range = range_tree_next(range);
    }
    native_encoding->destroy(native_encoding);
    range_tree_destroy(source, NULL);
  }

  return advance;
}

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
      search_node_set(shorten, (size_t)encoding_cache_find_codepoint(cache, offset));
    } else {
      codepoint_t cp[8];
      for (size_t n = 0; n<advance; n++) {
        cp[n] = encoding_cache_find_codepoint(cache, offset+n);
      }
      search_append_next_codepoint(last, &cp[0], advance);
    }
  }
  return advance;
}

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

void search_append_next_codepoint(struct search_node* last, codepoint_t* buffer, size_t size) {
  while (size>0) {
    last = search_append_next_index(last, (size_t)*buffer, SEARCH_NODE_TYPE_SET);
    buffer++;
    size--;
  }
}

void search_append_next_byte(struct search_node* last, uint8_t* buffer, size_t size) {
  while (size>0) {
    last = search_append_next_index(last, (size_t)*buffer, SEARCH_NODE_TYPE_SET|SEARCH_NODE_TYPE_BYTE);
    buffer++;
    size--;
  }
}

int search_debug_lengths[128];
int search_debug_stops[128];
void search_debug_indent(size_t depth) {
  for (size_t n = 0; n<depth; n++) {
    if (!search_debug_stops[n]) {
      printf("|");
    } else {
      printf(" ");
    }

    for (int l = 0; l<search_debug_lengths[n]-1; l++) {
      printf(" ");
    }
  }
}

void search_debug_tree(struct search* base, struct search_node* node, size_t depth, int length, int stop) {
  if (depth>0) {
    search_debug_lengths[depth-1] = length;
    search_debug_stops[depth-1] = stop;
  }
  length = 0;
  if (node->type&SEARCH_NODE_TYPE_GROUP_START) {
    printf("(");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_BRANCH) {
    printf("B");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_GREEDY) {
    printf("*");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_POSSESSIVE) {
    printf("+");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_MATCHING_TYPE) {
    printf("M");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_BYTE) {
    printf("8");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_GROUP_END) {
    printf(")");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_SET) {
    if (node->size==0) {
      printf("[");
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
                printf("\\%02x", c);
                length+=3;
              } else {
                printf("\\%06x", c);
                length+=7;
              }
            } else {
              printf("%c", c);
              length++;
            }
          }
        }
        codepoint += range->length;
        range = range_tree_next(range);
      }

      if (count==8) {
        printf("..");
        length+=2;
      }
      printf("]");
      length+=2;
    }

    if (node->size>0) {
      printf("\"");
      for (size_t n = 0; n<node->size; n++) {
        uint8_t c = node->plain[n];
        if (c<0x20 || c>0x7f) {
          if (node->type&SEARCH_NODE_TYPE_BYTE) {
            printf("\\%02x", c);
            length+=3;
          } else {
            printf("\\%06x", c);
            length+=7;
          }
        } else {
          printf("%c", c);
          length++;
        }
      }
      printf("\"");
      length+=2;
    }
  }

  if (node->min!=1 || node->max!=1) {
    char out[1024];
    length += sprintf(&out[0], "{%d,%d}", (int)node->min, (int)node->max);
    printf("%s", &out[0]);
  }

  if (node->next) {
    printf(" ");
    length+=1;
    search_debug_tree(base, node->next, depth+1, length, node->sub.first?0:1);
  } else {
    printf("\r\n");
  }

  struct list_node* subs = node->sub.first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    search_debug_indent(depth);
    printf("`- ");
    subs = subs->next;
    search_debug_tree(base, check, depth+1, 3, subs?0:1);
  }
}

void search_optimize(struct search* base, struct encoding* encoding) {
  int again;
  do {
    if (SEARCH_DEBUG) {
      printf("loop codepoints.. %d\r\n", search_node_count(base->root));
      search_debug_tree(base, base->root, 0, 0, 0);
    }
    again = search_optimize_combine_branch(base->root);
    again |= search_optimize_reduce_branch(base->root);
  } while(again);
  search_optimize_native(encoding, base->root);
  do {
    if (SEARCH_DEBUG) {
      printf("loop native.. %d\r\n", search_node_count(base->root));
      search_debug_tree(base, base->root, 0, 0, 0);
    }
    again = search_optimize_combine_branch(base->root);
    again |= search_optimize_reduce_branch(base->root);
  } while(again);
  search_optimize_plain(base->root);
  search_prepare(base, base->root, NULL);
  search_prepare_hash(base, base->root);
}

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

  if (node->next && node->size>0 && node->next->size>0 && !(node->type&SEARCH_NODE_TYPE_GROUP_END) && !(node->next->type&SEARCH_NODE_TYPE_GROUP_START)) {
    size_t size = node->size+node->next->size;
    uint8_t* plain = malloc(sizeof(uint8_t)*size);
    memcpy(plain, node->plain, node->size);
    memcpy(plain+node->size, node->next->plain, node->next->size);
    free(node->plain);
    node->plain = plain;
    node->size = size;
    struct search_node* remove = node->next;
    node->next = node->next->next;
    search_node_destroy(remove);
  }
}

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

inline void search_prepare_hash_next(uint32_t* generator) {
  *generator = ((*generator)*(*generator)+(*generator))&65535;
}

void search_prepare_hash(struct search* base, struct search_node* node) {
  uint32_t generator = 65521;
  for (size_t n = 0; n<256; n++) {
    base->hashes[n] = 0;
  }

  base->hash_length = 0;
  base->hash = 0;
  while (node) {
    if (!(node->type&SEARCH_NODE_TYPE_SET) || !(node->type&SEARCH_NODE_TYPE_BYTE) ||  node->min!=node->max) {
      break;
    }

    if (node->plain) {
      for (size_t n = 0; n<node->size; n++) {
        uint8_t codepoint = node->plain[n];
        if (base->hashes[codepoint]==0) {
          search_prepare_hash_next(&generator);
          base->hashes[codepoint] = generator;
        }
        base->hash += base->hashes[codepoint];
      }
      base->hash_length += node->size;
    } else {
      uint32_t hash = 0;
      int duplicate = 0;
      size_t codepoint = 0;
      struct range_tree_node* range = range_tree_first(node->set);
      while (range && !duplicate) {
        if (range->inserter&TIPPSE_INSERTER_MARK) {
          for (size_t n = codepoint; n<codepoint+range->length; n++) {
            if (base->hashes[n]!=0 && hash!=base->hashes[n]) {
              if (hash==0) {
                hash = base->hashes[n];
              } else {
                duplicate = 1;
                break;
              }
            }
          }
        }
        codepoint += range->length;
        range = range_tree_next(range);
      }

      if (duplicate) {
        break;
      }

      if (hash==0) {
        search_prepare_hash_next(&generator);
        hash = generator;
      }

      codepoint = 0;
      range = range_tree_first(node->set);
      while (range) {
        if (range->inserter&TIPPSE_INSERTER_MARK) {
          for (size_t n = codepoint; n<codepoint+range->length; n++) {
            base->hashes[n] = hash;
          }
        }
        codepoint += range->length;
        range = range_tree_next(range);
      }
      base->hash += hash*node->min;
      base->hash_length += node->min;
    }

    node = node->next;
  }

  //printf("%08x - %d\r\n", (int)base->hash, (int)base->hash_length);
  base->hashed = (base->hash_length>2)?1:0;
}

int search_find(struct search* base, struct encoding_stream* text, file_offset_t* left) {
  if (!base->root) {
    return 0;
  }

  file_offset_t count = left?*left:FILE_OFFSET_T_MAX;

  if (base->hashed) {
    if (!base->reverse) {
      struct encoding_stream head = *text;
      uint32_t hash = 0;
      for (size_t n = 0; n<base->hash_length; n++) {
        hash += base->hashes[encoding_stream_read_forward(&head)];
      }

      while (!encoding_stream_end(text) && count>0) {
        count--;
        if (hash==base->hash) {
          if (search_find_loop(base, base->root, text)) {
            base->hit_start = *text;
            encoding_stream_forward(text, 1);
            return 1;
          }
        }

        hash -= base->hashes[encoding_stream_read_forward(text)];
        hash += base->hashes[encoding_stream_read_forward(&head)];
      }
    } else {
      struct encoding_stream head = *text;
      uint32_t hash = 0;
      for (size_t n = 0; n<base->hash_length; n++) {
        hash += base->hashes[encoding_stream_read_reverse(text)];
      }

      while (!encoding_stream_start(text) && count>0) {
        count--;
        if (hash==base->hash) {
          if (search_find_loop(base, base->root, text)) {
            base->hit_start = *text;
            encoding_stream_reverse(text, 1);
            return 1;
          }
        }

        hash -= base->hashes[encoding_stream_read_reverse(&head)];
        hash += base->hashes[encoding_stream_read_reverse(text)];
      }
    }
  } else {
    if (!base->reverse) {
      while (!encoding_stream_end(text) && count>0) {
        count--;
        if (search_find_loop(base, base->root, text)) {
          base->hit_start = *text;
          encoding_stream_forward(text, 1);
          return 1;
        }

        encoding_stream_forward(text, 1);
      }
    } else {
      while (!encoding_stream_start(text) && count>0) {
        count--;
        if (search_find_loop(base, base->root, text)) {
          base->hit_start = *text;
          encoding_stream_reverse(text, 1);
          return 1;
        }

        encoding_stream_reverse(text, 1);
      }
    }
  }

  if (left) {
    *left = count;
  }

  return 0;
}

inline int search_node_bitset_check(struct search_node* node, uint8_t index) {
  return ((node->bitset[index/SEARCH_NODE_SET_BUCKET]>>(index%SEARCH_NODE_SET_BUCKET))&1);
}

inline int search_node_set_check(struct search_node* node, codepoint_t index) {
  return index>=0 && range_tree_marked(node->set, (file_offset_t)index, 1, TIPPSE_INSERTER_MARK);
}

int search_find_loop(struct search* base, struct search_node* node, struct encoding_stream* text) {
  if (!base->stack) {
    base->stack = (struct search_stack*)malloc(sizeof(struct search_stack*)*base->stack_size);
  }

  struct encoding_stream stream = *text;
  struct search_stack* load = base->stack;
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
          if (encoding_stream_end(&stream) || node->plain[n]!=encoding_stream_read_forward(&stream)) {
            enter = 0;
            break;
          }
        }
      } else {
        if (enter) {
          if (!(node->type&SEARCH_NODE_TYPE_POSSESSIVE)) {
            if ((node->type&SEARCH_NODE_TYPE_BYTE)) {
              for (size_t n = 0; n<node->min; n++) {
                if (encoding_stream_end(&stream) || !search_node_bitset_check(node, encoding_stream_read_forward(&stream))) {
                  enter = 0;
                  break;
                }
              }
            } else {
              for (size_t n = 0; n<node->min; n++) {
                size_t length;
                if (encoding_stream_end(&stream) || !search_node_set_check(node, base->encoding->decode(base->encoding, &stream, &length))) {
                  enter = 0;
                  break;
                }
              }
            }

            if (enter && node->min<node->max) {
              load++;
              load->node = node;
              load->previous = node->stack;
              load->loop = node->min;
              load->hits = hits;
              node->stack = load;
              load->stream = stream;
            }
          } else {
            if ((node->type&SEARCH_NODE_TYPE_BYTE)) {
              for (size_t n = 0; n<node->max; n++) {
                if (encoding_stream_end(&stream) || !search_node_bitset_check(node, encoding_stream_read_forward(&stream))) {
                  encoding_stream_reverse(&stream, 1);
                  enter = (n>=node->min)?1:0;
                  break;
                }
              }
            } else {
              for (size_t n = 0; n<node->max; n++) {
                size_t length = 0;
                if (encoding_stream_end(&stream) || !search_node_set_check(node, base->encoding->decode(base->encoding, &stream, &length))) {
                  encoding_stream_reverse(&stream, length);
                  enter = (n>=node->min)?1:0;
                  break;
                }
              }
            }
          }
        } else {
          struct search_stack* index = node->stack;
          index->loop++;
          enter = (index->loop<=node->max && ((node->type&SEARCH_NODE_TYPE_GREEDY) || load->hits==hits))?1:0;
          if (index->loop<node->max) {
            if ((node->type&SEARCH_NODE_TYPE_BYTE)) {
              if (encoding_stream_end(&load->stream) || !search_node_bitset_check(node, encoding_stream_read_forward(&load->stream))) {
                enter = 0;
              }
            } else {
              size_t length;
              if (encoding_stream_end(&load->stream) || !search_node_set_check(node, base->encoding->decode(base->encoding, &load->stream, &length))) {
                enter = 0;
              }
            }
          }

          if (!enter) {
            node->stack = load->previous;
            load--;
          } else {
            stream = load->stream;
          }
        }
      }
    } else {
      if (enter) {
        load++;
        load->node = node;
        load->previous = node->stack;
        load->loop = (enter==2)?node->stack->loop+1:0;
        load->sub = node->sub.first;
        load->stream = stream;
        load->hits = hits;
        node->stack = load;
        enter = 1;
      } else {
        stream = load->stream;
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
              load--;
              enter = 1;
            }
          }
        }
      }
      if (!enter) {
        node->stack = load->previous;
        load--;
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

      for (size_t n = 0; n<base->groups; n++) {
        base->group_hits[n].start = base->group_hits[n].node_start->start;
        base->group_hits[n].end = base->group_hits[n].node_end->end;
      }
      base->hit_end = stream;
      hit = 1;
      hits++;
      enter = 0;
    }

    node = load->node;
    if (!node) {
      break;
    }
  }

  return hit;
}

void search_test(void) {
  const char* needle_text = "Recently, Standard Japanese has ";
  //const char* needle_text = "Hallo äÖÜ ß Test Ú ń ǹ";
  //const char* needle_text = "(haLlO|(Teßt){1,2}?)";
  //const char* needle_text = "(\\S+)\\s(sch)*(schnappi|schnapp|schlappi).*?[A-C][^\\d]{2}";
  //const char* needle_text = "a|ab|abc";
  //const char* needle_text = "(schnappi|schnapp|schlappi){2}";
  //const char* needle_text = "make_";
  //const char* needle_text = "schnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappischnappi";
    struct encoding_stream needle_stream;
  encoding_stream_from_plain(&needle_stream, (uint8_t*)needle_text, strlen(needle_text));
  struct encoding* needle_encoding = encoding_utf8_create();
  struct encoding* output_encoding = encoding_utf8_create();
  struct search* search = search_create_regex(1, 0, needle_stream, needle_encoding, output_encoding);
  search_debug_tree(search, search->root, 0, 0, 0);

  struct encoding_stream text;
  const char* text_text = "Dies ist ein l TesstTeßt, HaLLo schschschSchlappiSchlappi ppi! 999 make_debug.sh abc";
  encoding_stream_from_plain(&text, (uint8_t*)text_text, strlen(text_text));
  int found = search_find(search, &text, NULL);
  printf("%d\r\n", found);
  if (found) {
    printf("\"");
    for (const uint8_t* plain = search->hit_start.plain+search->hit_start.displacement; plain!=search->hit_end.plain+search->hit_end.displacement; plain++) {
      printf("%c", *plain);
    }
    printf("\"\r\n");
    for (size_t n = 0; n<search->groups; n++) {
      printf("%d: \"", n);
      for (const uint8_t* plain = search->group_hits[n].start.plain+search->group_hits[n].start.displacement; plain!=search->group_hits[n].end.plain+search->group_hits[n].end.displacement; plain++) {
        printf("%c", *plain);
      }
      printf("\"\r\n");
    }
  }
  search_destroy(search);

  /*for (size_t n = 0; n<100; n++) {
    struct search* search = search_create_plain(1, 0, needle_stream, needle_encoding, output_encoding);
    search_destroy(search);
  }*/

  int f = open("enwik8", O_RDONLY);
  if (f!=-1) {
    size_t length = 100*1000*1000;
    uint8_t* buffer = (uint8_t*)malloc(length);
    read(f, buffer, length);
    close(f);

    printf("hash search...\r\n");
    struct encoding_stream buffered;
    struct search* search = search_create_plain(1, 0, needle_stream, needle_encoding, output_encoding);
    search_debug_tree(search, search->root, 0, 0, 0);
    int64_t tick = tick_count();
    for (size_t n = 0; n<100; n++) {
      encoding_stream_from_plain(&buffered, buffer, length);
      int found = search_find(search, &buffered, NULL);
      printf("%d %d us %d\r\n", (int)n, (int)((tick_count()-tick)/(n+1)), found);
      if (found) {
        printf("\"");
        for (const uint8_t* plain = search->hit_start.plain+search->hit_start.displacement; plain!=search->hit_end.plain+search->hit_end.displacement; plain++) {
          printf("%c", *plain);
        }
        printf("\"\r\n");
      }
      fflush(stdout);
    }
    search_destroy(search);
    free(buffer);
  }

  needle_encoding->destroy(needle_encoding);
  output_encoding->destroy(output_encoding);
}
