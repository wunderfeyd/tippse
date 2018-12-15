// Tippse - Screen - Display for splitter (a console or a window, depends on OS)

#include "screen.h"

#include "config.h"
#include "encoding/utf8.h"
#include "encoding/cp850.h"
#include "encoding/ascii.h"
#include "encoding/utf16le.h"
#include "unicode.h"

// Screen ANSI initialization
static const char* screen_ansi_init =
  "\x1b[?47h"    // Switch buffer
  "\x1b[?25l"    // Hide cursor
  "\x1b""7"      // Save cursor position
  "\x1b[2J"      // Clear screen
  "\x1b[?2004h"  // Bracketed past mode (Needed to catch insert key)
  "\x1b[?1002h"  // XTerm mouse mode (fallback/PuTTY)
  "\x1b[?1005h"; // UTF-8 mouse mode

// Screen ANSI restore
static const char* screen_ansi_restore =
  "\x1b[?1005l"  // Disable UTF-8 mouse mode
  "\x1b[?1002l"  // Disable XTerm mouse mode
  "\x1b[?2004l"  // Non bracketed paste mode
  "\x1b""8"      // Restore cursor position
  "\x1b[?25h"    // Show cursor
  "\x1b[?47l"    // Switch back to normal output
  "\x1b[39;49m"; // Restore colors

// color names from https://jonasjacek.github.io/colors
struct config_cache screen_color_codes[] = {
  {"background", -2, NULL},
  {"foreground", -1, NULL},
  {"black", 0, NULL},
  {"maroon", 1, NULL},
  {"green", 2, NULL},
  {"olive", 3, NULL},
  {"navy", 4, NULL},
  {"purple", 5, NULL},
  {"teal", 6, NULL},
  {"silver", 7, NULL},
  {"grey", 8, NULL},
  {"red", 9, NULL},
  {"lime", 10, NULL},
  {"yellow", 11, NULL},
  {"blue", 12, NULL},
  {"fuchsia", 13, NULL},
  {"aqua", 14, NULL},
  {"white", 15, NULL},
  {NULL, 0, NULL}
};

// rgb colors for index colors - TODO: copy this into the config (Tango config Gnome Terminal Linux)
struct screen_rgb rgb_index[] = {
  {0x00, 0x00, 0x00},
  {0xcc, 0x00, 0x00},
  {0x4e, 0x9a, 0x06},
  {0xc4, 0xa0, 0x00},
  {0x34, 0x65, 0xa4},
  {0x75, 0x50, 0x7b},
  {0x06, 0x98, 0x9a},
  {0xd3, 0xd7, 0xcf},
  {0x55, 0x57, 0x53},
  {0xef, 0x29, 0x29},
  {0x8a, 0xe2, 0x34},
  {0xfc, 0xe9, 0x4f},
  {0x72, 0x9f, 0xcf},
  {0xad, 0x7f, 0xa8},
  {0x34, 0xe2, 0xe2},
  {0xee, 0xee, 0xec}
};

// Screen resize signal bound in screen_create
static int screen_resize_counter = 0;
void screen_size_changed(int signal) {
  ++screen_resize_counter;
}

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
  base->resize_check = -1;

#ifdef _WINDOWS
  base->encoding = encoding_utf16le_create();
#else
  base->encoding = encoding_utf8_create();

  tcgetattr(STDIN_FILENO, &base->termios_original);

  struct termios raw;
  memset(&raw, 0, sizeof(raw));
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
  UNUSED(signal(SIGWINCH, screen_size_changed));

  UNUSED(write(STDOUT_FILENO, screen_ansi_init, strlen(screen_ansi_init)));
#endif
  return base;
}

// Destroy screen
void screen_destroy(struct screen* base) {
#ifdef _WINDOWS
#else
  UNUSED(write(STDOUT_FILENO, screen_ansi_restore, strlen(screen_ansi_restore)));
  tcsetattr(STDIN_FILENO, TCSANOW, &base->termios_original);
#endif

  free(base->title);
  free(base->title_new);
  free(base->visible);
  free(base->buffer);
  (*base->encoding->destroy)(base->encoding);
  free(base);
}

// Initialise screen
void screen_check(struct screen* base) {
  base->resize_check = screen_resize_counter;
  int width = 0;
  int height = 0;
#ifdef _WINDOWS
  RECT r;
  GetClientRect(base->window, &r);
  width = (r.right-r.left)/base->font_width;
  height = (r.bottom-r.top)/base->font_height;
#else
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  width = w.ws_col;
  height = w.ws_row;
#endif

  if (base->width!=width || base->height!=height) {
    base->width = width;
    base->height = height;
    free(base->visible);
    free(base->buffer);
    base->buffer = (struct screen_char*)malloc(sizeof(struct screen_char)*(size_t)((unsigned int)base->width*(unsigned int)base->height));
    base->visible = (struct screen_char*)malloc(sizeof(struct screen_char)*(size_t)((unsigned int)base->width*(unsigned int)base->height));

    for (int y = 0; y<base->height; y++) {
      for (int x = 0; x<base->width; x++) {
        struct screen_char* c = &base->buffer[y*base->width+x];
        c->sequence.length = 0;
        c->sequence.cp[0] = 0x20;
        c->foreground = 15;
        c->background = 0;
        c->modified = 1;

        c = &base->visible[y*base->width+x];
        c->sequence.length = 0;
        c->sequence.cp[0] = 0x20;
        c->foreground = 15;
        c->background = 0;
      }
    }
  }
}

