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
  memset(base, 0, sizeof(struct search_node));
  base->type = type;
  base->plain = NULL;
  base->sub = list_create(sizeof(struct search_node*));
  base->group_start = list_create(sizeof(size_t));
  base->group_end = list_create(sizeof(size_t));
  base->stack = NULL;
  base->next = NULL;
  base->forward = NULL;
  return base;
}

void search_node_destroy(struct search_node* base) {
  search_node_empty(base);
  free(base);
}

void search_node_empty(struct search_node* base) {
  while (base->sub->first) {
    list_remove(base->sub, base->sub->first);
  }
  list_destroy(base->sub);

  while (base->group_start->first) {
    list_remove(base->group_start, base->group_start->first);
  }
  list_destroy(base->group_start);

  while (base->group_end->first) {
    list_remove(base->group_end, base->group_end->first);
  }
  list_destroy(base->group_end);

  free(base->plain);
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

    while (node->sub->first) {
      search_node_destroy_recursive(*(struct search_node**)list_object(node->sub->first));
      list_remove(node->sub, node->sub->first);
    }

    search_node_destroy(node);

    node = next;
  }
}

int search_node_count(struct search_node* node) {
  int count = 0;
  while (node) {
    struct list_node* subs = node->sub->first;
    while (subs) {
      count += search_node_count(*((struct search_node**)list_object(subs)));
      subs = subs->next;
    }

    count++;
    node = node->next;
  }

  return count;
}

void search_node_set(struct search_node* node, size_t index) {
  node->set[index/SEARCH_NODE_SET_BUCKET] |= ((uint32_t)1)<<(index%SEARCH_NODE_SET_BUCKET);
}

void search_node_set_decode_rle(struct search_node* node, int invert, uint16_t* rle) {
  codepoint_t codepoint = 0;
  while (1) {
    int codes = (int)*rle++;
    if (codes==0) {
      break;
    }

    int set = (codes&1)^invert;
    codes >>= 1;
    if (set) {
      while (codes-->0) {
        if (codepoint<SEARCH_NODE_SET_CODES) {
          search_node_set(node, (size_t)codepoint);
        }
        codepoint++;
      }
    } else {
      codepoint += codes;
    }
  }
}



void search_destroy(struct search* base) {
  search_node_destroy_recursive(base->root);
  free(base->group_hits);
  free(base->stack);
  free(base);
}

struct search* search_create_plain(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding) {
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
    if (cp==0) {
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

    offset += search_append_unicode(last, ignore_case, &cache, offset, output_encoding);
  }

  search_optimize(base, output_encoding);
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
  list_insert(group[group_depth-1]->sub, NULL, &last);

  size_t offset = 0;
  int escape = 0;
  while (1) {
    codepoint_t cp = encoding_cache_find_codepoint(&cache, offset);
    if (cp==0) {
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
        list_insert(group[group_depth-1]->sub, NULL, &next);
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
          list_insert(last->group_start, NULL, &base->groups);
          list_insert(last->group_end, NULL, &base->groups);
          base->groups++;
        }

        struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BRANCH);
        next->min = 1;
        next->max = 1;
        list_insert(group[group_depth-1]->sub, NULL, &next);
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
        for (size_t n = 0; n<SEARCH_NODE_SET_SIZE; n++) {
          next->set[n] = ~(uint32_t)0;
        }
        last->next = next;
        last = next;
        offset++;
        continue;
      }
    } else {
      escape = 0;
      if (cp=='r' || cp=='n' || cp=='t' || cp=='b' || cp=='0' || cp=='d' || cp=='D' || cp=='w' || cp=='W' || cp=='s' || cp=='S') {
        struct search_node* next = search_node_create(SEARCH_NODE_TYPE_SET);
        next->min = 1;
        next->max = 1;
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

    offset += search_append_unicode(last, ignore_case, &cache, offset, output_encoding);
  }

  if (base->groups>0) {
    base->group_hits = (struct search_group*)malloc(sizeof(struct search_group)*base->groups);
    for (size_t n = 0; n<base->groups; n++) {
      base->group_hits[n].node_start = NULL;
      base->group_hits[n].node_end = NULL;
    }
  }

  search_optimize(base, output_encoding);
  return base;
}

