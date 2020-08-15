// Tippse - Splitter - Binary spaced document view

#include "splitter.h"

#include "document.h"
#include "document_text.h"
#include "document_hex.h"
#include "documentfile.h"
#include "documentview.h"
#include "list.h"
#include "misc.h"
#include "screen.h"

// Create splitter
struct splitter* splitter_create(int type, int split, struct splitter* side0, struct splitter* side1, const char* name) {
  struct splitter* base = (struct splitter*)malloc(sizeof(struct splitter));
  base->type = type;
  base->split = split;
  base->file = NULL;
  base->status = strdup("");
  base->name = strdup(name);
  base->document_text = NULL;
  base->document_hex = NULL;
  base->document = NULL;
  base->view = NULL;
  base->cursor_x = -1;
  base->cursor_y = -1;
  base->width = 0;
  base->height = 0;
  base->client_width = 0;
  base->client_height = 0;
  base->x = 0;
  base->y = 0;
  base->timeout = 0;
  base->parent = NULL;
  base->active = 0;
  base->grab = 0;

  if (!side0 || !side1) {
    base->side[0] = NULL;
    base->side[1] = NULL;
    base->type = type;

    base->document_text = document_text_create();
    base->document_hex = document_hex_create();
    base->document = base->document_text;
  } else {
    base->side[0] = side0;
    base->side[1] = side1;
    side0->parent = base;
    side1->parent = base;
  }

  return base;
}

// Destroy splitter
void splitter_destroy(struct splitter* base) {
  if (!base) {
    return;
  }

  splitter_unassign_document_file(base);
  document_text_destroy(base->document_text);
  document_hex_destroy(base->document_hex);

  splitter_destroy(base->side[0]);
  splitter_destroy(base->side[1]);

  free(base->name);
  free(base->status);
  free(base);
}

// Draw character
void splitter_drawchar(const struct splitter* base, struct screen* screen, int x, int y, const codepoint_t* codepoints, const size_t length, int foreground, int background) {
  screen_setchar(screen, base->x+x, base->y+y, base->x, base->y, base->client_width, base->client_height, codepoints, length, foreground, background);
}

// Draw unicode character
void splitter_drawunicode(const struct splitter* base, struct screen* screen, int x, int y, const struct unicode_sequence* sequence, int foreground, int background) {
  screen_setunicode(screen, base->x+x, base->y+y, base->x, base->y, base->client_width, base->client_height, sequence, foreground, background);
}

// Draw text
void splitter_drawtext(const struct splitter* base, struct screen* screen, int x, int y, const char* text, size_t length, int foreground, int background) {
  screen_drawtext(screen, base->x+x, base->y+y, base->x, base->y, base->client_width, base->client_height, text, length, foreground, background);
}

// Set splitter name
void splitter_name(struct splitter* base, const char* name) {
  if (name!=base->name) {
    free(base->name);
    base->name = strdup(name);
  }
}

// Set splitter status
void splitter_status(struct splitter* base, const char* status) {
  if (status!=base->status) {
    free(base->status);
    base->status = strdup(status);
  }
}

// Set cursor
void splitter_cursor(struct splitter* base, struct screen* screen, int x, int y) {
  base->cursor_x = x;
  base->cursor_y = y;
}

// Set scrollbar
void splitter_scrollbar(struct splitter* base, struct screen* screen) {
  struct document_view* view = base->view;

  if ((!view->show_scrollbar && view->scrollbar_timeout==0) || base->client_height<=0) {
    return;
  }

  int64_t tick = tick_count();
  if (view->show_scrollbar) {
    view->scrollbar_timeout = tick+tick_ms(500);
    view->show_scrollbar = 0;
  }

  if (view->scrollbar_timeout<tick) {
    view->scrollbar_timeout = 0;
    return;
  }

  if (view->scrollbar_timeout<base->timeout || base->timeout==0) {
    base->timeout = view->scrollbar_timeout;
  }

  position_t start = view->scroll_y;
  position_t end = view->scroll_y+base->client_height;
  position_t length = view->scroll_y_max+1;
  if (end>length) {
    end = length;
  }

  int size = (int)(((end-start)*(base->client_height-((base->client_height>=length)?0:1)))/length);
  if (size<1) {
    size = 1;
  }

  int pos_start = 0;
  if (start>0) {
    pos_start = (int)((start*(base->client_height-1-size))/(length-(end-start)))+1;
  }

  int pos_end = pos_start+size;
  codepoint_t cp = 0x20;
  for (int y = 0; y<base->client_height; y++) {
    if (y>=pos_start && y<pos_end) {
      splitter_drawchar(base, screen, base->client_width-1, y, &cp, 1, 17, 231);
    } else {
      splitter_drawchar(base, screen, base->client_width-1, y, &cp, 1, 17, 102);
    }
  }
}

