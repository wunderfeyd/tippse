#ifndef TIPPSE_DOCUMENT_H
#define TIPPSE_DOCUMENT_H

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "list.h"
#include "misc.h"

struct document;
struct screen;
struct splitter;

struct document {
  void (*reset)(struct document* base, struct splitter* splitter);
  void (*draw)(struct document* base, struct screen* screen, struct splitter* splitter);
  void (*keypress)(struct document* base, struct splitter* splitter, int command, int key, codepoint_t cp, int button, int button_old, int x, int y);
  int (*incremental_update)(struct document* base, struct splitter* splitter);
};

#include "misc.h"
#include "trie.h"
#include "filetype.h"
#include "rangetree.h"
#include "screen.h"
#include "clipboard.h"
#include "documentview.h"
#include "documentfile.h"
#include "encoding.h"
#include "unicode.h"
#include "splitter.h"
#include "document_text.h"
#include "search.h"

int document_search(struct splitter* splitter, struct range_tree_node* search_text, struct range_tree_node* replace_text, int reverse, int ignore_case, int regex, int all, int replace);
void document_search_directory(const char* path, struct range_tree_node* search_text, struct range_tree_node* replace_text, int ignore_case, int regex, int replace);
void document_directory(struct document_file* file, struct stream* filter_stream, struct encoding* filter_encoding);

#endif /* #ifndef TIPPSE_DOCUMENT_H */
