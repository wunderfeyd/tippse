#ifndef TIPPSE_EDITOR_H
#define TIPPSE_EDITOR_H

#include <stdlib.h>
#include <string.h>

#include "misc.h"
#include "documentfile.h"
#include "screen.h"
#include "splitter.h"
#include "list.h"

struct editor {
  int close;                  // editor closing?

  const char* base_path;      // base file path of startup
  struct screen* screen;      // display manager
  struct list* documents;     // open documents

  struct document_file* tabs_doc;     // document: open documents
  struct document_file* browser_doc;  // document: list of file from current directory
  struct document_file* search_doc;   // document: current search text
  struct document_file* document_doc; // document: inital empty document

  struct splitter* splitters;         // Tree of splitters
  struct splitter* panel;             // Extra panel for toolbox user input
  struct splitter* focus;             // Current focused document
  struct splitter* document;          // Current selected document
  struct splitter* last_document;     // Last selected user document
};

struct editor* editor_create(const char* base_path, struct screen* screen, int argc, const char** argv);
void editor_destroy(struct editor* base);
void editor_closed(struct editor* base);
void editor_draw(struct editor* base);
void editor_tick(struct editor* base);
void editor_keypress(struct editor* base, int cp, int modifier, int button, int button_old, int x, int y);

#endif /* #ifndef TIPPSE_EDITOR_H */