void splitter_hilight(const struct splitter* base, struct screen* screen, int x, int y, int color) {
  if (y<0 || y>=base->client_height || x<0 || x>=base->client_width) {
    return;
  }

  struct screen_char* c = &screen->buffer[(base->y+y)*screen->width+(base->x+x)];
  c->foreground = color;
}

void splitter_exchange_color(const struct splitter* base, struct screen* screen, int x, int y, int from, int to) {
  if (y<0 || y>=base->client_height || x<0 || x>=base->client_width) {
    return;
  }

  struct screen_char* c = &screen->buffer[(base->y+y)*screen->width+(base->x+x)];
  if (c->foreground==from) {
    c->foreground = to;
  }

  if (c->background==from) {
    c->background = to;
  }
}

void splitter_unassign_document_file(struct splitter* base) {
  if (!base->file) {
    return;
  }

  struct list_node* view = base->file->views->first;
  while (view) {
    if (*(struct document_view**)list_object(view)==base->view) {
      if (base->file->views->count>1) {
        list_remove(base->file->views, view);
        document_view_destroy(base->view);
      } else {
        base->file->view_inactive = 1;
      }
      break;
    }

    view = view->next;
  }

  base->view = NULL;
  base->file = NULL;
}

// Remove and set new splitter content from document file
void splitter_assign_document_file(struct splitter* base, struct document_file* file) {
  splitter_unassign_document_file(base);

  if (!file || base->file==file) {
    return;
  }

  base->file = file;
  if (base->file->view_inactive) {
    base->view = *(struct document_view**)list_object(base->file->views->first);
    base->file->view_inactive = 0;
  } else {
    if (base->file->views->first) {
      base->view = document_view_clone(*(struct document_view**)list_object(base->file->views->first), base->file);
    } else {
      base->view = document_view_create();
      document_view_reset(base->view, file, 1);
    }
    list_insert(base->file->views, NULL, &base->view);
  }

  (*base->document_text->reset)(base->document, base->view, base->file);
  (*base->document_hex->reset)(base->document, base->view, base->file);

  if (file->binary) {
    base->document = base->document_hex;
  } else {
    base->document = base->document_text;
  }
}

// Replace a document in all splitters with another
void splitter_exchange_document_file(struct splitter* base, struct document_file* from, struct document_file* to) {
  if (base->file==from) {
    splitter_unassign_document_file(base);
    splitter_assign_document_file(base, to);
  }

  if (base->side[0]) {
    splitter_exchange_document_file(base->side[0], from, to);
  }

  if (base->side[1]) {
    splitter_exchange_document_file(base->side[1], from, to);
  }
}