size_t search_append_unicode(struct search_node* last, int ignore_case, struct encoding_cache* cache, size_t offset, struct encoding* output_encoding) {
  size_t advance = 1;
  if (ignore_case) {
    advance = 0;
    size_t length = 0;
    struct unicode_transform_node* transform = unicode_transform(unicode_transform_upper, cache, offset, &advance, &length);
    if (!transform)
      transform = unicode_transform(unicode_transform_lower, cache, offset, &advance, &length);

    if (transform) {
      search_append_next_codepoint(last, &transform->cp[0], transform->length);
    } else {
      advance = 1;
    }
  }

  codepoint_t cp[8];
  for (size_t n = 0; n<advance; n++) {
    cp[n] = encoding_cache_find_codepoint(cache, offset+n);
  }
  search_append_next_codepoint(last, &cp[0], advance);
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
  list_insert(last->sub, NULL, &check);
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
      for (size_t n = 0; n<SEARCH_NODE_SET_CODES && count<8; n++) {
        if (((node->set[n/SEARCH_NODE_SET_BUCKET]>>(n%SEARCH_NODE_SET_BUCKET))&1)==1) {
          count++;
          if (n<0x20 || n>0x7f) {
            printf("\\%02x", (int)n);
            length+=3;
          } else {
            printf("%c", (int)n);
            length++;
          }
        }
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
          printf("\\%02x", c);
          length+=3;
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
    search_debug_tree(base, node->next, depth+1, length, node->sub->first?0:1);
  } else {
    printf("\r\n");
  }

  struct list_node* subs = node->sub->first;
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
      //search_debug_tree(base, base->root, 0, 0, 0);
    }
    again = search_optimize_combine_branch(base->root);
    again |= search_optimize_reduce_branch(base->root);
  } while(again);
  search_optimize_native(encoding, base->root);
  do {
    if (SEARCH_DEBUG) {
      printf("loop native.. %d\r\n", search_node_count(base->root));
      //search_debug_tree(base, base->root, 0, 0, 0);
    }
    again = search_optimize_combine_branch(base->root);
    again |= search_optimize_reduce_branch(base->root);
  } while(again);
  search_optimize_plain(base->root);
  search_prepare(base, base->root, NULL);
}

int search_optimize_reduce_branch(struct search_node* node) {
  int again = 0;
  if (node->sub->count==1) {
    struct search_node* check = *((struct search_node**)list_object(node->sub->first));
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
      search_node_group_copy(check->group_start, node->group_start);
      search_node_group_copy(last->group_end, node->group_end);
      check->type |= node->type&~(SEARCH_NODE_TYPE_BRANCH);
      search_node_move(node, check);
      again = 1;
    }
  }

  if (node->sub->count==0 && (node->type&SEARCH_NODE_TYPE_BRANCH) && node->next) {
    search_node_move(node, node->next);
  }

  if (node->next) {
    again |= search_optimize_reduce_branch(node->next);
  }

  struct list_node* subs = node->sub->first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    again |= search_optimize_reduce_branch(check);
    subs = subs->next;
  }

  return again;
}

