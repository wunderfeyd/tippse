// Tippse - Search - Compilation of text search algorithms

#include "search.h"

extern struct trie* unicode_transform_lower;
extern struct trie* unicode_transform_upper;

struct search_node* search_node_create(int type) {
  struct search_node* base = malloc(sizeof(struct search_node));
  memset(base, 0, sizeof(struct search_node));
  base->type = type;
  base->plain = NULL;
  base->sub = list_create(sizeof(struct search_node*));
  return base;
}

struct search* search_create_plain(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding) {
  struct search* base = malloc(sizeof(struct search));
  base->reverse = reverse;
  base->root = NULL;

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

  search_optimize(base);
  return base;
}

struct search* search_create_regex(int ignore_case, int reverse, struct encoding_stream needle, struct encoding* needle_encoding, struct encoding* output_encoding) {
  struct search* base = malloc(sizeof(struct search));
  base->reverse = reverse;

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
    }

    struct search_node* next = search_node_create(SEARCH_NODE_TYPE_BRANCH);
    next->min = 1;
    next->max = 1;
    last->next = next;
    last = next;

    offset += search_append_unicode(last, ignore_case, &cache, offset, output_encoding);
    escape = 0;
  }

  search_optimize(base);
  return base;
}

size_t search_append_unicode(struct search_node* last, int ignore_case, struct encoding_cache* cache, size_t offset, struct encoding* output_encoding) {
  size_t advance = 1;
  uint8_t coded[1024];
  if (ignore_case) {
    size_t advance = 0;
    size_t length = 0;
    struct unicode_transform_node* transform = unicode_transform(unicode_transform_upper, cache, offset, &advance, &length);
    if (!transform)
      transform = unicode_transform(unicode_transform_lower, cache, offset, &advance, &length);

    if (transform) {
      size_t size = 0;
      for (size_t n = 0; n<transform->length; n++) {
        size += output_encoding->encode(output_encoding, transform->cp[n], &coded[size], 8);
      }
      if (size>0) {
        search_append_next_char(last, &coded[0], size);
      }
    } else {
      advance = 1;
    }
  }

  size_t size = 0;
  for (size_t n = 0; n<advance; n++) {
    size += output_encoding->encode(output_encoding, encoding_cache_find_codepoint(cache, offset+n), &coded[size], 8);
  }
  if (size>0) {
    search_append_next_char(last, &coded[0], size);
  }
  return advance;
}

void search_append_next_char(struct search_node* last, uint8_t* buffer, size_t size) {
  if (size==0) {
    return;
  }

  struct list_node* subs = last->sub->first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    if (check->min==1 && check->max==1) {
      size_t n;
      for (n = 0; n<256; n++) {
        // TODO: this line is very ugly
        if (((check->set[n/SEARCH_NODE_SET_BUCKET]>>(n%SEARCH_NODE_SET_BUCKET))&1)!=((*buffer==n)?1:0)) {
          break;
        }
      }
      if (n==256) {
        search_append_next_char(check, buffer+1, size-1);
        return;
      }
    }

    subs = subs->next;
  }

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

  struct search_node* check = search_node_create(SEARCH_NODE_TYPE_SET);
  check->min = 1;
  check->max = 1;
  check->set[(*buffer)/SEARCH_NODE_SET_BUCKET] |= ((uint32_t)1)<<((*buffer)%SEARCH_NODE_SET_BUCKET);
  list_insert(last->sub, NULL, &check);
  search_append_next_char(check, buffer+1, size-1);
}