// Empty splitter rectangle and call the document handler
void splitter_draw(struct splitter* base, struct screen* screen) {
  codepoint_t cp = 0x20;
  for (int yy = 0; yy<base->height; yy++) {
    for (int xx = 0; xx<base->width; xx++) {
      screen_setchar(screen, base->x+xx, base->y+yy, base->x, base->y, base->client_width, base->client_height, &cp, 1, base->file->defaults.colors[VISUAL_FLAG_COLOR_TEXT], base->file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
    }
  }

  (*base->document->draw)(base->document, screen, base);
}

// Return one document file structure from the base splitter or its children
struct document_file* splitter_first_document(const struct splitter* base) {
  while (base && !base->file) {
    base = base->side[0];
  }

  return base->file;
}

// Draw horizontal split
void splitter_draw_split_horizontal(const struct splitter* base, struct screen* screen, int x, int y, int width) {
  struct document_file* file = splitter_first_document(base);

  codepoint_t cp = 0x2500;
  for (int n = 0; n<width; n++) {
    screen_setchar(screen, x+n, y, 0, 0, screen->width, screen->height, &cp, 1, base->grab?file->defaults.colors[VISUAL_FLAG_COLOR_MODIFIED]:file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
  }

  codepoint_t left = screen_getchar(screen, x-1, y);
  codepoint_t right = screen_getchar(screen, x+width, y);
  if (left==0x2502) {
    left = 0x251c;
  } else if (left==0x2524) {
    left = 0x253c;
  }

  if (right==0x2502) {
    right = 0x2524;
  } else if (right==0x251c) {
    right = 0x253c;
  }

  screen_setchar(screen, x-1, y, 0, 0, screen->width, screen->height, &left, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
  screen_setchar(screen, x+width, y, 0, 0, screen->width, screen->height, &right, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
}

// Draw vertical split
void splitter_draw_split_vertical(const struct splitter* base, struct screen* screen, int x, int y, int height) {
  struct document_file* file = splitter_first_document(base);
  codepoint_t cp = 0x2502;
  for (int n = 0; n<height; n++) {
    screen_setchar(screen, x, y+n, 0, 0, screen->width, screen->height, &cp, 1, base->grab?file->defaults.colors[VISUAL_FLAG_COLOR_MODIFIED]:file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
  }

  codepoint_t top = screen_getchar(screen, x, y-1);
  codepoint_t bottom = screen_getchar(screen, x, y+height);
  if (top==0x2500) {
    top = 0x252c;
  } else if (top==0x2534) {
    top = 0x253c;
  }

  if (bottom==0x2500) {
    bottom = 0x2534;
  } else if (bottom==0x252c) {
    bottom = 0x253c;
  }

  screen_setchar(screen, x, y-1, 0, 0, screen->width, screen->height, &top, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
  screen_setchar(screen, x, y+height, 0, 0, screen->width, screen->height, &bottom, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
}

// Draw base splitter and all its children
void splitter_draw_multiple_recursive(struct splitter* base, struct screen* screen, int x, int y, int width, int height, int incremental) {
  base->x = x;
  base->y = y;
  base->width = width>0?width:0;
  base->height = height>0?height:0;
  base->client_width = base->width;
  base->client_height = base->height;

  if (base->side[0] && base->side[1]) {
    int size = (base->type&TIPPSE_SPLITTER_HORZ)?width:height;
    int size0 = size;
    int size1 = size;
    if (base->type&TIPPSE_SPLITTER_FIXED0) {
      size0 = base->split<size?base->split:size;
      size1 = size-size0;
    } else if (base->type&TIPPSE_SPLITTER_FIXED1) {
      size1 = base->split<size?base->split:size;
      size0 = size-size1;
    } else {
      size0 = (size*base->split)/100;
      size1 = size-size0;
    }

    if (size1>0 && size0>0) {
      size1--;
    }

    int base0 = size0;
    if (base0>0) {
      base0++;
    }

    if (base->type&TIPPSE_SPLITTER_HORZ) {
      if (size0>0 && size1>0) {
        splitter_draw_split_vertical(base, screen, x+size0, y, height);
      }

      splitter_draw_multiple_recursive(base->side[0], screen, x, y, size0, height, incremental);
      splitter_draw_multiple_recursive(base->side[1], screen, x+base0, y, size1, height, incremental);
    } else {
      if (size0>0 && size1>0) {
        splitter_draw_split_horizontal(base, screen, x, y+size0, width);
      }

      splitter_draw_multiple_recursive(base->side[0], screen, x, y, width, size0, incremental);
      splitter_draw_multiple_recursive(base->side[1], screen, x, y+base0, width, size1, incremental);
    }
    if (!incremental) {
      base->timeout = base->side[0]->timeout;
      if (base->timeout==0 || (base->side[1]->timeout!=0 && base->side[1]->timeout<base->timeout)) {
        base->timeout = base->side[1]->timeout;
      }
    }
  } else {
    if (!incremental) {
      base->timeout = 0;
      splitter_draw(base, screen);
    } else {
      (*base->document->incremental_update)(base->document, base->view, base->file);
    }
    if (base->active) {
      if (base->cursor_x>=0 && base->cursor_y>=0 && base->cursor_x<base->width && base->cursor_y<base->height) {
        screen_cursor(screen, base->cursor_x+x, base->cursor_y+y);
      }
    }
  }
}

// Simplified call to splitter recursive drawing
void splitter_draw_multiple(struct splitter* base, struct screen* screen, int incremental) {
  screen_cursor(screen, -1, -1);
  splitter_draw_multiple_recursive(base, screen, 0, 1, screen->width, screen->height-1, incremental);
}

// Return splitter from absolute screen position
struct splitter* splitter_by_coordinate(struct splitter* base, int x, int y) {
  if (base->side[0] && base->side[1]) {
    struct splitter* found = splitter_by_coordinate(base->side[0], x, y);
    if (!found) {
      found = splitter_by_coordinate(base->side[1], x, y);
    }
    return found;
  }

  if (x>=base->x && y>=base->y && x<base->x+base->width && y<base->y+base->height) {
    return base;
  }

  return NULL;
}

// Find next splitter in list
struct splitter* splitter_next(struct splitter* base, int side) {
  struct splitter* target = base;
  while (target->parent) {
    if (target->parent->side[side]==target) {
      target = target->parent->side[side^1];
      break;
    }
    target = target->parent;
  }

  while (target->side[side]) {
    target = target->side[side];
  }

  return target->side[0]?NULL:target;
}

// Splitter line coordinates for line sorting
struct splitter_coords {
  struct splitter* splitter;  // splitter the structure belongs to
  int x;                      // start or end of line horizontally
  int y;                      // statz or end of line vertically
};

// Recursively add splitter lines
void splitter_grab_load(struct splitter* base, struct list* sort, int side) {
  if (!base) {
    return;
  }

  if (base->side[0] && base->side[1] && base->split!=0 && base->split!=100) {
    struct splitter_coords* coords = (struct splitter_coords*)list_object(list_insert_empty(sort, sort->last));
    coords->splitter = base;
    if (base->type&TIPPSE_SPLITTER_HORZ) {
      coords->x = base->side[1]->x-1;
      coords->y = side?base->y:base->y+base->height;
    } else {
      coords->x = side?base->x:base->x+base->width;
      coords->y = base->side[1]->y-1;
    }

  }

  if (base->split!=0) {
    splitter_grab_load(base->side[0], sort, side);
  }

  if (base->split!=100) {
    splitter_grab_load(base->side[1], sort, side);
  }
}

// Sort splitter lines
int splitter_grab_sort_left(void* left, void* right) {
  struct splitter_coords* split_left = (struct splitter_coords*)left;
  struct splitter_coords* split_right = (struct splitter_coords*)right;

  if (split_left->x<split_right->x) {
    return -1;
  } else if (split_left->x>split_right->x) {
    return 1;
  }

  if (split_left->y<split_right->y) {
    return -1;
  } else if (split_left->y>split_right->y) {
    return 1;
  }

  return 0;
}

// Reverse sort of splitter lines
int splitter_grab_sort_right(void* left, void* right) {
  return -splitter_grab_sort_left(left, right);
}

// Find next parent splitter in list
struct splitter* splitter_grab_next(struct splitter* base, struct splitter* current, int side) {
  struct list sort;
  list_create_inplace(&sort, sizeof(struct splitter_coords));
  splitter_grab_load(base, &sort, side);

  void** sort1 = (void**)malloc(sizeof(void*)*sort.count);
  void** sort2 = (void**)malloc(sizeof(void*)*sort.count);
  struct list_node* it = sort.first;
  for (size_t n = 0; it; n++) {
    sort1[n] = (void*)list_object(it);
    it = it->next;
  }

  struct splitter_coords** sorted = (struct splitter_coords**)merge_sort(sort1, sort2, sort.count, side?splitter_grab_sort_right:splitter_grab_sort_left);

  struct splitter* select = NULL;
  for (size_t n = 0; n<sort.count; n++) {
    if (current==NULL) {
      select = sorted[n]->splitter;
      break;
    }

    if (sorted[n]->splitter==current || (n+1==sort.count && current)) {
      select = sorted[(n+1)%sort.count]->splitter;
      break;
    }
  }

  free(sort1);
  free(sort2);

  while (sort.first) {
    list_remove(&sort, sort.first);
  }

  return select;
}

// Half a splitter and clone
void splitter_split(struct splitter* base) {
  struct splitter* parent = base->parent;
  struct splitter* split = splitter_create(0, 0, NULL, NULL, "Document");

  splitter_assign_document_file(split, base->file);

  struct splitter* splitter = splitter_create(TIPPSE_SPLITTER_HORZ, 50, base, split, "");
  if (parent->side[0]==base) {
    parent->side[0] = splitter;
  } else {
    parent->side[1] = splitter;
  }
  splitter->parent = parent;
}

// Combine two neighbor splitters
struct splitter* splitter_unsplit(struct splitter* base, struct splitter* root) {
  if (base->side[0] || base->side[1]) {
    return base;
  }

  struct splitter* parent = base->parent;
  if (root==parent) {
    return base;
  }

  struct splitter* parentup = parent->parent;
  if (!parentup) {
    return base;
  }

  struct splitter* other = (base!=parent->side[0])?parent->side[0]:parent->side[1];

  if (parentup->side[0]==parent) {
    parentup->side[0] = other;
  } else {
    parentup->side[1] = other;
  }

  other->parent = parentup;

  parent->side[0] = NULL;
  parent->side[1] = NULL;
  splitter_destroy(parent);
  splitter_destroy(base);

  while (other->side[0]) {
    other = other->side[0];
  }

  return other;
}
