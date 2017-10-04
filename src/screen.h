#ifndef __TIPPSE_SCREEN__
#define __TIPPSE_SCREEN__

struct screen_char;
struct screen;

#include <termios.h>
#include "encoding/utf8.h"

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

void screen_destroy(struct screen* screen);
struct screen* screen_create();
void screen_check(struct screen* screen);
void screen_draw_char(struct screen* screen, char** pos, int n, int* w, int* foreground_old, int* background_old);
void screen_draw_update(struct screen* screen, char** pos, int old, int n, int* w, int* foreground_old, int* background_old);
void screen_title(struct screen* screen, const char* title);
void screen_cursor(struct screen* screen, int x, int y);
void screen_draw(struct screen* screen);
void screen_drawtext(const struct screen* screen, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, const char* text, size_t length, int foreground, int background);
int screen_getchar(const struct screen* screen, int x, int y);
void screen_setchar(const struct screen* screen, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, int* codepoints, size_t length, int foreground, int background);

#endif /* #ifndef __TIPPSE_SCREEN__ */
