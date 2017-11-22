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

// Screen ANSI initialization
const char* screen_ansi_init =
  "\x1b[?47h"    // Switch buffer
  "\x1b[?25l"    // Hide cursor
  "\x1b""7"      // Save cursor position
  "\x1b[?2004h"  // Bracketed past mode (Needed to catch insert key)
  "\x1b[?1002h"  // XTerm mouse mode (fallback/PuTTY)
  "\x1b[?1005h"; // UTF-8 mouse mode

// Screen ANSI restore
const char* screen_ansi_restore =
  "\x1b[?1005l"  // Disable UTF-8 mouse mode
  "\x1b[?1002l"  // Disable XTerm mouse mode
  "\x1b[?2004l"  // Non bracketed paste mode
  "\x1b""8"      // Restore cursor position
  "\x1b[?25h"    // Show cursor
  "\x1b[?47l"    // Switch back to normal output
  "\x1b[39;49m"; // Restore colors

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
  struct screen* base = (struct screen*)malloc(sizeof(struct screen));
  base->width = 0;
  base->height = 0;
  base->buffer = NULL;
  base->visible = NULL;
  base->title = NULL;
  base->title_new = strdup("Tippse");
  base->cursor_x = -1;
  base->cursor_y = -1;

  tcgetattr(STDIN_FILENO, &base->termios_original);

  struct termios raw;
  memset(&raw, 0, sizeof(raw));
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);

  write(STDOUT_FILENO, screen_ansi_init, strlen(screen_ansi_init));

  return base;
}

// Destroy screen
void screen_destroy(struct screen* base) {
  write(STDOUT_FILENO, screen_ansi_restore, strlen(screen_ansi_restore));
  tcsetattr(STDIN_FILENO, TCSANOW, &base->termios_original);

  free(base->title);
  free(base->title_new);
  free(base->visible);
  free(base->buffer);
  free(base);
}

// Initialise screen
void screen_check(struct screen* base) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  if (base->width!=w.ws_col || base->height!=w.ws_row) {
    base->width = w.ws_col;
    base->height = w.ws_row;
    free(base->visible);
    free(base->buffer);
    base->buffer = (struct screen_char*)malloc(sizeof(struct screen_char)*(size_t)(base->width*base->height));
    base->visible = (struct screen_char*)malloc(sizeof(struct screen_char)*(size_t)(base->width*base->height));

    for (int y = 0; y<base->height; y++) {
      for (int x = 0; x<base->width; x++) {
        struct screen_char* c = &base->buffer[y*base->width+x];
        c->length = 0;
        c->codepoints[0] = 0x20;
        c->foreground = 15;
        c->background = 0;

        c = &base->visible[y*base->width+x];
        c->length = 0;
        c->codepoints[0] = 0x20;
        c->foreground = 15;
        c->background = 0;
      }
    }
  }
}

// Put char on backbuffer
void screen_draw_char(struct screen* base, char** pos, int n, int* w, int* foreground_old, int* background_old) {
  if ((n/base->width)!=((*w)/base->width)) {
    *pos += sprintf(*pos, "\r\n");
  }

  struct screen_char* c = &base->buffer[n];
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

// Check if a relocate is shorter than the complete write
void screen_draw_update(struct screen* base, char** pos, int old, int n, int* w, int* foreground_old, int* background_old) {
  if (old>=n) {
    return;
  }

  if (old+6<n) {
    *pos += sprintf(*(char**)pos, "\x1b[%d;%dH", (n/base->width)+1, (n%base->width)+1);
    *w = n;
  } else {
    while (old<n) {
      screen_draw_char(base, pos, old, w, foreground_old, background_old);
      old++;
    }
  }
}

// Set screen title
void screen_title(struct screen* base, const char* title) {
  if (base->title_new) {
    free(base->title_new);
  }

  base->title_new = strdup(title);
}

// Set screen cursor
void screen_cursor(struct screen* base, int x, int y) {
  base->cursor_x = x;
  base->cursor_y = y;
}

// Output screen difference data
void screen_draw(struct screen* base) {
  int foreground_old = -3;
  int background_old = -3;
  struct screen_char* c;
  struct screen_char* v;
  char* output = (char*)malloc((size_t)((5+32)*base->width*base->height+5));
  char* pos = output;
  if (base->title_new) {
    if (!base->title || strcmp(base->title, base->title_new)!=0) {
      if (base->title) {
        free(base->title);
      }

      base->title = base->title_new;
      base->title_new = NULL;
    }

    if (base->title_new) {
      free(base->title_new);
    }

    base->title_new = NULL;
  }

  pos += sprintf(pos, "\x1b[?25l\x1b[H");
  int w = 0;
  int old = 0;
  int n;
  for (n = 0; n<base->width*base->height; n++) {
    c = &base->buffer[n];
    v = &base->visible[n];
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
      screen_draw_update(base, &pos, old, n, &w, &foreground_old, &background_old);
      old = n+1;
      screen_draw_char(base, &pos, n, &w, &foreground_old, &background_old);
      v->length = c->length;
      v->foreground = c->foreground;
      v->background = c->background;
      for (size_t copy = 0; copy<c->length; copy++) {
        v->codepoints[copy] = c->codepoints[copy];
      }
    }
  }

  screen_draw_update(base, &pos, old, n, &w, &foreground_old, &background_old);

  if (base->cursor_x>=0 && base->cursor_y>=0 && base->cursor_x<base->width && base->cursor_y<base->height) {
    pos += sprintf(pos, "\x1b[%d;%dH\x1b[?25h", base->cursor_y+1, base->cursor_x+1);
  }

  write(STDOUT_FILENO, output, (size_t)(pos-output));
  free(output);
}

// Put text to a specific location
void screen_drawtext(const struct screen* base, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, const char* text, size_t length, int foreground, int background) {
  if (y<clip_y || y>=clip_y+clip_height) {
    return;
  }

  struct encoding_stream stream;
  encoding_stream_from_plain(&stream, (uint8_t*)text, length);
  while (length>0 && x<base->width) {
    size_t used;
    codepoint_t cp = encoding_utf8_decode(NULL, &stream, &used);
    if (cp==0) {
      break;
    }

    if (cp==-1) {
      cp = 0xfffd;
    }

    screen_setchar(base, x, y, clip_x, clip_y, clip_width, clip_height, &cp, 1, foreground, background);

    x++;
    length--;
    encoding_stream_forward(&stream, used);
  }
}

// Return codepoint at screen location
codepoint_t screen_getchar(const struct screen* base, int x, int y) {
  if (y<0 || y>=base->height || x<0 || x>=base->width) {
    return 0;
  }

  return base->buffer[y*base->width+x].codepoints[0];
}

// Update screen location with variable codepoint
void screen_setchar(const struct screen* base, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, codepoint_t* codepoints, size_t length, int foreground, int background) {
  if (y<clip_y || y>=clip_y+clip_height || x<clip_x || x>=clip_x+clip_width) {
    return;
  }

  struct screen_char* c = &base->buffer[y*base->width+x];
  for (size_t copy = 0; copy<length; copy++) {
    c->codepoints[copy] = codepoints[copy];
  }

  c->length = length;
  c->foreground = foreground;
  c->background = background;
}
