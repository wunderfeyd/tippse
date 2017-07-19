/* Tippse - Splitter - Binary spaced document view */

#include "splitter.h"

struct splitter* splitter_create(int type, int split, struct splitter* side0, struct splitter* side1, int foreground, int background, const char* name) {
  struct splitter* splitter = malloc(sizeof(struct splitter));
  splitter->type = type;
  splitter->split = split;
  if (!side0 || !side1) {
    splitter->side[0] = NULL;
    splitter->side[1] = NULL;
    splitter->active = 0;
    splitter->foreground = foreground;
    splitter->background = background;
    splitter->name = strdup(name);
    splitter->status = strdup("");
    splitter->status_inverted = 0;
    splitter->type = type;
    
    splitter->document.file = NULL;
    splitter->document.keep_status = 0;
    document_view_reset(&splitter->document.view);
  } else {
    splitter->side[0] = side0;
    splitter->side[1] = side1;
    side0->parent = splitter;
    side1->parent = splitter;
  }

  return splitter;
}

void splitter_free(struct splitter* splitter) {
  if (!splitter) {
    return;
  }

  free(splitter);
}

void splitter_drawchar(struct screen* screen, const struct splitter* splitter, int x, int y, int cp, int foreground, int background) {
  if (y<0 || y>=splitter->client_height) {
    return;
  }

  if (x<0 || x>=splitter->client_width) {
    return;
  }

  screen_setchar(screen, splitter->x+x, splitter->y+y, cp, foreground, background);
}

void splitter_drawtext(struct screen* screen, const struct splitter* splitter, int x, int y, const char* text, size_t length, int foreground, int background) {
  size_t width = (size_t)(splitter->client_width-x);
  if (width<length) {
    length = width;
  }

  screen_drawtext(screen, splitter->x+x, splitter->y+y, text, length, foreground, background);
}

void splitter_name(struct splitter* splitter, const char* name) {
  if (name!=splitter->name) {
    free(splitter->name);
    splitter->name = strdup(name);
  }
}

void splitter_status(struct splitter* splitter, const char* status, int status_inverted) {
  if (status!=splitter->status) {
    free(splitter->status);
    splitter->status = strdup(status);
  }

  splitter->status_inverted = status_inverted;
}

void splitter_cursor(struct screen* screen, const struct splitter* splitter, int x, int y) {
  if (y<0 || y>=splitter->client_height) {
    return;
  }

  if (x<0 || x>=splitter->client_width) {
    return;
  }

  struct screen_char* c = &screen->buffer[(splitter->y+y)*screen->width+(splitter->x+x)];
  if (splitter->active) {
    c->foreground = screen_half_inverse_color(c->foreground);
    c->background = screen_half_inverse_color(c->background);
  } else {
    c->foreground = screen_half_color(screen_half_inverse_color(c->foreground));
    c->background = screen_half_color(screen_half_inverse_color(c->background));
  }
}

void splitter_assign_document_file(struct splitter* splitter, struct document_file* file, int content_document) {
  splitter->document.file = file;
  splitter->document.content_document = content_document;
  list_insert(splitter->document.file->views, NULL, &splitter->document.view);
}

void splitter_draw(struct screen* screen, struct splitter* splitter) {
  int xx, yy;
  for (yy=0; yy<splitter->height; yy++) {
    for (xx=0; xx<splitter->width; xx++) {
      screen_setchar(screen, splitter->x+xx, splitter->y+yy, 0x20, splitter->foreground, splitter->background);
    }
  }

  document_draw(screen, splitter);
}

// TODO: hardcoded color values
void splitter_draw_split_horizontal(struct screen* screen, int x, int y, int width) {
  int n;
  for (n = 0; n<width; n++) {
    screen_setchar(screen, x+n, y, 0x2500, 231, 17);
  }

  int left = screen_getchar(screen, x-1, y);
  int right = screen_getchar(screen, x+width, y);
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

  screen_setchar(screen, x-1, y, left, 231, 17);
  screen_setchar(screen, x+width, y, right, 231, 17);
}

void splitter_draw_split_vertical(struct screen* screen, int x, int y, int height) {
  int n;
  for (n = 0; n<height; n++) {
    screen_setchar(screen, x, y+n, 0x2502, 231, 17);
  }

  int top = screen_getchar(screen, x, y-1);
  int bottom = screen_getchar(screen, x, y+height);
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

  screen_setchar(screen, x, y-1, top, 231, 17);
  screen_setchar(screen, x, y+height, bottom, 231, 17);
}

void splitter_draw_multiple_recursive(struct screen* screen, int x, int y, int width, int height, struct splitter* splitter, int incremental) {
  if (width<=0 && height<=0) {
    return;
  }

  if (splitter->side[0] && splitter->side[1]) {
    int size = (splitter->type&TIPPSE_SPLITTER_HORZ)?width:height;
    int size0 = size;
    int size1 = size;
    if (splitter->type&TIPPSE_SPLITTER_FIXED0) {
      size0 = splitter->split;
      size1 = size-size0;
    } else if (splitter->type&TIPPSE_SPLITTER_FIXED1) {
      size1 = splitter->split;
      size0 = size-size1;
    } else {
      size0 = (size*splitter->split)/100;
      size1 = size-size0;
    }

    if (size1>0 && size0>0) {
      size1--;
    }

    int base0 = size0;
    if (base0>0) {
      base0++;
    }

    if (splitter->type&TIPPSE_SPLITTER_HORZ) {
      if (size0>0 && size1>0) {
        splitter_draw_split_vertical(screen, x+size0, y, height);
      }

      splitter_draw_multiple_recursive(screen, x, y, size0, height, splitter->side[0], incremental);
      splitter_draw_multiple_recursive(screen, x+base0, y, size1, height, splitter->side[1], incremental);
    } else {
      if (size0>0 && size1>0) {
        splitter_draw_split_horizontal(screen, x, y+size0, width);
      }

      splitter_draw_multiple_recursive(screen, x, y, width, size0, splitter->side[0], incremental);
      splitter_draw_multiple_recursive(screen, x, y+base0, width, size1, splitter->side[1], incremental);
    }
  } else {
    screen = screen;
    splitter->x = x;
    splitter->y = y;
    splitter->width = width;
    splitter->height = height;
    splitter->client_width = width;
    splitter->client_height = height;
    if (!incremental) {
      splitter_draw(screen, splitter);
    } else {
      document_incremental_update(splitter);
    }
  }
}

void splitter_draw_multiple(struct screen* screen, struct splitter* splitters, int incremental) {
  splitter_draw_multiple_recursive(screen, 0, 1, screen->width, screen->height-1, splitters, incremental);
}

struct splitter* splitter_by_coordinate(struct splitter* splitter, int x, int y) {
  if (splitter->side[0] && splitter->side[1]) {
    struct splitter* found = splitter_by_coordinate(splitter->side[0], x, y);
    if (!found) {
      found = splitter_by_coordinate(splitter->side[1], x, y);
    }
    return found;
  }

  if (x>=splitter->x && y>=splitter->y && x<splitter->x+splitter->width && y<splitter->y+splitter->height) {
    return splitter;
  }
  
  return NULL;
}

