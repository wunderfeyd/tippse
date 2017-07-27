#ifndef __TIPPSE_SCREEN__
#define __TIPPSE_SCREEN__

struct screen_char;
struct screen;

#include "encoding_utf8.h"

#define TIPPSE_SCREEN_BACKGROUND 17

struct screen_char {
  int character;
  int foreground;
  int background;
};

struct screen {
  int width;
  int height;
  struct screen_char* buffer;
  struct screen_char* visible;
  char* title;
  char* title_new;
};

void screen_free(struct screen* screen);
struct screen* screen_init();
void screen_draw_char(struct screen* screen, char** pos, int n, int* w, int* foreground_old, int* background_old);
void screen_draw_update(struct screen* screen, char** pos, int old, int n, int* w, int* foreground_old, int* background_old);
void screen_title(struct screen* screen, const char* title);
int screen_inverse_color(int color);
int screen_half_inverse_color(int color);
int screen_half_color(int color);
void screen_draw(struct screen* screen);
void screen_drawtext(const struct screen* screen, int x, int y, const char* text, size_t length, int foreground, int background);
int screen_getchar(const struct screen* screen, int x, int y);
void screen_setchar(const struct screen* screen, int x, int y, int cp, int foreground, int background);

#endif /* #ifndef __TIPPSE_SCREEN__ */
