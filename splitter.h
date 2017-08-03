#ifndef __TIPPSE_SPLITTER__
#define __TIPPSE_SPLITTER__

#include <stdlib.h>

struct splitter;

#include "list.h"
#include "screen.h"
#include "document.h"
#include "document_text.h"
#include "document_raw.h"

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

  int active;
  char* name;
  char* status;
  int status_inverted;
  int content;

  struct document_file* file;
  struct document_view view;

  struct document* document;
  struct document* document_text;
  struct document* document_raw;
};

struct splitter* splitter_create(int type, int split, struct splitter* side0, struct splitter* side1, const char* name);
void splitter_destroy(struct splitter* splitter);
void splitter_drawchar(struct screen* screen, const struct splitter* splitter, int x, int y, int* codepoints, size_t length, int foreground, int background);
void splitter_drawtext(struct screen* screen, const struct splitter* splitter, int x, int y, const char* text, size_t length, int foreground, int background);
void splitter_name(struct splitter* splitter, const char* name);
void splitter_status(struct splitter* splitter, const char* status, int status_inverted);
void splitter_cursor(struct screen* screen, const struct splitter* splitter, int x, int y);
void splitter_unassign_document_file(struct splitter* splitter);
void splitter_assign_document_file(struct splitter* splitter, struct document_file* file, int content);
void splitter_draw(struct screen* screen, struct splitter* splitter);
struct document_file* splitter_first_document(const struct splitter* splitter);
void splitter_draw_split_horizontal(struct screen* screen, const struct splitter* splitter, int x, int y, int width);
void splitter_draw_split_vertical(struct screen* screen, const struct splitter* splitter, int x, int y, int height);
void splitter_draw_multiple_recursive(struct screen* screen, int x, int y, int width, int height, struct splitter* splitter, int incremental);
void splitter_draw_multiple(struct screen* screen, struct splitter* splitters, int incremental);
struct splitter* splitter_by_coordinate(struct splitter* splitter, int x, int y);

#endif /* #ifndef __TIPPSE_SPLITTER__ */
