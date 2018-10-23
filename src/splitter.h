#ifndef TIPPSE_SPLITTER_H
#define TIPPSE_SPLITTER_H

#include <stdlib.h>
#include "types.h"

// Splitter flags
#define TIPPSE_SPLITTER_HORZ 1
#define TIPPSE_SPLITTER_VERT 2
#define TIPPSE_SPLITTER_FIXED0 4
#define TIPPSE_SPLITTER_FIXED1 8

struct splitter {
  struct splitter* parent;    // Parent splitter object
  struct splitter* side[2];   // Left/Right or Top/Bottom sides
  int type;                   // Combined splitter flags
  int split;                  // Split position

  int x;                      // On screen x position
  int y;                      // On screen y position
  int width;                  // On screen x size
  int height;                 // On screen y size
  int client_width;           // Width of the document conrent (without splitter head etc.)
  int client_height;          // Height of the document conrent (without splitter head etc.)

  int cursor_x;               // Cursor placement in client area
  int cursor_y;               // Cursor placement in client area

  int64_t timeout;            // Next timeout callback

  int active;                 // Focused splitter?
  char* name;                 // Name
  char* status;               // Status text

  struct document_file* file; // Assigned document file
  struct document_view* view; // Current view object

  struct document* document;  // Current document handler
  struct document* document_text; // Text editor handler
  struct document* document_hex;  // Hex editor handler
};

// TODO: reorder drawing functions and exchange screen & splitter
struct splitter* splitter_create(int type, int split, struct splitter* side0, struct splitter* side1, const char* name);
void splitter_destroy(struct splitter* base);
void splitter_drawchar(const struct splitter* base, struct screen* screen, int x, int y, codepoint_t* codepoints, size_t length, int foreground, int background);
void splitter_drawtext(const struct splitter* base, struct screen* screen, int x, int y, const char* text, size_t length, int foreground, int background);
void splitter_name(struct splitter* base, const char* name);
void splitter_status(struct splitter* base, const char* status);
void splitter_cursor(struct splitter* base, struct screen* screen, int x, int y);
void splitter_scrollbar(struct splitter* base, struct screen* screen);
void splitter_hilight(const struct splitter* base, struct screen* screen, int x, int y, int color);
void splitter_exchange_color(const struct splitter* base, struct screen* screen, int x, int y, int from, int to);
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
struct splitter* splitter_next(struct splitter* base, int side);
void splitter_split(struct splitter* base);
struct splitter* splitter_unsplit(struct splitter* base, struct splitter* root);

#endif /* #ifndef TIPPSE_SPLITTER_H */
