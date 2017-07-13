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
  struct screen_char* c;

  if (y<0 || y>=splitter->client_height) {
    return;
  }

  if (x<0 || x>=splitter->client_width) {
    return;
  }

  if (splitter->active!=-1) {
    c = &screen->buffer[(splitter->y+y+1)*screen->width+(splitter->x+x+1)];
  } else {
    c = &screen->buffer[(splitter->y+y)*screen->width+(splitter->x+x)];
  }

  c->character = cp;
  c->foreground = foreground;
  c->background = background;
}

void splitter_drawtext(struct screen* screen, const struct splitter* splitter, int x, int y, const char* text, size_t length, int foreground, int background) {
  while (*text && length>0) {
    int cp = -1;
    text = utf8_decode(&cp, text, ~0, 1);
    if (cp==-1) {
      cp = 0xfffd;
    }

    splitter_drawchar(screen, splitter, x, y, cp, foreground, background);

    x++;
    length--;
  }
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

  struct screen_char* c = &screen->buffer[(splitter->y+y+1)*screen->width+(splitter->x+x+1)];
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
  for (yy=1; yy<splitter->height-1; yy++) {
    for (xx=1; xx<splitter->width-1; xx++) {
      screen_drawtext(screen, splitter->x+xx, splitter->y+yy, " ", ~0, splitter->foreground, splitter->background);
    }
  }

  document_draw(screen, splitter);

  for (xx=1; xx<splitter->width-1; xx++) {
    screen_drawtext(screen, splitter->x+xx, splitter->y, splitter->active?"═":"─", ~0, splitter->foreground, splitter->background);
    screen_drawtext(screen, splitter->x+xx, splitter->y+splitter->height-1, splitter->active?"═":"─", ~0, splitter->foreground, splitter->background);
  }

  for (yy=1; yy<splitter->height-1; yy++) {
    screen_drawtext(screen, splitter->x, splitter->y+yy, splitter->active?"║":"│", ~0, splitter->foreground, splitter->background);
    screen_drawtext(screen, splitter->x+splitter->width-1, splitter->y+yy, splitter->active?"║":"│", ~0, splitter->foreground, splitter->background);
  }

  screen_drawtext(screen, splitter->x, splitter->y, splitter->active?"╔":"┌", ~0, splitter->foreground, splitter->background);
  screen_drawtext(screen, splitter->x+splitter->width-1, splitter->y, splitter->active?"╗":"┐", ~0, splitter->foreground, splitter->background);
  screen_drawtext(screen, splitter->x, splitter->y+splitter->height-1, splitter->active?"╚":"└", ~0, splitter->foreground, splitter->background);
  screen_drawtext(screen, splitter->x+splitter->width-1, splitter->y+splitter->height-1, splitter->active?"╝":"┘", ~0, splitter->foreground, splitter->background);

  if (splitter->width>6) {
    size_t slen = utf8_strlen(splitter->name);
    if (slen>splitter->width-7) {
      slen = splitter->width-7;
    }

    screen_drawtext(screen, splitter->x+1, splitter->y, splitter->active?"╡ ":"┤ ", ~0, splitter->foreground, splitter->background);
    screen_drawtext(screen, splitter->x+3, splitter->y, splitter->name, slen, splitter->foreground, splitter->background);
    screen_drawtext(screen, splitter->x+3+slen, splitter->y, splitter->active?" ╞":" ├", ~0, splitter->foreground, splitter->background);
  }

  if (splitter->width>4) {
    size_t slen = utf8_strlen(splitter->status);
    if (slen>splitter->width-5) {
      slen = splitter->width-5;
    }

    screen_drawtext(screen, splitter->x+splitter->width-2, splitter->y+splitter->height-1, splitter->active?" ":" ", ~0, splitter->foreground, splitter->background);
    screen_drawtext(screen, splitter->x+splitter->width-2-slen, splitter->y+splitter->height-1, splitter->status, slen, splitter->status_inverted?splitter->background:screen_half_color(splitter->foreground), splitter->status_inverted?splitter->foreground:splitter->background);
    screen_drawtext(screen, splitter->x+splitter->width-3-slen, splitter->y+splitter->height-1, splitter->active?" ":" ", ~0, splitter->foreground, splitter->background);
  }
}

void splitter_draw_multiple_recursive(struct screen* screen, int x, int y, int width, int height, struct splitter* splitter, int incremental) {
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
    
    if (splitter->type&TIPPSE_SPLITTER_HORZ) {
      splitter_draw_multiple_recursive(screen, x, y, size0, height, splitter->side[0], incremental);
      splitter_draw_multiple_recursive(screen, x+size0, y, size1, height, splitter->side[1], incremental);
    } else {
      splitter_draw_multiple_recursive(screen, x, y, width, size0, splitter->side[0], incremental);
      splitter_draw_multiple_recursive(screen, x, y+size0, width, size1, splitter->side[1], incremental);
    }
  } else {
    screen = screen;
    splitter->x = x;
    splitter->y = y;
    splitter->width = width;
    splitter->height = height;
    splitter->client_width = width-2;
    splitter->client_height = height-2;
    if (!incremental) {
      splitter_draw(screen, splitter);
    } else {
      document_incremental_update(splitter);
    }
  }
}

void splitter_draw_multiple(struct screen* screen, struct splitter* splitters, int incremental) {
  splitter_draw_multiple_recursive(screen, 0, 0, screen->width, screen->height, splitters, incremental);
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

