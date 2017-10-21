#ifndef TIPPSE_DOCUMENT_HEX_H
#define TIPPSE_DOCUMENT_HEX_H

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
  int codepoints[8];        // Unicode code points
  int visuals[8];           // Unicode code points to show
  size_t length;            // number of code points
};

struct document_hex {
  struct document vtbl;     // virtual table of document
  int cp_first;             // first pressed key
};

struct document* document_hex_create(void);
void document_hex_destroy(struct document* base);

void document_hex_reset(struct document* base, struct splitter* splitter);
int document_hex_incremental_update(struct document* base, struct splitter* splitter);
void document_hex_draw(struct document* base, struct screen* screen, struct splitter* splitter);
void document_hex_render(struct document* base, struct screen* screen, struct splitter* splitter, file_offset_t offset, position_t y, const uint8_t* data, size_t data_size, struct document_hex_char* chars);
void document_hex_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y);
void document_hex_cursor_from_point(struct document* base, struct splitter* splitter, int x, int y, file_offset_t* offset);
uint8_t document_hex_value(int cp);
uint8_t document_hex_value_from_string(const char* text, size_t length);
void document_hex_convert(struct document_hex_char* cps, int show_invisibles, int cp_default);

#endif /* #ifndef TIPPSE_DOCUMENT_HEX_H */
