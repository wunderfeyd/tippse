#ifndef TIPPSE_SCREEN_H
#define TIPPSE_SCREEN_H

#include <stdlib.h>
#ifdef _WINDOWS
#include <windows.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#endif
#include "types.h"

// Single display character (usally fits into a terminal character cell)
struct screen_char {
  codepoint_t codepoints[8];  // Codepoints used for encoding the character
  size_t length;              // Number of codepoints used in this structure
  int foreground;             // Foreground color
  int background;             // Background color
  int modified;               // Character was touched
#ifdef _WINDOWS
  wchar_t codes[16];          // transformed output
  uint8_t* pos;               // transformation end
#endif
};

struct screen_rgb {
  uint8_t r;                  // red channel intensity
  uint8_t g;                  // green channel intensity
  uint8_t b;                  // blue channel intensity
};

struct screen {
  int width;                        // Screen size x direction
  int height;                       // Screen size y direction
  int cursor_x;                     // Current cursor placement
  int cursor_y;                     // Current cursor placement
  int resize_check;                 // Resize counter on last check
  struct screen_char* buffer;       // Back buffer to hold the complete screen as it should be presented
  struct screen_char* visible;      // Buffer that holds the screen as it is currently presented
  char* title;                      // Console window title
  char* title_new;                  // Title update
#ifdef _WINDOWS
  HWND window;                      // window to draw to
  HFONT font;                       // font to use for drawing
  int font_width;                   // font width
  int font_height;                  // font height
#else
  struct termios termios_original;  // Termios structure for change detection
#endif
  struct encoding* encoding;        // Terminal encoding
};

void screen_destroy(struct screen* base);
struct screen* screen_create(void);
void screen_check(struct screen* base);
int screen_resized(struct screen* base);
void screen_character_width_detect(struct screen* base);
void screen_draw_char(struct screen* base, char** pos, int n, int* w, int* foreground_old, int* background_old);
void screen_draw_update(struct screen* base, char** pos, int old, int n, int* w, int* foreground_old, int* background_old);
void screen_title(struct screen* base, const char* title);
void screen_cursor(struct screen* base, int x, int y);
#ifdef _WINDOWS
void screen_draw(struct screen* base, HDC context, int redraw, int cursor);
#else
void screen_draw(struct screen* base);
#endif
void screen_update(struct screen* base);
void screen_drawtext(const struct screen* base, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, const char* text, size_t length, int foreground, int background);
codepoint_t screen_getchar(const struct screen* base, int x, int y);
void screen_setchar(const struct screen* base, int x, int y, int clip_x, int clip_y, int clip_width, int clip_height, codepoint_t* codepoints, size_t length, int foreground, int background);

#endif /* #ifndef TIPPSE_SCREEN_H */
