#ifndef __TIPPSE_DOCUMENT_RAW__
#define __TIPPSE_DOCUMENT_RAW__

#include <stdlib.h>

struct document_raw;

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
#include "document.h"
#include "splitter.h"
#include "documentundo.h"

struct document_raw {
  struct document vtbl;
};

struct document* document_raw_create();
void document_raw_destroy(struct document* base);

void document_raw_reset(struct document* base, struct splitter* splitter);
int document_raw_incremental_update(struct document* base, struct splitter* splitter);
void document_raw_draw(struct document* base, struct screen* screen, struct splitter* splitter);
void document_raw_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y);
void document_raw_cursor_from_point(struct document* base, struct splitter* splitter, int x, int y, int* cursor_x, int* cursor_y);
void document_raw_cursor_update(struct document* base, struct splitter* splitter);

#endif /* #ifndef __TIPPSE_DOCUMENT_RAW__ */
