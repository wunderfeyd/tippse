#ifndef TIPPSE_SCREEN_H
#define TIPPSE_SCREEN_H

struct screen_char;
struct screen;

#include <termios.h>

struct screen_char {
  int codepoints[8];
  size_t length;
  int foreground;
  int background;
};

struct screen {
  int width;
  int height;
  int cursor_x;
  int cursor_y;
  struct screen_char* buffer;
  struct screen_char* visible;
  char* title;
  char* title_new;
  struct termios termios_original;
};

#include "encoding/utf8.h"
#include "config.h"

void screen_destroy(struct screen* base);
struct screen* screen_create(void);
void screen_check(struct screen* base);
void screen_draw_char(struct screen* base, char** pos, int n, int* w, int* foreground_old, int* background_old);
void screen_draw_update(struct screen* base, char** pos, int old, int n, int* w, int* foreground_old, int* background_old);
void screen_title(struct screen* base, const char* title);
void screen_cursor(struct screen* base, int x, int y);
void screen_draw(struct screen* base);
void screen_drawtext(const struct screen* base, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, const char* text, size_t length, int foreground, int background);
int screen_getchar(const struct screen* base, int x, int y);
void screen_setchar(const struct screen* base, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, int* codepoints, size_t length, int foreground, int background);

#endif /* #ifndef TIPPSE_SCREEN_H */
