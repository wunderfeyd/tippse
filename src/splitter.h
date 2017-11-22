#ifndef TIPPSE_SPLITTER_H
#define TIPPSE_SPLITTER_H

#include <stdlib.h>

struct document_file;
struct document_view;
struct document;
struct splitter;

#define TIPPSE_SPLITTER_HORZ 1
#define TIPPSE_SPLITTER_VERT 2
#define TIPPSE_SPLITTER_FIXED0 4
#define TIPPSE_SPLITTER_FIXED1 8

struct splitter {
  struct splitter* parent;
  struct splitter* side[2];
  int type;
  int split;

  int x;
  int y;
  int width;
  int height;
  int client_width;
  int client_height;

  int cursor_x;
  int cursor_y;

  int active;
  char* name;
  char* status;
  int status_inverted;

  struct document_file* file;
  struct document_view* view;

  struct document* document;
  struct document* document_text;
  struct document* document_hex;
};

#include "list.h"
#include "screen.h"
#include "document.h"
#include "document_text.h"
#include "document_hex.h"

// TODO: reorder drawing functions and exchange screen & splitter
struct splitter* splitter_create(int type, int split, struct splitter* side0, struct splitter* side1, const char* name);
void splitter_destroy(struct splitter* base);
void splitter_drawchar(const struct splitter* base, struct screen* screen, int x, int y, codepoint_t* codepoints, size_t length, int foreground, int background);
void splitter_drawtext(const struct splitter* base, struct screen* screen, int x, int y, const char* text, size_t length, int foreground, int background);
void splitter_name(struct splitter* base, const char* name);
void splitter_status(struct splitter* base, const char* status, int status_inverted);
void splitter_cursor(struct splitter* base, struct screen* screen, int x, int y);
void splitter_scrollbar(const struct splitter* base, struct screen* screen);
void splitter_hilight(const struct splitter* base, struct screen* screen, int x, int y, int color);
void splitter_unassign_document_file(struct splitter* base);
void splitter_assign_document_file(struct splitter* base, struct document_file* file);
void splitter_exchange_document_file(struct splitter* base, struct document_file* from, struct document_file* to);
void splitter_draw(struct splitter* base, struct screen* screen);
struct document_file* splitter_first_document(const struct splitter* base);
void splitter_draw_split_horizontal(const struct splitter* base, struct screen* screen, int x, int y, int width);
void splitter_draw_split_vertical(const struct splitter* base, struct screen* screen, int x, int y, int height);
void splitter_draw_multiple_recursive(struct splitter* base, struct screen* screen, int x, int y, int width, int height, int incremental);
void splitter_draw_multiple(struct splitter* base, struct screen* screen, int incremental);
struct splitter* splitter_by_coordinate(struct splitter* base, int x, int y);

#endif /* #ifndef TIPPSE_SPLITTER_H */
