/* Tippse - Splitter - Binary spaced document view */

#include "splitter.h"

struct splitter* splitter_create(int type, int split, struct splitter* side0, struct splitter* side1, const char* name) {
  struct splitter* splitter = malloc(sizeof(struct splitter));
  splitter->type = type;
  splitter->split = split;
  splitter->file = NULL;
  splitter->status = strdup("");
  splitter->name = strdup(name);
  splitter->document_text = NULL;
  splitter->document_hex = NULL;
  splitter->document = NULL;
  splitter->view = document_view_create();
  splitter->cursor_x = -1;
  splitter->cursor_y = -1;

  if (!side0 || !side1) {
    splitter->side[0] = NULL;
    splitter->side[1] = NULL;
    splitter->active = 0;
    splitter->status_inverted = 0;
    splitter->type = type;
    splitter->content = 0;

    splitter->document_text = document_text_create();
    splitter->document_hex = document_hex_create();
    splitter->document = splitter->document_text;
  } else {
    splitter->side[0] = side0;
    splitter->side[1] = side1;
    side0->parent = splitter;
    side1->parent = splitter;
  }

  return splitter;
}

void splitter_destroy(struct splitter* splitter) {
  if (!splitter) {
    return;
  }

  splitter_unassign_document_file(splitter);
  document_view_destroy(splitter->view);
  document_text_destroy(splitter->document_text);
  document_hex_destroy(splitter->document_hex);

  splitter_destroy(splitter->side[0]);
  splitter_destroy(splitter->side[1]);

  free(splitter->name);
  free(splitter->status);
  free(splitter);
}

