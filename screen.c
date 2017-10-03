/* Tippse - Screen - Display for splitter (a console or a window, depends on OS) */

#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "screen.h"

struct screen* screen_create() {
  struct screen* screen = (struct screen*)malloc(sizeof(struct screen));
  screen->width = 0;
  screen->height = 0;
  screen->buffer = NULL;
  screen->visible = NULL;
  screen->title = NULL;
  screen->title_new = strdup("Tippse");
  screen->cursor_x = -1;
  screen->cursor_y = -1;

  tcgetattr(STDIN_FILENO, &screen->termios_original);

  struct termios raw;
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);

  write(STDOUT_FILENO, "\x1b[?47h", 6);
  write(STDOUT_FILENO, "\x1b[?25l", 6);
  write(STDOUT_FILENO, "\x1b""7", 2);
  write(STDOUT_FILENO, "\x1b[?2004h", 8);
  write(STDOUT_FILENO, "\x1b[?1003h", 8);
  write(STDOUT_FILENO, "\x1b[?1005h", 8);

  return screen;
}

void screen_destroy(struct screen* screen) {
  if (screen==NULL) {
    return;
  }

  write(STDOUT_FILENO, "\x1b[?1005l", 8);
  write(STDOUT_FILENO, "\x1b[?1003l", 8);
  write(STDOUT_FILENO, "\x1b[?2004l", 8);
  write(STDOUT_FILENO, "\x1b""8", 2);
  write(STDOUT_FILENO, "\x1b[?25h", 6);
  write(STDOUT_FILENO, "\x1b[?47l", 6);
  write(STDOUT_FILENO, "\x1b[39;49m", 8);
  tcsetattr(STDIN_FILENO, TCSANOW, &screen->termios_original);

  free(screen->title);
  free(screen->title_new);
  free(screen->visible);
  free(screen->buffer);
  free(screen);
}

void screen_check(struct screen* screen) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  if (screen->width!=w.ws_col || screen->height!=w.ws_row) {
    screen->width = w.ws_col;
    screen->height = w.ws_row;
    free(screen->visible);
    free(screen->buffer);
    screen->buffer = (struct screen_char*)malloc(sizeof(struct screen_char)*screen->width*screen->height);
    screen->visible = (struct screen_char*)malloc(sizeof(struct screen_char)*screen->width*screen->height);

    int x, y;
    for (y=0; y<screen->height; y++) {
      for (x=0; x<screen->width; x++) {
        struct screen_char* c = &screen->buffer[y*screen->width+x];
        c->length = 0;
        c->codepoints[0] = 0x20;
        c->foreground = 15;
        c->background = 0;

        c = &screen->visible[y*screen->width+x];
        c->length = 0;
        c->codepoints[0] = 0x20;
        c->foreground = 15;
        c->background = 0;
      }
    }
  }
}

void screen_draw_char(struct screen* screen, char** pos, int n, int* w, int* foreground_old, int* background_old) {
  if ((n/screen->width)!=((*w)/screen->width)) {
    *pos += sprintf(*pos, "\r\n");
  }

  struct screen_char* c = &screen->buffer[n];
  if (c->foreground!=*foreground_old) {
    *foreground_old = c->foreground;
    if (c->foreground>=0) {
      *pos += sprintf(*pos, "\x1b[38;5;%dm", c->foreground);
    } else {
      *pos += sprintf(*pos, "\x1b[39m");
    }
  }

  if (c->background!=*background_old) {
    *background_old = c->background;
    if (c->background>=0) {
      *pos += sprintf(*pos, "\x1b[48;5;%dm", c->background);
    } else {
      *pos += sprintf(*pos, "\x1b[49m");
    }
  }

  size_t copy;
  for (copy = 0; copy<c->length; copy++) {
    *pos += encoding_utf8_encode(NULL, c->codepoints[copy], (uint8_t*)*pos, ~0);
  }

  if (copy==0) {
    **pos = 0x20;
    *pos += 1;
  }

  *w = n;
}

void screen_draw_update(struct screen* screen, char** pos, int old, int n, int* w, int* foreground_old, int* background_old) {
  if (old>=n) {
    return;
  }

  if (old+6<n) {
    *pos += sprintf(*(char**)pos, "\x1b[%d;%dH", (n/screen->width)+1, (n%screen->width)+1);
    *w = n;
  } else {
    while (old<n) {
      screen_draw_char(screen, pos, old, w, foreground_old, background_old);
      old++;
    }
  }
}

void screen_title(struct screen* screen, const char* title) {
  if (screen->title_new) {
    free(screen->title_new);
  }

  screen->title_new = strdup(title);
}

int screen_inverse_color(int color) {
  if (color<16) {
    color = 15-color;
  } else if(color<232) {
    color -= 16;
    int r = color%6;
    int g = (color/6)%6;
    int b = (color/36)%6;
    color = (5-r)+((5-g)*6)+((5-b)*36)+16;
  } else {
    color = (24-(color-232))+232;
  }

  return color;
}