// Was the screen resized since last size check?
int screen_resized(struct screen* base) {
  return (screen_resize_counter!=base->resize_check)?1:0;
}

// Update character widths with real terminal display width
void screen_character_width_detect(struct screen* base) {
#ifdef _WINDOWS
#else
  codepoint_t cps = 0x10000;
  char* output = (char*)malloc((size_t)(24*cps));
  char* pos = output;
  pos += sprintf(pos, "\x1b[?25l");
  for (codepoint_t cp = 0; cp<cps; cp++) {
    pos += sprintf(pos, "\x1b[H");
    pos += (*base->encoding->encode)(NULL, cp, (uint8_t*)pos, SIZE_T_MAX);
    pos += sprintf(pos, "\x1b[6n");
  }
  UNUSED(write(STDOUT_FILENO, output, (size_t)(pos-output)));
  free(output);

  int width = 0;
  codepoint_t cp = 0;
  while (cp<cps) {
    uint8_t input[1024];
    int in = read(STDIN_FILENO, &input[0], sizeof(input));
    for (int n = 0; n<in; n++) {
      if (input[n]=='R') {
        unicode_width_adjust(cp, width);
        cp++;
        if (cp==cps) {
          break;
        }
      } else {
        width = (int)(input[n]-'1');
      }
    }
  }
#endif
}

