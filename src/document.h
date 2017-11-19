#ifndef TIPPSE_DOCUMENT_H
#define TIPPSE_DOCUMENT_H

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

struct document;
struct screen;
struct splitter;

struct document {
  void (*reset)(struct document* base, struct splitter* splitter);
  void (*draw)(struct document* base, struct screen* screen, struct splitter* splitter);
  void (*keypress)(struct document* base, struct splitter* splitter, int command, int key, int cp, int button, int button_old, int x, int y);
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

int document_compare(struct range_tree_node* left, file_offset_t displacement_left, struct range_tree_node* right_root, file_offset_t length);
int document_search(struct splitter* splitter, struct range_tree_node* search_text, file_offset_t search_length, struct range_tree_node* replace_text, file_offset_t replace_length, int forward, int all, int replace);
void document_directory(struct document_file* file);

#endif /* #ifndef TIPPSE_DOCUMENT_H */