int screen_half_inverse_color(int color) {
  if (color<16) {
    color = 15-color;
  } else if(color<232) {
    color -= 16;
    int r = color%6;
    int g = (color/6)%6;
    int b = (color/36)%6;
    color = (5-r)+((g)*6)+((5-b)*36)+16;
  } else {
    color = (24-(color-232))+232;
  }

  return color;
}

int screen_half_color(int color) {
  if (color<16) {
  } else if(color<232) {
    color -= 16;
    int r = color%6;
    int g = (color/6)%6;
    int b = (color/36)%6;
    color = (r/2)+((g/2)*6)+((b/2)*36)+16;
  }

  return color;
}

int screen_intense_color(int color) {
  if (color<16) {
  } else if(color<232) {
    color -= 16;
    int r = color%6;
    int g = (color/6)%6;
    int b = (color/36)%6;
    color = b+(r*6)+(g*36)+16;
  }

  return color;
}

void screen_cursor(struct screen* screen, int x, int y) {
  screen->cursor_x = x;
  screen->cursor_y = y;
}

void screen_draw(struct screen* screen) {
  int n, w, old;
  int foreground_old = -2;
  int background_old = -2;
  struct screen_char* c;
  struct screen_char* v;
  char* output = (char*)malloc((5+32)*screen->width*screen->height+5);
  char* pos = output;
  if (screen->title_new) {
    if (!screen->title || strcmp(screen->title, screen->title_new)!=0) {
      if (screen->title) {
        free(screen->title);
      }

      screen->title = screen->title_new;
      screen->title_new = NULL;
      // pos += sprintf(pos, "\x1b[0;%s\x07", screen->title);
    }

    if (screen->title_new) {
      free(screen->title_new);
    }

    screen->title_new = NULL;
  }

  pos += sprintf(pos, "\x1b[?25l\x1b[H");
  w = 0;
  old = 0;
  for (n=0; n<screen->width*screen->height; n++) {
    c = &screen->buffer[n];
    v = &screen->visible[n];
    int modified = 0;
    if (v->length!=c->length) {
      modified = 1;
    } else {
      size_t check;
      for (check = 0; check<c->length; check++) {
        if (v->codepoints[check]!=c->codepoints[check]) {
          modified = 1;
          break;
        }
      }
    }

    if (modified || v->foreground!=c->foreground || v->background!=c->background) {
      screen_draw_update(screen, &pos, old, n, &w, &foreground_old, &background_old);
      old = n+1;
      screen_draw_char(screen, &pos, n, &w, &foreground_old, &background_old);
      v->length = c->length;
      v->foreground = c->foreground;
      v->background = c->background;
      size_t copy;
      for (copy = 0; copy<c->length; copy++) {
        v->codepoints[copy] = c->codepoints[copy];
      }
    }
  }

  screen_draw_update(screen, &pos, old, n, &w, &foreground_old, &background_old);

  if (screen->cursor_x>=0 && screen->cursor_y>=0 && screen->cursor_x<screen->width && screen->cursor_y<screen->height) {
    pos += sprintf(pos, "\x1b[%d;%dH\x1b[?25h", screen->cursor_y+1, screen->cursor_x+1);
  }

  write(STDOUT_FILENO, output, pos-output);
  free(output);
}

void screen_drawtext(const struct screen* screen, int x, int y, const char* text, size_t length, int foreground, int background) {
  if (y<0 || y>=screen->height) {
    return;
  }

  struct encoding_stream stream;
  encoding_stream_from_plain(&stream, (uint8_t*)text, length);
  while (length>0 && x<screen->width) {
    size_t used;
    int cp = encoding_utf8_decode(NULL, &stream, &used);
    if (cp==0) {
      break;
    }

    if (cp==-1) {
      cp = 0xfffd;
    }

    screen_setchar(screen, x, y, &cp, 1, foreground, background);

    x++;
    length--;
    encoding_stream_forward(&stream, used);
  }
}

int screen_getchar(const struct screen* screen, int x, int y) {
  if (y<0 || y>=screen->height || x<0 || x>=screen->width) {
    return 0;
  }

  return screen->buffer[y*screen->width+x].codepoints[0];
}

void screen_setchar(const struct screen* screen, int x, int y, int* codepoints, size_t length, int foreground, int background) {
  if (y<0 || y>=screen->height || x<0 || x>=screen->width) {
    return;
  }

  struct screen_char* c = &screen->buffer[y*screen->width+x];
  size_t copy;
  for (copy = 0; copy<length; copy++) {
    c->codepoints[copy] = codepoints[copy];
  }

  c->length = length;
  c->foreground = foreground;
  c->background = background;
}
