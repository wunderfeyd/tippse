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
#include "search.h"

struct tippse_ansi_key {
  const char* text; // text in input stream
  int key;          // associated key
  codepoint_t cp;   // codepoint if needed
};

// Ansi command table
// ? = single byte
// m = modifier param
struct tippse_ansi_key ansi_keys[] = {
  {"\x00", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '^'},
  {"\x01", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'a'},
  {"\x02", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'b'},
  {"\x03", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'c'},
  {"\x04", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'd'},
  {"\x05", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'e'},
  {"\x06", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'f'},
  {"\x07", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'g'},
  {"\x08", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'h'},
  {"\t", TIPPSE_KEY_TAB, 0},
  {"\n", TIPPSE_KEY_RETURN, 0},
  {"\x0b", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'k'},
  {"\x0c", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'l'},
  {"\r", TIPPSE_KEY_RETURN, 0},
  {"\x0e", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'n'},
  {"\x0f", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'o'},
  {"\x10", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'p'},
  {"\x11", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'q'},
  {"\x12", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'r'},
  {"\x13", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 's'},
  {"\x14", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 't'},
  {"\x15", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'u'},
  {"\x16", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'v'},
  {"\x17", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'w'},
  {"\x18", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'x'},
  {"\x19", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'y'},
  {"\x1a", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'z'},
  {"\x1b[A", TIPPSE_KEY_UP, 0},
  {"\x1b[B", TIPPSE_KEY_DOWN, 0},
  {"\x1b[C", TIPPSE_KEY_RIGHT, 0},
  {"\x1b[D", TIPPSE_KEY_LEFT, 0},
  {"\x1b[F", TIPPSE_KEY_LAST, 0},
  {"\x1b[H", TIPPSE_KEY_FIRST, 0},
  {"\x1b[1;mA", TIPPSE_KEY_UP, 0},
  {"\x1b[1;mB", TIPPSE_KEY_DOWN, 0},
  {"\x1b[1;mC", TIPPSE_KEY_RIGHT, 0},
  {"\x1b[1;mD", TIPPSE_KEY_LEFT, 0},
  {"\x1b[1;mF", TIPPSE_KEY_LAST, 0},
  {"\x1b[1;mH", TIPPSE_KEY_FIRST, 0},
  {"\x1b[1;mP", TIPPSE_KEY_F1, 0},
  {"\x1b[1;mQ", TIPPSE_KEY_F2, 0},
  {"\x1b[1;mR", TIPPSE_KEY_F3, 0},
  {"\x1b[1;mS", TIPPSE_KEY_F4, 0},
  {"\x1b[200~", TIPPSE_KEY_BRACKET_PASTE_START, 0},
  {"\x1b[201~", TIPPSE_KEY_BRACKET_PASTE_STOP, 0},
  {"\x1b[1;m~", TIPPSE_KEY_FIRST, 0},
  {"\x1b[15;m~", TIPPSE_KEY_F5, 0},
  {"\x1b[17;m~", TIPPSE_KEY_F6, 0},
  {"\x1b[18;m~", TIPPSE_KEY_F7, 0},
  {"\x1b[19;m~", TIPPSE_KEY_F8, 0},
  {"\x1b[2;m~", TIPPSE_KEY_INSERT, 0},
  {"\x1b[3;m~", TIPPSE_KEY_DELETE, 0},
  {"\x1b[4;m~", TIPPSE_KEY_LAST, 0},
  {"\x1b[5;m~", TIPPSE_KEY_PAGEUP, 0},
  {"\x1b[6;m~", TIPPSE_KEY_PAGEDOWN, 0},
  {"\x1b[1~", TIPPSE_KEY_FIRST, 0},
  {"\x1b[11~", TIPPSE_KEY_F1, 0},
  {"\x1b[12~", TIPPSE_KEY_F2, 0},
  {"\x1b[13~", TIPPSE_KEY_F3, 0},
  {"\x1b[14~", TIPPSE_KEY_F4, 0},
  {"\x1b[15~", TIPPSE_KEY_F5, 0},
  {"\x1b[17~", TIPPSE_KEY_F6, 0},
  {"\x1b[18~", TIPPSE_KEY_F7, 0},
  {"\x1b[19~", TIPPSE_KEY_F8, 0},
  {"\x1b[2~", TIPPSE_KEY_INSERT, 0},
  {"\x1b[20~", TIPPSE_KEY_F9, 0},
  {"\x1b[21~", TIPPSE_KEY_F10, 0},
  {"\x1b[23~", TIPPSE_KEY_F1|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[24~", TIPPSE_KEY_F2|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[25~", TIPPSE_KEY_F3|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[26~", TIPPSE_KEY_F4|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[28~", TIPPSE_KEY_F5|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[29~", TIPPSE_KEY_F6|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[3~", TIPPSE_KEY_DELETE, 0},
  {"\x1b[31~", TIPPSE_KEY_F7|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[32~", TIPPSE_KEY_F8|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[33~", TIPPSE_KEY_F9|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[34~", TIPPSE_KEY_F10|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[4~", TIPPSE_KEY_LAST, 0},
  {"\x1b[5~", TIPPSE_KEY_PAGEUP, 0},
  {"\x1b[6~", TIPPSE_KEY_PAGEDOWN, 0},
  {"\x1b[M???", TIPPSE_KEY_MOUSE, 0},
  {"\x1b[Z", TIPPSE_KEY_TAB|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1bOA", TIPPSE_KEY_UP, 0},
  {"\x1bOB", TIPPSE_KEY_DOWN, 0},
  {"\x1bOC", TIPPSE_KEY_RIGHT, 0},
  {"\x1bOD", TIPPSE_KEY_LEFT, 0},
  {"\x1bOP", TIPPSE_KEY_F1, 0},
  {"\x1bOQ", TIPPSE_KEY_F2, 0},
  {"\x1bOR", TIPPSE_KEY_F3, 0},
  {"\x1bOS", TIPPSE_KEY_F4, 0},
  {"\x1c", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '#'},
  {"\x1d", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '~'},
  {"\x1e", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '^'},
  {"\x1f", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '\\'},
  {"\x7f", TIPPSE_KEY_BACKSPACE, 0},
  {NULL, 0, 0}
};

// Local document crawler
void tippse_append_inputs(struct editor* editor, fd_set* set_read, fd_set* set_write, int* nfds);
int tippse_check_inputs(struct editor* editor, fd_set* set_read, fd_set* set_write, int* nfds);

// Helper for crawling document pipes
void tippse_append_inputs(struct editor* editor, fd_set* set_read, fd_set* set_write, int* nfds) {
  struct list_node* docs = editor->documents->first;
  while (docs) {
    struct document_file* file = *(struct document_file**)list_object(docs);
    if (file->pipefd[0]!=-1) {
      FD_SET(file->pipefd[0], set_read);
      if (file->pipefd[0]>*nfds) {
        *nfds = file->pipefd[0];
      }
    }

    docs = docs->next;
  }
}

int tippse_check_inputs(struct editor* editor, fd_set* set_read, fd_set* set_write, int* nfds) {
  int input = 0;
  struct list_node* docs = editor->documents->first;
  while (docs) {
    struct document_file* file = *(struct document_file**)list_object(docs);
    if (file->pipefd[0]!=-1) {
      if (FD_ISSET(file->pipefd[0], set_read)) {
        input = 1;

        uint8_t buffer[1024];
        ssize_t length = read(file->pipefd[0], &buffer[0], 1024);
        if (length>0) {
          document_file_fill_pipe(file, &buffer[0], (size_t)length);
        } else if (length==0) {
          document_file_close_pipe(file);
        }
      }
    }

    docs = docs->next;
  }

  return input;
}

int main(int argc, const char** argv) {
  char* base_path = realpath(".", NULL);

  unicode_init();

  if (0) {
    search_test();
    unicode_free();
    free(base_path);
    exit(0);
  }

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
    int stop = 0;
    int64_t start = 0;
    while (in==0) {
      int64_t tick = tick_count();
      if (!stop) {
        start = tick;
      }

      int64_t left = 100000-(tick-start);
      if (left<=0) {
        break;
      }

      fd_set set_read;
      fd_set set_write;
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = left;
      FD_ZERO(&set_read);
      FD_ZERO(&set_write);
      int nfds = STDIN_FILENO;
      FD_SET(STDIN_FILENO, &set_read);

      tippse_append_inputs(editor, &set_read, &set_write, &nfds);
      select(nfds+1, &set_read, NULL, NULL, &tv);
      if (FD_ISSET(STDIN_FILENO, &set_read)) {
        in = read(STDIN_FILENO, &input_buffer[input_pos], sizeof(input_buffer)-input_pos);
        if (in>0) {
          input_pos += (size_t)in;
        }
      }

      editor_tick(editor);

      if (tippse_check_inputs(editor, &set_read, &set_write, &nfds)) {
        stop = 1;
      }
    }

    size_t check = 0;
    while (1) {
      size_t used = 0;
      int keep = 0;

      // TODO: use binary search for key finding process
      for (size_t pos = 0; ansi_keys[pos].text; pos++) {
        size_t c;
        int modifier = 0;
        for (c = 0; c<input_pos-check; c++) {
          if (ansi_keys[pos].text[c]=='m') {
            modifier = input_buffer[c+check]-'1';
            continue;
          } else if (ansi_keys[pos].text[c]=='?') {
            continue;
          } else if ((ansi_keys[pos].text[c]==0 && c>0) || input_buffer[c+check]!=ansi_keys[pos].text[c]) {
            break;
          }
        }

        if (c==input_pos-check || (ansi_keys[pos].text[c]==0 && c>0)) {
          keep = 1;
          if (ansi_keys[pos].text[c]==0 && c>0) {
            int key = ansi_keys[pos].key;
            if (modifier&1) {
              key |= TIPPSE_KEY_MOD_SHIFT;
            }

            if (modifier&2) {
              key |= TIPPSE_KEY_MOD_ALT;
            }

            if (modifier&4) {
              key |= TIPPSE_KEY_MOD_CTRL;
            }

            codepoint_t cp = ansi_keys[pos].cp;

            if (!bracket_paste) {
              if (key==TIPPSE_KEY_MOUSE) {
                if (input_buffer[3]&4) key |= TIPPSE_KEY_MOD_SHIFT;
                if (input_buffer[3]&8) key |= TIPPSE_KEY_MOD_ALT;
                if (input_buffer[3]&16) key |= TIPPSE_KEY_MOD_CTRL;
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

              editor_keypress(editor, key, cp, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
            }

            if (key==TIPPSE_KEY_BRACKET_PASTE_START && !bracket_paste) {
              bracket_paste = 1;
            } else if (key==TIPPSE_KEY_BRACKET_PASTE_STOP && bracket_paste) {
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
        codepoint_t cp = (*screen->encoding->decode)(NULL, &stream, &used);
        if (cp==-1) {
          used = 0;
          break;
        }

        if (!bracket_paste) {
          editor_keypress(editor, TIPPSE_KEY_CHARACTER, cp, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
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
  unicode_free();
  free(base_path);

  return 0;
}