void search_destroy(struct search* base) {
  free(base);
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
  if (node->type&SEARCH_NODE_TYPE_BRANCH) {
    printf("B");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_GREEDY) {
    printf("G");
    length++;
  }

  if (node->type&SEARCH_NODE_TYPE_SET) {
    if (node->size==0) {
      printf("[");
      for (size_t n = 0; n<256; n++) {
        if (((node->set[n/SEARCH_NODE_SET_BUCKET]>>(n%SEARCH_NODE_SET_BUCKET))&1)) {
          if (n<0x20 || n>0x7f) {
            printf("\\%02x", (int)n);
            length+=3;
          } else {
            printf("%c", (int)n);
            length++;
          }
        }
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
    printf(" - ");
    length+=3;
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

void search_optimize(struct search* base) {
  int again;
  do {
    printf("loop..\r\n");
    search_debug_tree(base, base->root, 0, 0, 0);
    again = search_optimize_combine_branch(base->root);
    again |= search_optimize_reduce_branch(base->root);
  } while(again);
  search_optimize_plain(base->root);
}

int search_optimize_reduce_branch(struct search_node* node) {
  int again = 0;
  if (node->sub->count==1) {
    struct search_node* check = *((struct search_node**)list_object(node->sub->first));
    int flat = 1;
    struct search_node* crawl = check;
    struct search_node* last = check;
    size_t count = 0;
    while (crawl) {
      if (crawl->sub->count!=0) {
        flat = 0;
        break;
      }
      last = crawl;
      crawl = crawl->next;
      count++;
    }

    if (flat && ((node->min==1 && node->max==1) || (count==1 && ((last->min==last->max) || (node->min==node->max))))) {
      last->next = node->next;
      if (count==1) {
        last->min *= node->min;
        last->max *= node->max;
      }
      int type = node->type;
      *node = *check;
      node->type |= type&~SEARCH_NODE_TYPE_BRANCH;
      again = 1;
    } else if (!node->next && (node->min==1 && node->max==1)) {
      *node = *check;
      again = 1;
    }
  }

  if (node->sub->count==0 && (node->type&SEARCH_NODE_TYPE_BRANCH) && node->next) {
    // TODO: clean up node
    *node = *(node->next);
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
                }
              } else {
                list_insert(check1->next->sub, NULL, &check2->next);
              }
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

void search_optimize_plain(struct search_node* node) {
  if (node->size==0 && node->sub->count==0 && node->min==node->max && node->max<16 && node->min>0) {
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

  if (node->next) {
    if (node->size>0 && node->next->size>0) {
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
}

int search_find(struct search* base, struct encoding_stream* text) {
  if (!base->root) {
    return 0;
  }

  while (!encoding_stream_end(text)) {
    struct encoding_stream copy = *text;
    if (search_find_recursive(base, base->root, &copy)) {
      base->hit_start = *text;
      return 1;
    }

    encoding_stream_forward(text, 1);
  }

  return 0;
}

int search_find_recursive_subs(struct search* base, struct search_node* node, struct encoding_stream* text, size_t depth) {
  struct encoding_stream copy = *text;
  if (depth>=node->min) {
    if (search_find_recursive(base, node->next, text)) {
      if (!(node->type&SEARCH_NODE_TYPE_GREEDY)) {
        return 1;
      }
    } else {
      return 0;
    }
  }

  if (depth>=node->max) {
    return 0;
  }

  struct list_node* subs = node->sub->first;
  while (subs) {
    struct search_node* check = *((struct search_node**)list_object(subs));
    *text = copy;
    if (search_find_recursive(base, check, text)) {
      if (search_find_recursive_subs(base, node, text, depth+1)) {
        return 1;
      }
    }

    subs = subs->next;
  }

  if (depth>node->min) {
    *text = copy;
    return 1;
  }

  return 0;
}

int search_find_recursive(struct search* base, struct search_node* node, struct encoding_stream* text) {
  if (!node) {
    // TODO: save groups
    base->hit_end = *text;
    return 1;
  }

  if (node->type&SEARCH_NODE_TYPE_SET) {
    if (node->plain) {
      for (size_t n = 0; n<node->size; n++) {
        if (encoding_stream_end(text) || node->plain[n]!=encoding_stream_peek(text, 0)) {
          return 0;
        }
        encoding_stream_forward(text, 1);
      }
      return search_find_recursive(base, node->next, text);
    } else {
      size_t n;
      for (n = 0; n<node->min; n++) {
        uint8_t c = encoding_stream_peek(text, 0);
        if (encoding_stream_end(text) ||  !((node->set[c/SEARCH_NODE_SET_BUCKET]>>(c%SEARCH_NODE_SET_BUCKET))&1)) {
          return 0;
        }
        encoding_stream_forward(text, 1);
      }

      if (!(node->type&SEARCH_NODE_TYPE_GREEDY)) {
        return search_find_recursive(base, node->next, text);
      } else {
        struct encoding_stream hit;
        while (1) {
          struct encoding_stream copy = *text;
          int found = search_find_recursive(base, node->next, &copy);
          if (!found || n>=node->max) {
            break;
          }

          hit = copy;
          n++;

          uint8_t c = encoding_stream_peek(text, 0);
          if (encoding_stream_end(text) ||  !((node->set[c/SEARCH_NODE_SET_BUCKET]>>(c%SEARCH_NODE_SET_BUCKET))&1)) {
            break;
          }
          encoding_stream_forward(text, 1);
        }

        if (n>node->min) {
          *text = hit;
          return 1;
        }
        return 0;
      }
    }
  } else {
    return search_find_recursive_subs(base, node, text, 0);
  }
}

void search_test(void) {
  //const char* needle_text = "Hallo äÖÜ ß Test Ú ń ǹ";
  //const char* needle_text = "haLlO";
  //const char* needle_text = "(hallo|hall)O (schnappi|schnapp|schlappi)";
  const char* needle_text = "(schnappi|schnapp|schlappi){2}";
  struct encoding_stream needle_stream;
  encoding_stream_from_plain(&needle_stream, (uint8_t*)needle_text, strlen(needle_text));
  struct encoding* needle_encoding = encoding_utf8_create();
  struct encoding* output_encoding = encoding_utf8_create();
  struct search* search = search_create_regex(1, 0, needle_stream, needle_encoding, output_encoding);
  search_debug_tree(search, search->root, 0, 0, 0);

  struct encoding_stream text;
  const char* text_text = "Dies ist ein l Tesst, HaLLo SchlappiSchlappi!";
  encoding_stream_from_plain(&text, (uint8_t*)text_text, strlen(text_text));
  int found = search_find(search, &text);
  printf("%d\r\n", found);
  if (found) {
    printf("\"");
    for (const uint8_t* plain = search->hit_start.plain; plain!=search->hit_end.plain; plain++) {
      printf("%c", *plain);
    }
    printf("\"\r\n");
  }
  exit(0);
}
