// Tippse - A fast simple editor
// Project is in early stage, use with care at your own risk, anything could happen
// Project may change substantially during this phase

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "screen.h"
#include "encoding/utf8.h"
#include "unicode.h"
#include "editor.h"

struct tippse_ansi_key {
  const char* text;
  int cp;
  int modifier;
};

struct tippse_ansi_key ansi_keys[] = {
  {"\x1b[A", TIPPSE_KEY_UP, 0},
  {"\x1b[B", TIPPSE_KEY_DOWN, 0},
  {"\x1b[C", TIPPSE_KEY_RIGHT, 0},
  {"\x1b[D", TIPPSE_KEY_LEFT, 0},
  {"\x1bOA", TIPPSE_KEY_UP, 0},
  {"\x1bOB", TIPPSE_KEY_DOWN, 0},
  {"\x1bOC", TIPPSE_KEY_RIGHT, 0},
  {"\x1bOD", TIPPSE_KEY_LEFT, 0},
  {"\x1b[1;2A", TIPPSE_KEY_UP, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;2B", TIPPSE_KEY_DOWN, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;2C", TIPPSE_KEY_RIGHT, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;2D", TIPPSE_KEY_LEFT, TIPPSE_KEY_MOD_SHIFT},
  {"\x11", TIPPSE_KEY_CLOSE, 0},
  {"\x10", TIPPSE_KEY_SHOW_INVISIBLES, 0},
  {"\x1b[6~", TIPPSE_KEY_PAGEDOWN, 0},
  {"\x1b[5~", TIPPSE_KEY_PAGEUP, 0},
  {"\x1b[6;2~", TIPPSE_KEY_PAGEDOWN, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[5;2~", TIPPSE_KEY_PAGEUP, TIPPSE_KEY_MOD_SHIFT},
  {"\x7f", TIPPSE_KEY_BACKSPACE, 0},
  {"\x1b[4~", TIPPSE_KEY_LAST, 0},
  {"\x1b[3~", TIPPSE_KEY_DELETE, 0},
  {"\x1b[2~", TIPPSE_KEY_INSERT, 0},
  {"\x1b[1~", TIPPSE_KEY_FIRST, 0},
  {"\x1b[H", TIPPSE_KEY_FIRST, 0},
  {"\x1b[F", TIPPSE_KEY_LAST, 0},
  {"\x1b[1;2H", TIPPSE_KEY_FIRST, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;2F", TIPPSE_KEY_LAST, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;5H", TIPPSE_KEY_HOME, TIPPSE_KEY_MOD_CTRL},
  {"\x1b[1;5F", TIPPSE_KEY_END, TIPPSE_KEY_MOD_CTRL},
  {"\x1b[1;6H", TIPPSE_KEY_HOME, TIPPSE_KEY_MOD_CTRL|TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;6F", TIPPSE_KEY_END, TIPPSE_KEY_MOD_CTRL|TIPPSE_KEY_MOD_SHIFT},
  {"\x1bOQ", TIPPSE_KEY_BOOKMARK, 0},
  {"\x06", TIPPSE_KEY_SEARCH, 0},
  {"\x1bOR", TIPPSE_KEY_SEARCH_NEXT, 0},
  {"\x1b[1;2R", TIPPSE_KEY_SEARCH_PREV, 0},
  {"\x1a", TIPPSE_KEY_UNDO, 0},
  {"\x19", TIPPSE_KEY_REDO, 0},
  {"\x02", TIPPSE_KEY_BROWSER, 0},
  {"\x03", TIPPSE_KEY_COPY, 0},
  {"\x18", TIPPSE_KEY_CUT, 0},
  {"\x1b[3;2~", TIPPSE_KEY_CUT, 0},
  {"\x16", TIPPSE_KEY_PASTE, 0},
  {"\x09", TIPPSE_KEY_TAB, 0},
  {"\x13", TIPPSE_KEY_SAVE, 0},
  {"\x1b[Z", TIPPSE_KEY_UNTAB, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[200~", TIPPSE_KEY_BRACKET_PASTE_START, 0},
  {"\x1b[201~", TIPPSE_KEY_BRACKET_PASTE_STOP, 0},
  {"\x1b[M???", TIPPSE_KEY_TIPPSE_MOUSE_INPUT, 0},
  {"\x0f", TIPPSE_KEY_OPEN, 0},
  {"\x14", TIPPSE_KEY_NEW_VERT_TAB, 0},
  {"\x15", TIPPSE_KEY_VIEW_SWITCH, 0},
  {"\x17", TIPPSE_KEY_WORDWRAP, 0},
  {"\x04", TIPPSE_KEY_DOCUMENTSELECTION, 0},
  {"\r", TIPPSE_KEY_RETURN, 0},
  {"\n", TIPPSE_KEY_RETURN, 0},
  {"\x01", TIPPSE_KEY_SELECT_ALL, 0},
  {NULL, 0, 0}
};

int main(int argc, const char** argv) {
  char* base_path = realpath(".", NULL);

  unicode_init();

  struct screen* screen = screen_create();
  struct editor* editor = editor_create(base_path, screen, argc, argv);

  int bracket_paste = 0;
  int mouse_buttons = 0;
  int mouse_buttons_old = 0;
  int mouse_x = 0;
  int mouse_y = 0;

  unsigned char input_buffer[1024];
  size_t input_pos = 0;
  while (!editor->close) {
    editor_draw(editor);

    ssize_t in = 0;
    while (in==0) {
      fd_set set_read;
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 500*1000;
      FD_ZERO(&set_read);
      FD_SET(STDIN_FILENO, &set_read);

      select(1, &set_read, NULL, NULL, &tv);
      if (FD_ISSET(0, &set_read)) {
        in = read(STDIN_FILENO, &input_buffer[input_pos], sizeof(input_buffer)-input_pos);
        if (in>0) {
          input_pos += (size_t)in;
        }
      } else {
        editor_tick(editor);
      }
    }

    size_t check = 0;
    while (1) {
      size_t used = 0;
      int keep = 0;
      for (size_t pos = 0; ansi_keys[pos].cp!=0; pos++) {
        size_t c;
        for (c = 0; c<input_pos-check; c++) {
          if (ansi_keys[pos].text[c]==0 || (ansi_keys[pos].text[c]!=input_buffer[c+check] && ansi_keys[pos].text[c]!='?')) {
            break;
          }
        }

        int mouse_modifier = 0;
        if (c==input_pos-check || ansi_keys[pos].text[c]==0) {
          keep = 1;
          if (ansi_keys[pos].text[c]==0) {
            if (!bracket_paste) {
              if (ansi_keys[pos].cp==TIPPSE_KEY_TIPPSE_MOUSE_INPUT) {
                if (input_buffer[3]&4) mouse_modifier |= TIPPSE_KEY_MOD_SHIFT;
                if (input_buffer[3]&8) mouse_modifier |= TIPPSE_KEY_MOD_ALT;
                if (input_buffer[3]&16) mouse_modifier |= TIPPSE_KEY_MOD_CTRL;
                int buttons = input_buffer[3] & ~(4+8+16);
                mouse_x = input_buffer[4]-33;
                mouse_y = input_buffer[5]-33;

                mouse_buttons_old = mouse_buttons;
                if (buttons==35) {
                  mouse_buttons &= ~TIPPSE_MOUSE_LBUTTON & ~TIPPSE_MOUSE_RBUTTON & ~TIPPSE_MOUSE_MBUTTON;
                } else if (buttons==32) {
                  mouse_buttons |= TIPPSE_MOUSE_LBUTTON;
                } else if (buttons==33) {
                  mouse_buttons |= TIPPSE_MOUSE_RBUTTON;
                } else if (buttons==34) {
                  mouse_buttons |= TIPPSE_MOUSE_MBUTTON;
                }

                mouse_buttons &= ~TIPPSE_MOUSE_WHEEL_UP & ~TIPPSE_MOUSE_WHEEL_DOWN;
                if (buttons==96) {
                  mouse_buttons |= TIPPSE_MOUSE_WHEEL_UP;
                } else if (buttons==97) {
                  mouse_buttons |= TIPPSE_MOUSE_WHEEL_DOWN;
                }
              }

              editor_keypress(editor, ansi_keys[pos].cp, ansi_keys[pos].modifier|mouse_modifier, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
            }

            if (ansi_keys[pos].cp==TIPPSE_KEY_BRACKET_PASTE_START && !bracket_paste) {
              bracket_paste = 1;
            } else if (ansi_keys[pos].cp==TIPPSE_KEY_BRACKET_PASTE_STOP && bracket_paste) {
              bracket_paste = 0;
            }

            used = c;
            break;
          }
        }
      }

      if (!keep) {
        struct encoding_stream stream;
        encoding_stream_from_plain(&stream, (const uint8_t*)&input_buffer[check], (size_t)((const uint8_t*)&input_buffer[input_pos]-(const uint8_t*)&input_buffer[check]));
        int cp = encoding_utf8_decode(NULL, &stream, &used);
        if (cp==-1) {
          used = 0;
          break;
        }

        if (!bracket_paste) {
          editor_keypress(editor, cp, 0, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
        }
      }

      if (used==0) {
        break;
      }

      check += used;
    }

    memcpy(&input_buffer[0], &input_buffer[input_pos], input_pos-check);
    input_pos -= check;
  }

  editor_destroy(editor);
  screen_destroy(screen);
  free(base_path);

  return 0;
}