#ifndef _WINDOWS
// Put char on backbuffer
void screen_draw_char(struct screen* base, char** pos, int n, int* w, int* foreground_old, int* background_old) {
  if ((n/base->width)!=((*w)/base->width)) {
    *pos += sprintf(*pos, "\r\n");
  }

  struct screen_char* c = &base->buffer[n];
  int reversed = (c->background==-1 || c->foreground==-2)?1:0;
  int background = reversed?-1:c->background;
  int foreground = c->foreground;

  if (foreground!=*foreground_old || background!=*background_old) {
    if (*background_old==-1 || *foreground_old==-1 || *foreground_old==-2 || *foreground_old==-3) {
      *pos += sprintf(*pos, "\x1b[m");
      *foreground_old = -4;
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

  if (c->sequence.length==0 || c->sequence.cp[0]!=UNICODE_CODEPOINT_BAD) {
    size_t copy;
    if (unicode_joiner(c->sequence.cp[0])) {
      **pos = 'o';
      *pos += 1;
    }

    for (copy = 0; copy<c->sequence.length; copy++) {
      *pos += (*base->encoding->encode)(NULL, c->sequence.cp[copy], (uint8_t*)*pos, SIZE_T_MAX);
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
#endif

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

#ifdef _WINDOWS
// Output screen
void screen_draw(struct screen* base, HDC context, int redraw, int cursor) {
  SelectObject(context, base->font);
  struct screen_char empty;
  empty.sequence.cp[0] = ' ';
  empty.sequence.length = 1;
  empty.background = VISUAL_FLAG_COLOR_BACKGROUND;
  empty.foreground = VISUAL_FLAG_COLOR_TEXT;
  empty.codes[0] = ' ';
  empty.pos = (uint8_t*)&empty.codes[1];

  DWORD background_system = GetSysColor(COLOR_WINDOW);
  DWORD foreground_system = GetSysColor(COLOR_WINDOWTEXT);

  int n = 0;
  for (int y = 0; y<=base->height; y++) {
    for (int x = 0; x<=base->width; x++) {
      struct screen_char* v = (x<base->width && y<base->height)?&base->visible[n]:&empty;
      struct screen_char* c = (x<base->width && y<base->height)?&base->buffer[n++]:&empty;

      int modified = c->modified;
      if (modified) {
        if (v->length!=c->length) {
          modified = 1;
        } else {
          for (size_t check = 0; check<c->sequence.length; check++) {
            if (v->sequence.cp[check]!=c->sequence.cp[check]) {
              modified = 1;
              break;
            }
          }
        }
      }

      if (modified) {
        c->modified = 0;
        v->length = c->length;
        for (size_t copy = 0; copy<c->length; copy++) {
          v->codepoints[copy] = c->codepoints[copy];
        }

        v->pos = (uint8_t*)&v->codes[0];
        for (size_t copy = 0; copy<v->length; copy++) {
          v->pos += (*base->encoding->encode)(NULL, v->codepoints[copy], v->pos, SIZE_T_MAX);
        }
      }

      int reversed = (c->background==-1 || c->foreground==-2)?1:0;
      int index_background = reversed?-1:c->background;
      int index_foreground = c->foreground;

      if (cursor && x==base->cursor_x && y==base->cursor_y) {
        reversed ^= cursor;
      }

      if (reversed) {
        int swap = index_background;
        index_background = index_foreground;
        index_foreground = swap;
      }

      if (modified || v->foreground!=index_foreground || v->background!=index_background || redraw) {
        v->foreground = index_foreground;
        v->background = index_background;

        if (index_foreground<0) {
          SetTextColor(context, reversed?background_system:foreground_system);
        } else {
          struct screen_rgb* foreground = &rgb_index[index_foreground];
          SetTextColor(context, RGB(foreground->r, foreground->g, foreground->b));
        }

        if (index_background<0) {
          SetBkColor(context, reversed?foreground_system:background_system);
        } else {
          struct screen_rgb* background = &rgb_index[index_background];
          SetBkColor(context, RGB(background->r, background->g, background->b));
        }
        TextOutW(context, x*base->font_width, y*base->font_height, &v->codes[0], (v->pos-(uint8_t*)&v->codes[0])/(int)sizeof(wchar_t));
      }
    }
  }
}
#else
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
    int modified = c->modified;
    if (modified) {
      if (v->sequence.length!=c->sequence.length) {
        modified = 1;
      } else {
        for (size_t check = 0; check<c->sequence.length; check++) {
          if (v->sequence.cp[check]!=c->sequence.cp[check]) {
            modified = 1;
            break;
          }
        }
      }
    }

    if (modified || v->foreground!=c->foreground || v->background!=c->background) {
      screen_draw_update(base, &pos, old, n, &w, &foreground_old, &background_old);
      old = n+1;
      screen_draw_char(base, &pos, n, &w, &foreground_old, &background_old);
      c->modified = 0;
      v->sequence.length = c->sequence.length;
      v->foreground = c->foreground;
      v->background = c->background;
      for (size_t copy = 0; copy<c->sequence.length; copy++) {
        v->sequence.cp[copy] = c->sequence.cp[copy];
      }
    }
  }

  screen_draw_update(base, &pos, old, n, &w, &foreground_old, &background_old);

  if (base->cursor_x>=0 && base->cursor_y>=0 && base->cursor_x<base->width && base->cursor_y<base->height) {
    pos += sprintf(pos, "\x1b[%d;%dH\x1b[?25h", base->cursor_y+1, base->cursor_x+1);
  }

  UNUSED(write(STDOUT_FILENO, output, (size_t)(pos-output)));
  free(output);
}
#endif

void screen_update(struct screen* base) {
#ifdef _WINDOWS
  RECT r;
  GetClientRect(base->window, &r);
  InvalidateRect(base->window, &r, FALSE);
#else
  screen_draw(base);
#endif
}

// Put text to a specific location
void screen_drawtext(const struct screen* base, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, const char* text, size_t length, int foreground, int background) {
  if (y<clip_y || y>=clip_y+clip_height) {
    return;
  }

  struct stream stream;
  stream_from_plain(&stream, (uint8_t*)text, length);
  while (length>0 && x<base->width) {
    size_t used;
    codepoint_t cp = encoding_utf8_decode(NULL, &stream, &used);
    if (cp==0) {
      break;
    }

    if (cp==UNICODE_CODEPOINT_BAD) {
      cp = 0xfffd;
    }

    screen_setchar(base, x, y, clip_x, clip_y, clip_width, clip_height, &cp, 1, foreground, background);

    x++;
    length--;
  }
  stream_destroy(&stream);
}

// Return codepoint at screen location
codepoint_t screen_getchar(const struct screen* base, int x, int y) {
  if (y<0 || y>=base->height || x<0 || x>=base->width) {
    return 0;
  }

  return base->buffer[y*base->width+x].sequence.cp[0];
}

// Update screen location with variable codepoint
void screen_setchar(const struct screen* base, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, const codepoint_t* codepoints, const size_t length, int foreground, int background) {
  if (y<clip_y || y>=clip_y+clip_height || x<clip_x || x>=clip_x+clip_width) {
    return;
  }

  int pos = y*base->width+x;
  struct screen_char* c = &base->buffer[pos];
  if (c->sequence.cp[0]==UNICODE_CODEPOINT_BAD && pos>0) {
    struct screen_char* prev = &base->buffer[pos-1];
    if (prev->sequence.cp[0]!=UNICODE_CODEPOINT_BAD && prev->width>1) {
      prev->sequence.length = 1;
      prev->sequence.cp[0] = '?';
      prev->modified = 1;
    }
  }

  if (pos<base->width*base->height && c->width>1) {
    struct screen_char* next = &base->buffer[pos+1];
    if (next->sequence.cp[0]==UNICODE_CODEPOINT_BAD) {
      next->sequence.length = 1;
      next->sequence.cp[0] = '?';
      next->modified = 1;
    }
  }

  c->modified = 1;
  c->width = unicode_width(&codepoints[0], length);
  if (x==base->width-1 && c->width>1) {
    c->sequence.length = 1;
    c->sequence.cp[0] = '?';
    c->foreground = foreground;
    c->background = background;
  } else {
    for (size_t copy = 0; copy<length; copy++) {
      c->sequence.cp[copy] = codepoints[copy];
    }

    c->sequence.length = length;
    c->foreground = foreground;
    c->background = background;
  }
}
