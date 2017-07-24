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

void screen_free(struct screen* screen) {
  if (screen==NULL) {
    return;
  }

  free(screen->visible);
  free(screen->buffer);
  free(screen);
}

struct screen* screen_init() {
  int x, y;
  struct screen_char* c;
  struct winsize w;
  struct screen* screen = (struct screen*)malloc(sizeof(struct screen));

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  screen->width = w.ws_col;
  screen->height = w.ws_row;
  screen->buffer = (struct screen_char*)malloc(sizeof(struct screen_char)*screen->width*screen->height);
  screen->visible = (struct screen_char*)malloc(sizeof(struct screen_char)*screen->width*screen->height);
  screen->title = NULL;
  screen->title_new = strdup("Tippse");

  for (y=0; y<screen->height; y++) {
    for (x=0; x<screen->width; x++) {
      c = &screen->buffer[y*screen->width+x];
      c->character = ' ';
      c->foreground = 15;
      c->background = 0;
    }
  }

  return screen;
}

void screen_draw_char(struct screen* screen, char** pos, int n, int* w, int* foreground_old, int* background_old) {
  if ((n/screen->width)!=((*w)/screen->width)) {
    *pos += sprintf(*pos, "\r\n");
  }

  struct screen_char* c = &screen->buffer[n];
  if (c->foreground!=*foreground_old) {
    *foreground_old = c->foreground;
    //*pos += sprintf(*pos, "\x1b[38;2;%d;%d;%dm", (c->foreground>>16)&255, (c->foreground>>8)&255, (c->foreground>>0)&255);
    *pos += sprintf(*pos, "\x1b[38;5;%dm", c->foreground);
  }

  if (c->background!=*background_old) {
    *background_old = c->background;
    //*pos += sprintf(*pos, "\x1b[48;2;%d;%d;%dm", (c->background>>16)&255, (c->background>>8)&255, (c->background>>0)&255);
    *pos += sprintf(*pos, "\x1b[48;5;%dm", c->background);
  }
  *pos += encoding_utf8_encode(NULL, c->character, (char*)*pos, ~0);
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
  } else {
  }

  return color;
}

void screen_draw(struct screen* screen) {
  int n, w, old;
  int foreground_old = -1;
  int background_old = -1;
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
      pos += sprintf(pos, "\x1b[0;%s\x07", screen->title);
    }

    if (screen->title_new) {
      free(screen->title_new);
    }

    screen->title_new = NULL;
  }

  *pos++ = 0x1b;
  *pos++ = '[';
  *pos++ = 'H';
  w = 0;
  old = 0;
  for (n=0; n<screen->width*screen->height; n++) {
    c = &screen->buffer[n];
    v = &screen->visible[n];
    if (v->character!=c->character || v->foreground!=c->foreground || v->background!=c->background) {
      screen_draw_update(screen, &pos, old, n, &w, &foreground_old, &background_old);
      old = n+1;
      screen_draw_char(screen, &pos, n, &w, &foreground_old, &background_old);
      *v = *c;
    }
  }

  screen_draw_update(screen, &pos, old, n, &w, &foreground_old, &background_old);
  write(STDOUT_FILENO, output, pos-output);
  free(output);
}

void screen_drawtext(const struct screen* screen, int x, int y, const char* text, size_t length, int foreground, int background) {
  if (y<0 || y>=screen->height) {
    return;
  }

  struct encoding_stream stream;
  encoding_stream_from_plain(&stream, text, length);
  while (length>0 && x<screen->width) {
    size_t used;
    int cp = encoding_utf8_decode(NULL, &stream, &used);
    if (cp==0) {
      break;
    }

    if (cp==-1) {
      cp = 0xfffd;
    }

    screen_setchar(screen, x, y, cp, foreground, background);

    x++;
    length--;
    encoding_stream_forward(&stream, used);
  }
}

int screen_getchar(const struct screen* screen, int x, int y) {
  if (y<0 || y>=screen->height || x<0 || x>=screen->width) {
    return 0;
  }

  return screen->buffer[y*screen->width+x].character;
}

void screen_setchar(const struct screen* screen, int x, int y, int cp, int foreground, int background) {
  if (y<0 || y>=screen->height || x<0 || x>=screen->width) {
    return;
  }

  struct screen_char* c = &screen->buffer[y*screen->width+x];
  c->character = cp;
  c->foreground = foreground;
  c->background = background;
}
