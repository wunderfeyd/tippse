/* Tippse - Document Raw View - Cursor and display operations for raw 1d display */

#include "document_raw.h"

struct document* document_raw_create() {
  struct document_raw* document = (struct document_raw*)malloc(sizeof(struct document_raw));
  document->vtbl.draw = document_raw_draw;
  document->vtbl.keypress = document_raw_keypress;
  document->vtbl.incremental_update = document_raw_incremental_update;

  return (struct document*)document;
}

void document_raw_destroy(struct document* base) {
  free(base);
}

// Find next dirty pages and rerender them (background task)
int document_raw_incremental_update(struct document* base, struct splitter* splitter) {
  return 0;
}

// Draw entire visible screen space
void document_raw_draw(struct document* base, struct screen* screen, struct splitter* splitter) {
  if (!screen) {
    return;
  }

  const char* text = "Hexeditor stub needs to be filled...";
  splitter_drawtext(screen, splitter, 0, 0, text, strlen(text), 1, 14);
  splitter_cursor(screen, splitter, 0, 0);
}

void document_raw_keypress(struct document* base, struct splitter* splitter, int cp, int modifier, int button, int button_old, int x, int y) {
}
