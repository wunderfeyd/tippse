#ifndef __TIPPSE_DOCUMENT_HEX__
#define __TIPPSE_DOCUMENT_HEX__

#include <stdlib.h>

struct document_hex;

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

struct document_hex_char {
  int codepoints[8];
  int visuals[8];
  size_t length;
};

struct document_hex {
  struct document vtbl;
  int cp_first;
};

struct document* document_hex_create();
void document_hex_destroy(struct document* base);

void document_hex_reset(struct document* base, struct splitter* splitter);
int document_hex_incremental_update(struct document* base, struct splitter* splitter);
void document_hex_draw(struct document* base, struct screen* screen, struct splitter* splitter);
void document_hex_render(struct document* base, struct screen* screen, struct splitter* splitter, file_offset_t offset, int y, const uint8_t* data, int data_size, struct document_hex_char* chars);
void document_hex_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y);
void document_hex_cursor_from_point(struct document* base, struct splitter* splitter, int x, int y, file_offset_t* offset);
uint8_t document_hex_value(int cp);
void document_hex_convert(struct document_hex_char* cps, int show_invisibles, int cp_default);

#endif /* #ifndef __TIPPSE_DOCUMENT_HEX__ */