void splitter_drawchar(struct screen* screen, const struct splitter* splitter, int x, int y, int* codepoints, size_t length, int foreground, int background) {
  if (y<0 || y>=splitter->client_height) {
    return;
  }

  if (x<0 || x>=splitter->client_width) {
    return;
  }

  screen_setchar(screen, splitter->x+x, splitter->y+y, codepoints, length, foreground, background);
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

void splitter_cursor(struct screen* screen, struct splitter* splitter, int x, int y) {
  splitter->cursor_x = x;
  splitter->cursor_y = y;
}

void splitter_scrollbar(struct screen* screen, const struct splitter* splitter) {
  struct document_view* view = splitter->view;

  if (view->show_scrollbar) {
    file_offset_t start = view->scroll_y;
    file_offset_t end = view->scroll_y+splitter->client_height;
    file_offset_t length = view->scroll_y_max;
    if (end==~0) {
      end = length+1;
    }

    int pos_start = (start*(file_offset_t)splitter->client_height)/(length+1);
    int pos_end = (end*(file_offset_t)splitter->client_height)/(length+1);
    int cp = 0x20;
    int y;
    for (y = 0; y<splitter->client_height; y++) {
      if (y>=pos_start && y<=pos_end) {
        splitter_drawchar(screen, splitter, splitter->client_width-1, y, &cp, 1, 17, 231);
      } else {
        splitter_drawchar(screen, splitter, splitter->client_width-1, y, &cp, 1, 17, 102);
      }
    }
    view->show_scrollbar = 0;
  }
}

void splitter_hilight(struct screen* screen, const struct splitter* splitter, int x, int y) {
  if (y<0 || y>=splitter->client_height) {
    return;
  }

  if (x<0 || x>=splitter->client_width) {
    return;
  }

  struct screen_char* c = &screen->buffer[(splitter->y+y)*screen->width+(splitter->x+x)];
  c->foreground = screen_intense_color(c->foreground);
  c->background = screen_intense_color(c->background);
}

void splitter_unassign_document_file(struct splitter* splitter) {
  if (!splitter->file) {
    return;
  }

  struct list_node* view = splitter->file->views->first;
  while (view) {
    if ((struct document_view*)view->object==splitter->view) {
      list_remove(splitter->file->views, view);
      break;
    }

    view = view->next;
  }

  if (!splitter->file->views->first) {
    document_view_clone(splitter->file->view, splitter->view, splitter->file);
  }

  splitter->file = NULL;
}

void splitter_assign_document_file(struct splitter* splitter, struct document_file* file, int content) {
  splitter_unassign_document_file(splitter);

  if (!file) {
    return;
  }

  splitter->file = file;
  document_file_reload_config(file);
  splitter->content = content;
  if (splitter->file->views->first) {
    document_view_clone(splitter->view, (struct document_view*)splitter->file->views->first->object, splitter->file);
  } else {
    document_view_clone(splitter->view, splitter->file->view, splitter->file);
  }

  list_insert(splitter->file->views, NULL, splitter->view);

  (*splitter->document_text->reset)(splitter->document, splitter);
  (*splitter->document_hex->reset)(splitter->document, splitter);

  if (file->binary) {
    splitter->document = splitter->document_hex;
  } else {
    splitter->document = splitter->document_text;
  }
}

void splitter_draw(struct screen* screen, struct splitter* splitter) {
  int xx, yy;
  int cp = 0x20;
  for (yy=0; yy<splitter->height; yy++) {
    for (xx=0; xx<splitter->width; xx++) {
      screen_setchar(screen, splitter->x+xx, splitter->y+yy, &cp, 1, splitter->file->defaults.colors[VISUAL_FLAG_COLOR_TEXT], splitter->file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
    }
  }

  (*splitter->document->draw)(splitter->document, screen, splitter);
}

struct document_file* splitter_first_document(const struct splitter* splitter) {
  while (splitter && !splitter->file) {
    splitter = splitter->side[0];
  }

  return splitter->file;
}

// TODO: hardcoded color values
void splitter_draw_split_horizontal(struct screen* screen, const struct splitter* splitter, int x, int y, int width) {
  struct document_file* file = splitter_first_document(splitter);

  int n;
  int cp = 0x2500;
  for (n = 0; n<width; n++) {
    screen_setchar(screen, x+n, y, &cp, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
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

  screen_setchar(screen, x-1, y, &left, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
  screen_setchar(screen, x+width, y, &right, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
}

void splitter_draw_split_vertical(struct screen* screen, const struct splitter* splitter, int x, int y, int height) {
  struct document_file* file = splitter_first_document(splitter);
  int n;
  int cp = 0x2502;
  for (n = 0; n<height; n++) {
    screen_setchar(screen, x, y+n, &cp, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
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

  screen_setchar(screen, x, y-1, &top, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
  screen_setchar(screen, x, y+height, &bottom, 1, file->defaults.colors[VISUAL_FLAG_COLOR_FRAME], file->defaults.colors[VISUAL_FLAG_COLOR_BACKGROUND]);
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
        splitter_draw_split_vertical(screen, splitter, x+size0, y, height);
      }

      splitter_draw_multiple_recursive(screen, x, y, size0, height, splitter->side[0], incremental);
      splitter_draw_multiple_recursive(screen, x+base0, y, size1, height, splitter->side[1], incremental);
    } else {
      if (size0>0 && size1>0) {
        splitter_draw_split_horizontal(screen, splitter, x, y+size0, width);
      }

      splitter_draw_multiple_recursive(screen, x, y, width, size0, splitter->side[0], incremental);
      splitter_draw_multiple_recursive(screen, x, y+base0, width, size1, splitter->side[1], incremental);
    }
  } else {
    splitter->x = x;
    splitter->y = y;
    splitter->width = width;
    splitter->height = height;
    splitter->client_width = width;
    splitter->client_height = height;
    if (!incremental) {
      splitter_draw(screen, splitter);
    } else {
      (*splitter->document->incremental_update)(splitter->document, splitter);
    }
    if (splitter->active) {
      if (splitter->cursor_x>=0 && splitter->cursor_y>=0 && splitter->cursor_x<splitter->width && splitter->cursor_y<splitter->height) {
        screen_cursor(screen, splitter->cursor_x+x, splitter->cursor_y+y);
      }
    }
  }
}

void splitter_draw_multiple(struct screen* screen, struct splitter* splitters, int incremental) {
  screen_cursor(screen, -1, -1);
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