int search_optimize_combine_branch(struct search_node* node) {
  int again = 0;
  struct list_node* subs1 = node->sub->first;
  while (subs1) {
    struct search_node* check1 = *((struct search_node**)list_object(subs1));
    struct list_node* subs2 = subs1->next;
    while (subs2) {
      struct search_node* check2 = *((struct search_node**)list_object(subs2));
      if (check1->type==check2->type && check1->min==check2->min && check1->max==1 && check2->max==1 && (check1->type&SEARCH_NODE_TYPE_SET)) {
        if (check1->next==NULL && check2->next==NULL) { // Merge only if both paths are equal (or if there are single nodes)
          for (size_t n = 0; n<SEARCH_NODE_SET_SIZE; n++) {
            check1->set[n] |= check2->set[n];
          }
          search_node_group_copy(check1->group_start, check2->group_start);
          search_node_group_copy(check1->group_end, check2->group_end);
          search_node_destroy_recursive(check2);
          list_remove(node->sub, subs2);
          again = 1;
          break;
        }

        size_t n;
        for (n = 0; n<SEARCH_NODE_SET_SIZE; n++) {
          if (check1->set[n]!=check2->set[n]) {
            break;
          }
        }
        if (n==SEARCH_NODE_SET_SIZE) { // Merge if both nodes are equal
          int next1 = (check1->next && (!(check1->next->type&SEARCH_NODE_TYPE_BRANCH) || (check1->next->sub->count!=0)))?1:0;
          int next2 = (check2->next && (!(check2->next->type&SEARCH_NODE_TYPE_BRANCH) || (check2->next->sub->count!=0)))?1:0;
          if ((next1 && next2) || (!next1 && !next2)) {
            if (!check1->next || !(check1->next->type&SEARCH_NODE_TYPE_BRANCH) || check1->next->min!=1 || check1->next->max!=1) {
              struct search_node* branch = search_node_create(SEARCH_NODE_TYPE_BRANCH);
              branch->min = 1;
              branch->max = 1;
              branch->next = NULL;
              if (check1->next) {
                list_insert(branch->sub, NULL, &check1->next);
              }
              check1->next = branch;
            }

            if (check2->next) {
              if ((check2->next->type&SEARCH_NODE_TYPE_BRANCH) && check2->next->min==1 && check2->next->max==1) {
                struct list_node* subs = check2->next->sub->first;
                while (subs) {
                  list_insert(check1->next->sub, NULL, list_object(subs));
                  subs = subs->next;
                }

                if (check2->next->next) {
                  list_insert(check1->next->sub, NULL, &check2->next->next);
                  search_node_group_copy(check1->next->group_start, check2->next->group_start);
                  search_node_group_copy(check1->next->group_end, check2->next->group_end);
                  search_node_destroy(check2->next);
                }
              } else {
                list_insert(check1->next->sub, NULL, &check2->next);
              }
              search_node_group_copy(check1->next->group_start, check2->group_start);
              search_node_group_copy(check1->next->group_end, check2->group_end);
              search_node_destroy(check2);
            }
            list_remove(node->sub, subs2);
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

  struct list_node* subs = node->sub->first;
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

  struct list_node* subs = node->sub->first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    search_optimize_native(encoding, check);
    subs = subs->next;
  }

  if ((node->type&SEARCH_NODE_TYPE_SET) && !(node->type&SEARCH_NODE_TYPE_BYTE)) {
    int count = 0;
    for (size_t n = 0; n<SEARCH_NODE_SET_SIZE && count<SEARCH_NODE_TYPE_NATIVE_COUNT; n++) {
      if (node->set[n]!=0) {
        for (size_t m = 0; m<SEARCH_NODE_SET_BUCKET && count<SEARCH_NODE_TYPE_NATIVE_COUNT; m++) {
          if (((node->set[n]>>m)&1)) {
            count++;
          }
        }
      }
    }

    if (count<SEARCH_NODE_TYPE_NATIVE_COUNT) {
      node->type &= ~SEARCH_NODE_TYPE_SET;
      node->type |= SEARCH_NODE_TYPE_BRANCH;

      for (size_t n = 0; n<SEARCH_NODE_SET_SIZE; n++) {
        if (node->set[n]!=0) {
          for (size_t m = 0; m<SEARCH_NODE_SET_BUCKET; m++) {
            if (((node->set[n]>>m)&1)) {
              uint8_t coded[1024];
              size_t size = encoding->encode(encoding, (codepoint_t)(n*SEARCH_NODE_SET_BUCKET+m), &coded[0], 8);
              if (size>0) {
                search_append_next_byte(node, &coded[0], size);
              }
            }
          }
        }
      }
    }
  }
}

void search_optimize_plain(struct search_node* node) {
  if ((node->type&SEARCH_NODE_TYPE_BYTE) && node->size==0 && node->sub->count==0 && node->min==node->max && node->max<16 && node->min>0) {
    size_t set = 256;
    for (size_t n = 0; n<256; n++) {
      if (((node->set[n/SEARCH_NODE_SET_BUCKET]>>(n%SEARCH_NODE_SET_BUCKET))&1)) {
        if (set==256) {
          set = n;
        } else {
          set = 256;
          break;
        }
      }
    }

    if (set<256) {
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

  struct list_node* subs = node->sub->first;
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
    node->next = node->next->next;
  }
}

void search_prepare(struct search* base, struct search_node* node, struct search_node* prev) {
  if (node->group_start->first) {
    node->type |= SEARCH_NODE_TYPE_GROUP_START;
    struct list_node* groups = node->group_start->first;
    while (groups) {
      base->group_hits[*(size_t*)list_object(groups)].node_start = node;
      groups = groups->next;
    }
  }
  if (node->group_end->first) {
    node->type |= SEARCH_NODE_TYPE_GROUP_END;
    struct list_node* groups = node->group_end->first;
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

  struct list_node* subs = node->sub->first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    search_prepare(base, check, node);
    subs = subs->next;
  }
}

int search_find(struct search* base, struct encoding_stream* text) {
  if (!base->root) {
    return 0;
  }

  while (!encoding_stream_end(text)) {
    if (search_find_loop(base, base->root, text)) {
      base->hit_start = *text;
      return 1;
    }

    encoding_stream_forward(text, 1);
  }

  return 0;
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
          if (encoding_stream_end(&stream) || node->plain[n]!=encoding_stream_peek(&stream, 0)) {
            enter = 0;
            break;
          }
          encoding_stream_forward(&stream, 1);
        }
      } else {
        if ((node->type&SEARCH_NODE_TYPE_BYTE)) {
          if (enter) {
            if (!(node->type&SEARCH_NODE_TYPE_POSSESSIVE)) {
              for (size_t n = 0; n<node->min; n++) {
                uint8_t c = encoding_stream_peek(&stream, 0);
                if (encoding_stream_end(&stream) || !((node->set[c/SEARCH_NODE_SET_BUCKET]>>(c%SEARCH_NODE_SET_BUCKET))&1)) {
                  enter = 0;
                  break;
                }
                encoding_stream_forward(&stream, 1);
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
              for (size_t n = 0; n<node->max; n++) {
                uint8_t c = encoding_stream_peek(&stream, 0);
                if (encoding_stream_end(&stream) || !((node->set[c/SEARCH_NODE_SET_BUCKET]>>(c%SEARCH_NODE_SET_BUCKET))&1)) {
                  enter = (n>=node->min)?1:0;
                  break;
                }
                encoding_stream_forward(&stream, 1);
              }
            }
          } else {
            struct search_stack* index = node->stack;
            index->loop++;
            enter = (index->loop<=node->max && ((node->type&SEARCH_NODE_TYPE_GREEDY) || load->hits==hits))?1:0;
            if (index->loop<node->max) {
              uint8_t c = encoding_stream_peek(&load->stream, 0);
              if (encoding_stream_end(&load->stream) || !((node->set[c/SEARCH_NODE_SET_BUCKET]>>(c%SEARCH_NODE_SET_BUCKET))&1)) {
                enter = 0;
              }
              encoding_stream_forward(&load->stream, 1);
            }

            if (!enter) {
              node->stack = load->previous;
              load--;
            } else {
              stream = load->stream;
            }
          }
        } else {
          if (enter) {
            if (!(node->type&SEARCH_NODE_TYPE_POSSESSIVE)) {
              for (size_t n = 0; n<node->min; n++) {
                size_t length;
                codepoint_t c = base->encoding->decode(base->encoding, &stream, &length);
                if (encoding_stream_end(&stream) || length==0 || c==-1 || !((node->set[((size_t)c)/SEARCH_NODE_SET_BUCKET]>>(((size_t)c)%SEARCH_NODE_SET_BUCKET))&1)) {
                  enter = 0;
                  break;
                }
                encoding_stream_forward(&stream, length);
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
              for (size_t n = 0; n<node->max; n++) {
                size_t length;
                codepoint_t c = base->encoding->decode(base->encoding, &stream, &length);
                if (encoding_stream_end(&stream) || length==0 || c==-1 || !((node->set[((size_t)c)/SEARCH_NODE_SET_BUCKET]>>(((size_t)c)%SEARCH_NODE_SET_BUCKET))&1)) {
                  enter = (n>=node->min)?1:0;
                  break;
                }
                encoding_stream_forward(&stream, length);
              }
            }
          } else {
            struct search_stack* index = node->stack;
            index->loop++;
            enter = (index->loop<=node->max && ((node->type&SEARCH_NODE_TYPE_GREEDY) || load->hits==hits))?1:0;
            if (index->loop<node->max) {
              size_t length;
              codepoint_t c = base->encoding->decode(base->encoding, &load->stream, &length);
              if (encoding_stream_end(&load->stream) || length==0 || c==-1 || !((node->set[((size_t)c)/SEARCH_NODE_SET_BUCKET]>>(((size_t)c)%SEARCH_NODE_SET_BUCKET))&1)) {
                enter = 0;
              }
              encoding_stream_forward(&load->stream, length);
            }

            if (!enter) {
              node->stack = load->previous;
              load--;
            } else {
              stream = load->stream;
            }
          }
        }
      }
    } else {
      if (enter) {
        load++;
        load->node = node;
        load->previous = node->stack;
        load->loop = (enter==2)?node->stack->loop+1:0;
        load->sub = node->sub->first;
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
            if ((node->type&(SEARCH_NODE_TYPE_GREEDY|SEARCH_NODE_TYPE_POSSESSIVE)) || index->loop<node->min || load->sub!=node->sub->first || load->hits==hits) {
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
  //const char* needle_text = "Hallo äÖÜ ß Test Ú ń ǹ";
  //const char* needle_text = "(haLlO|(Teßt){1,2}?)";
  const char* needle_text = "(\\S+)\\s(sch)*(schnappi|schnapp|schlappi).*?pp";
  //const char* needle_text = "(schnappi|schnapp|schlappi){2}";
  struct encoding_stream needle_stream;
  encoding_stream_from_plain(&needle_stream, (uint8_t*)needle_text, strlen(needle_text));
  struct encoding* needle_encoding = encoding_utf8_create();
  struct encoding* output_encoding = encoding_utf8_create();
  struct search* search = search_create_regex(1, 0, needle_stream, needle_encoding, output_encoding);
  search_debug_tree(search, search->root, 0, 0, 0);

  struct encoding_stream text;
  const char* text_text = "Dies ist ein l TesstTeßt, HaLLo schschschSchlappiSchlappi ppi!";
  encoding_stream_from_plain(&text, (uint8_t*)text_text, strlen(text_text));
  int found = search_find(search, &text);
  printf("%d\r\n", found);
  if (found) {
    printf("\"");
    for (const uint8_t* plain = search->hit_start.plain; plain!=search->hit_end.plain; plain++) {
      printf("%c", *plain);
    }
    printf("\"\r\n");
    for (size_t n = 0; n<search->groups; n++) {
      printf("%d: \"", n);
      for (const uint8_t* plain = search->group_hits[n].start.plain; plain!=search->group_hits[n].end.plain; plain++) {
        printf("%c", *plain);
      }
      printf("\"\r\n");
    }
  }
  search_destroy(search);
  needle_encoding->destroy(needle_encoding);
  output_encoding->destroy(output_encoding);
}
