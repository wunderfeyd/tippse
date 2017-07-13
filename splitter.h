#ifndef __TIPPSE_SPLITTER__
#define __TIPPSE_SPLITTER__

#include <stdlib.h>

struct splitter;

#include "list.h"
#include "screen.h"
#include "document.h"

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

  int foreground;
  int background;
  int active;
  char* name;
  char* status;
  int status_inverted;

  struct document document;
};

struct splitter* splitter_create(int type, int split, struct splitter* side0, struct splitter* side1, int foreground, int background, const char* name);
void splitter_free(struct splitter* splitter);
void splitter_drawchar(struct screen* screen, const struct splitter* splitter, int x, int y, int cp, int foreground, int background);
void splitter_drawtext(struct screen* screen, const struct splitter* splitter, int x, int y, const char* text, size_t length, int foreground, int background);
void splitter_name(struct splitter* splitter, const char* name);
void splitter_status(struct splitter* splitter, const char* status, int status_inverted);
void splitter_cursor(struct screen* screen, const struct splitter* splitter, int x, int y);
void splitter_assign_document_file(struct splitter* splitter, struct document_file* file, int content_document);
void splitter_draw(struct screen* screen, struct splitter* splitter);
void splitter_draw_multiple_recursive(struct screen* screen, int x, int y, int width, int height, struct splitter* splitter, int incremental);
void splitter_draw_multiple(struct screen* screen, struct splitter* splitters, int incremental);
struct splitter* splitter_by_coordinate(struct splitter* splitter, int x, int y);

#endif /* #ifndef __TIPPSE_SPLITTER__ */
