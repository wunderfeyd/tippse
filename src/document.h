#ifndef TIPPSE_DOCUMENT_H
#define TIPPSE_DOCUMENT_H

#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "list.h"

struct document;
struct screen;
struct splitter;
struct config_command;

struct document {
  void (*reset)(struct document* base, struct splitter* splitter);
  void (*draw)(struct document* base, struct screen* screen, struct splitter* splitter);
  void (*keypress)(struct document* base, struct splitter* splitter, int command, struct config_command* arguments, int key, codepoint_t cp, int button, int button_old, int x, int y);
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
#include "editor.h"
#include "config.h"

int document_search(struct splitter* splitter, struct range_tree_node* search_text, struct encoding* search_encoding, struct range_tree_node* replace_text, struct encoding* replace_encoding, int reverse, int ignore_case, int regex, int all, int replace);
void document_search_directory(const char* path, struct range_tree_node* search_text, struct encoding* search_encoding, struct range_tree_node* replace_text, struct encoding* replace_encoding, int ignore_case, int regex, int replace);
void document_directory(struct document_file* file, struct stream* filter_stream, struct encoding* filter_encoding);
void document_select_all(struct splitter* splitter, int update_offset);
void document_select_nothing(struct splitter* splitter);

#endif /* #ifndef TIPPSE_DOCUMENT_H */
