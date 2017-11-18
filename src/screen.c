// Tippse - Screen - Display for splitter (a console or a window, depends on OS)

#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "screen.h"

// color names from https://jonasjacek.github.io/colors
struct config_cache screen_color_codes[] = {
  {"background", -2},
  {"foreground", -1},
  {"black", 0},
  {"maroon", 1},
  {"green", 2},
  {"olive", 3},
  {"navy", 4},
  {"purple", 5},
  {"teal", 6},
  {"silver", 7},
  {"grey", 8},
  {"red", 9},
  {"lime", 10},
  {"yellow", 11},
  {"blue", 12},
  {"fuchsia", 13},
  {"aqua", 14},
  {"white", 15},
  {NULL, 0}
};

// Create screen
struct screen* screen_create(void) {
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
  memset(&raw, 0, sizeof(raw));
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);

  write(STDOUT_FILENO, "\x1b[?47h", 6);
  write(STDOUT_FILENO, "\x1b[?25l", 6);
  write(STDOUT_FILENO, "\x1b""7", 2);
  write(STDOUT_FILENO, "\x1b[?2004h", 8);
  write(STDOUT_FILENO, "\x1b[?1002h", 8);
  write(STDOUT_FILENO, "\x1b[?1005h", 8);

  return screen;
}

// Destroy screen
void screen_destroy(struct screen* screen) {
  write(STDOUT_FILENO, "\x1b[?1005l", 8);
  write(STDOUT_FILENO, "\x1b[?1002l", 8);
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

// Initialise screen
void screen_check(struct screen* screen) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  if (screen->width!=w.ws_col || screen->height!=w.ws_row) {
    screen->width = w.ws_col;
    screen->height = w.ws_row;
    free(screen->visible);
    free(screen->buffer);
    screen->buffer = (struct screen_char*)malloc(sizeof(struct screen_char)*(size_t)(screen->width*screen->height));
    screen->visible = (struct screen_char*)malloc(sizeof(struct screen_char)*(size_t)(screen->width*screen->height));

    for (int y = 0; y<screen->height; y++) {
      for (int x = 0; x<screen->width; x++) {
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
  int reversed = (c->background==-1 || c->foreground==-2)?1:0;
  int background = reversed?-1:c->background;
  int foreground = reversed?-1:c->foreground;

  if (foreground!=*foreground_old || background!=*background_old) {
    if (*background_old==-1 || *foreground_old==-2 || *foreground_old==-3) {
      *pos += sprintf(*pos, "\x1b[m");
    }
  }

  if (foreground!=*foreground_old) {
    *foreground_old = foreground;
    if (foreground>=0) {
      *pos += sprintf(*pos, "\x1b[38;5;%dm", foreground);
    } else {
      *pos += sprintf(*pos, "\x1b[39m");
    }
  }

  if (background!=*background_old) {
    *background_old = background;
    if (background>=0) {
      *pos += sprintf(*pos, "\x1b[48;5;%dm", background);
    } else {
      *pos += sprintf(*pos, "\x1b[49m");
    }
  }

  if (reversed) {
    *pos += sprintf(*pos, "\x1b[7m");
  }

  if (c->length==0 || c->codepoints[0]!=-1) {
    size_t copy;
    for (copy = 0; copy<c->length; copy++) {
      *pos += encoding_utf8_encode(NULL, c->codepoints[copy], (uint8_t*)*pos, SIZE_T_MAX);
    }

    if (copy==0) {
      **pos = 0x20;
      *pos += 1;
    }
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

// Set screen title
void screen_title(struct screen* screen, const char* title) {
  if (screen->title_new) {
    free(screen->title_new);
  }

  screen->title_new = strdup(title);
}

// Set screen cursor
void screen_cursor(struct screen* screen, int x, int y) {
  screen->cursor_x = x;
  screen->cursor_y = y;
}

void screen_draw(struct screen* screen) {
  int foreground_old = -3;
  int background_old = -3;
  struct screen_char* c;
  struct screen_char* v;
  char* output = (char*)malloc((size_t)((5+32)*screen->width*screen->height+5));
  char* pos = output;
  if (screen->title_new) {
    if (!screen->title || strcmp(screen->title, screen->title_new)!=0) {
      if (screen->title) {
        free(screen->title);
      }

      screen->title = screen->title_new;
      screen->title_new = NULL;
    }

    if (screen->title_new) {
      free(screen->title_new);
    }

    screen->title_new = NULL;
  }

  pos += sprintf(pos, "\x1b[?25l\x1b[H");
  int w = 0;
  int old = 0;
  int n;
  for (n = 0; n<screen->width*screen->height; n++) {
    c = &screen->buffer[n];
    v = &screen->visible[n];
    int modified = 0;
    if (v->length!=c->length) {
      modified = 1;
    } else {
      for (size_t check = 0; check<c->length; check++) {
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
      for (size_t copy = 0; copy<c->length; copy++) {
        v->codepoints[copy] = c->codepoints[copy];
      }
    }
  }

  screen_draw_update(screen, &pos, old, n, &w, &foreground_old, &background_old);

  if (screen->cursor_x>=0 && screen->cursor_y>=0 && screen->cursor_x<screen->width && screen->cursor_y<screen->height) {
    pos += sprintf(pos, "\x1b[%d;%dH\x1b[?25h", screen->cursor_y+1, screen->cursor_x+1);
  }

  write(STDOUT_FILENO, output, (size_t)(pos-output));
  free(output);
}

void screen_drawtext(const struct screen* screen, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, const char* text, size_t length, int foreground, int background) {
  if (y<clip_y || y>=clip_y+clip_height) {
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

    screen_setchar(screen, x, y, clip_x, clip_y, clip_width, clip_height, &cp, 1, foreground, background);

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

void screen_setchar(const struct screen* screen, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, int* codepoints, size_t length, int foreground, int background) {
  if (y<clip_y || y>=clip_y+clip_height || x<clip_x || x>=clip_x+clip_width) {
    return;
  }

  struct screen_char* c = &screen->buffer[y*screen->width+x];
  for (size_t copy = 0; copy<length; copy++) {
    c->codepoints[copy] = codepoints[copy];
  }

  c->length = length;
  c->foreground = foreground;
  c->background = background;
}
