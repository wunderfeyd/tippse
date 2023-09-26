// Tippse - A fast simple editor - Ansi - Posix

#ifdef _ANSI_POSIX
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#ifdef __APPLE__
#include <sys/param.h>
#include <stdlib.h>
#endif

#include "types.h"

#include "clipboard.h"
#include "documentfile.h"
#include "editor.h"
#include "library/encoding/utf8.h"
#include "library/file.h"
#include "library/misc.h"
#include "screen.h"
#include "library/search.h"
#include "library/unicode.h"

#ifdef _PERFORMANCE
#include "splitter.h"
#endif

struct tippse_ansi_key {
  const char* text; // text in input stream
  int key;          // associated key
  codepoint_t cp;   // codepoint if needed
};

// Ansi command table
// ? = single byte, + = digits, # = modifier param
struct tippse_ansi_key ansi_keys[] = {
  {"\x00\xff", TIPPSE_KEY_SPACE|TIPPSE_KEY_MOD_CTRL, 0},
  {"\x01\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'a'},
  {"\x02\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'b'},
  {"\x03\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'c'},
  {"\x04\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'd'},
  {"\x05\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'e'},
  {"\x06\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'f'},
  {"\x07\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'g'},
  {"\x08\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'h'},
  {"\t\xff", TIPPSE_KEY_TAB, 0},
  {"\n\xff", TIPPSE_KEY_RETURN, 0},
  {"\x0b\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'k'},
  {"\x0c\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'l'},
  {"\r\xff", TIPPSE_KEY_RETURN, 0},
  {"\x0e\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'n'},
  {"\x0f\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'o'},
  {"\x10\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'p'},
  {"\x11\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'q'},
  {"\x12\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'r'},
  {"\x13\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 's'},
  {"\x14\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 't'},
  {"\x15\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'u'},
  {"\x16\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'v'},
  {"\x17\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'w'},
  {"\x18\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'x'},
  {"\x19\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'y'},
  {"\x1a\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, 'z'},
  {"\x1b[A\xff", TIPPSE_KEY_UP, 0},
  {"\x1b[B\xff", TIPPSE_KEY_DOWN, 0},
  {"\x1b[C\xff", TIPPSE_KEY_RIGHT, 0},
  {"\x1b[D\xff", TIPPSE_KEY_LEFT, 0},
  {"\x1b[F\xff", TIPPSE_KEY_LAST, 0},
  {"\x1b[H\xff", TIPPSE_KEY_FIRST, 0},
  {"\x1b[1;#A\xff", TIPPSE_KEY_UP, 0},
  {"\x1b[1;#B\xff", TIPPSE_KEY_DOWN, 0},
  {"\x1b[1;#C\xff", TIPPSE_KEY_RIGHT, 0},
  {"\x1b[1;#D\xff", TIPPSE_KEY_LEFT, 0},
  {"\x1b[1;#F\xff", TIPPSE_KEY_LAST, 0},
  {"\x1b[1;#H\xff", TIPPSE_KEY_FIRST, 0},
  {"\x1b[1;#P\xff", TIPPSE_KEY_F1, 0},
  {"\x1b[1;#Q\xff", TIPPSE_KEY_F2, 0},
  {"\x1b[1;#R\xff", TIPPSE_KEY_F3, 0},
  {"\x1b[1;#S\xff", TIPPSE_KEY_F4, 0},
  {"\x1b[200~\xff", TIPPSE_KEY_BRACKET_PASTE_START, 0},
  {"\x1b[201~\xff", TIPPSE_KEY_BRACKET_PASTE_STOP, 0},
  {"\x1b[1;#~\xff", TIPPSE_KEY_FIRST, 0},
  {"\x1b[15;#~\xff", TIPPSE_KEY_F5, 0},
  {"\x1b[17;#~\xff", TIPPSE_KEY_F6, 0},
  {"\x1b[18;#~\xff", TIPPSE_KEY_F7, 0},
  {"\x1b[19;#~\xff", TIPPSE_KEY_F8, 0},
  {"\x1b[20;#~\xff", TIPPSE_KEY_F9, 0},
  {"\x1b[21;#~\xff", TIPPSE_KEY_F10, 0},
  {"\x1b[23;#~\xff", TIPPSE_KEY_F11, 0},
  {"\x1b[24;#~\xff", TIPPSE_KEY_F12, 0},
  {"\x1b[2;#~\xff", TIPPSE_KEY_INSERT, 0},
  {"\x1b[3;#~\xff", TIPPSE_KEY_DELETE, 0},
  {"\x1b[4;#~\xff", TIPPSE_KEY_LAST, 0},
  {"\x1b[5;#~\xff", TIPPSE_KEY_PAGEUP, 0},
  {"\x1b[6;#~\xff", TIPPSE_KEY_PAGEDOWN, 0},
  {"\x1b[1~\xff", TIPPSE_KEY_FIRST, 0},
  {"\x1b[11~\xff", TIPPSE_KEY_F1, 0},
  {"\x1b[12~\xff", TIPPSE_KEY_F2, 0},
  {"\x1b[13~\xff", TIPPSE_KEY_F3, 0},
  {"\x1b[14~\xff", TIPPSE_KEY_F4, 0},
  {"\x1b[15~\xff", TIPPSE_KEY_F5, 0},
  {"\x1b[17~\xff", TIPPSE_KEY_F6, 0},
  {"\x1b[18~\xff", TIPPSE_KEY_F7, 0},
  {"\x1b[19~\xff", TIPPSE_KEY_F8, 0},
  {"\x1b[2~\xff", TIPPSE_KEY_INSERT, 0},
  {"\x1b[20~\xff", TIPPSE_KEY_F9, 0},
  {"\x1b[21~\xff", TIPPSE_KEY_F10, 0},
  {"\x1b[23~\xff", TIPPSE_KEY_F1|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[24~\xff", TIPPSE_KEY_F2|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[25~\xff", TIPPSE_KEY_F3|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[26~\xff", TIPPSE_KEY_F4|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[28~\xff", TIPPSE_KEY_F5|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[29~\xff", TIPPSE_KEY_F6|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[3~\xff", TIPPSE_KEY_DELETE, 0},
  {"\x1b[31~\xff", TIPPSE_KEY_F7|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[32~\xff", TIPPSE_KEY_F8|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[33~\xff", TIPPSE_KEY_F9|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[34~\xff", TIPPSE_KEY_F10|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1b[4~\xff", TIPPSE_KEY_LAST, 0},
  {"\x1b[5~\xff", TIPPSE_KEY_PAGEUP, 0},
  {"\x1b[6~\xff", TIPPSE_KEY_PAGEDOWN, 0},
  {"\x1b[M???\xff", TIPPSE_KEY_MOUSE, 0},
  {"\x1b[<+;+;+M\xff", TIPPSE_KEY_MOUSE, 0},
  {"\x1b[<+;+;+m\xff", TIPPSE_KEY_MOUSE, 0},
  {"\x1b[Z\xff", TIPPSE_KEY_TAB|TIPPSE_KEY_MOD_SHIFT, 0},
  {"\x1bOA\xff", TIPPSE_KEY_UP, 0},
  {"\x1bOB\xff", TIPPSE_KEY_DOWN, 0},
  {"\x1bOC\xff", TIPPSE_KEY_RIGHT, 0},
  {"\x1bOD\xff", TIPPSE_KEY_LEFT, 0},
  {"\x1bOH\xff", TIPPSE_KEY_FIRST, 0},
  {"\x1bOF\xff", TIPPSE_KEY_LAST, 0},
  {"\x1bOP\xff", TIPPSE_KEY_F1, 0},
  {"\x1bOQ\xff", TIPPSE_KEY_F2, 0},
  {"\x1bOR\xff", TIPPSE_KEY_F3, 0},
  {"\x1bOS\xff", TIPPSE_KEY_F4, 0},
  {"\x1bO#A\xff", TIPPSE_KEY_UP, 0},
  {"\x1bO#B\xff", TIPPSE_KEY_DOWN, 0},
  {"\x1bO#C\xff", TIPPSE_KEY_RIGHT, 0},
  {"\x1bO#D\xff", TIPPSE_KEY_LEFT, 0},
  {"\x1bO#H\xff", TIPPSE_KEY_FIRST, 0},
  {"\x1bO#F\xff", TIPPSE_KEY_LAST, 0},
  {"\x1bO#P\xff", TIPPSE_KEY_F1, 0},
  {"\x1bO#Q\xff", TIPPSE_KEY_F2, 0},
  {"\x1bO#R\xff", TIPPSE_KEY_F3, 0},
  {"\x1bO#S\xff", TIPPSE_KEY_F4, 0},
  {"\x1b\x1b\xff", TIPPSE_KEY_ESCAPE, 0},
  {"\x1c\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '#'},
  {"\x1d\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '~'},
  {"\x1e\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '^'},
  {"\x1f\xff", TIPPSE_KEY_CHARACTER|TIPPSE_KEY_MOD_CTRL, '\\'},
  {"\x20\xff", TIPPSE_KEY_SPACE, 0},
  {"\x7f\xff", TIPPSE_KEY_BACKSPACE, 0},
  {NULL, 0, 0}
};

int tippse_pipefd[2];

void tippse_unblock_pipe(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags|O_NONBLOCK);
}

void tippse_update_signal(struct document_file* file) {
  UNUSED(write(tippse_pipefd[1], &file, sizeof(struct document_file*)));
}

int main(int argc, const char** argv) {
  encoding_init();
  static char base_path[PATH_MAX];
  if (!realpath(".", &base_path[0])) {
    base_path[0] = '.';
    base_path[1] = '\0';
  }

  unicode_init();

  if (0) {
    search_test();
    unicode_free();
    free(base_path);
    exit(0);
  }

  UNUSED(pipe(tippse_pipefd));
  tippse_unblock_pipe(tippse_pipefd[0]);
  tippse_unblock_pipe(tippse_pipefd[1]);

  struct screen* screen = screen_create();
  struct editor* editor = editor_create(base_path, screen, argc, argv, tippse_update_signal);

  // screen_character_width_detect(screen);

  int bracket_paste = 0;
  int mouse_buttons = 0;
  int mouse_buttons_old = 0;
  int mouse_x = 0;
  int mouse_y = 0;

  //encoding_utf8_build_tables();
#ifndef _PERFORMANCE // allow human input :)
  uint8_t input_buffer[1024];
  size_t input_pos = 0;
  int64_t ansi_timeout = 0;
  while (!editor->close) {
    editor_draw(editor);
    ssize_t in = 0;
    int stop = 0;
    int64_t start = 0;
    if (input_pos==0) {
      ansi_timeout = 0;
    }

    while (in==0) {
      int64_t tick = tick_count();
      if (!stop) {
        start = tick;
      } else {
#ifdef _TESTSUITE
        break;
#endif
      }

#ifdef _TESTSUITE
      int64_t left = tick_ms(1)-(tick-start);
      stop = 1;
#else
      int64_t left = tick_ms(100)-(tick-start);
#endif
      if (left<=0) {
        break;
      }

      fd_set set_read;
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = left;
      FD_ZERO(&set_read);
      FD_SET(STDIN_FILENO, &set_read);
      FD_SET(tippse_pipefd[0], &set_read);
      int nfds = tippse_pipefd[0];

      int ret = select(nfds+1, &set_read, NULL, NULL, &tv);
      if (ret>0) {
        if (FD_ISSET(STDIN_FILENO, &set_read)) {
          in = read(STDIN_FILENO, &input_buffer[input_pos], sizeof(input_buffer)-input_pos-1);
          if (in>0) {
            input_pos += (size_t)in;
            ansi_timeout = tick_count()+tick_ms(25);
          }
        }

        if (FD_ISSET(tippse_pipefd[0], &set_read)) {
          while (1) {
            struct document_file* file;
            int r = read(tippse_pipefd[0], &file, sizeof(struct document_file*));
            if (r!=sizeof(struct document_file*)) {
              break;
            }

            stop = 1;
            document_file_flush_pipe(file);
          }
        }
      }

      if (ansi_timeout && tick_count()>ansi_timeout) {
        if (input_pos>0 && input_buffer[input_pos-1]=='\x1b') {
          input_buffer[input_pos++] = '\x1b';
          in++;
        } else {
          input_pos = 0;
        }
        ansi_timeout = 0;
      }

      editor_tick(editor);
    }

    size_t check = 0;
    while (check!=input_pos) {
      uint8_t* data = &input_buffer[check];
      if (*data==0x1b && input_pos-check==1) {
        break;
      }

      size_t used = 0;
      for (size_t pos = 0; ansi_keys[pos].text; pos++) {
        uint8_t* text = (uint8_t*)ansi_keys[pos].text;
        size_t p = 0;
        int modifier = 0;
        for (; p<input_pos-check;) {
          uint8_t c = *text;
          if (c=='#' && data[p]>='1' && data[p]<='9') {
            modifier = data[p]-'1';
          } else if (c=='?') {
          } else if (c=='+') {
            if (data[p]>='0' && data[p]<='9') {
              p++;
            } else {
              text++;
            }

            continue;
          } else if (c==0xff || data[p]!=c) {
            break;
          }

          text++;
          p++;
        }

        if (*text==0xff) {
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
              int buttons = 3;
              if (data[2]=='<') {
                unsigned char* in = &data[2];
                for (int count = 0; count<3; count++) {
                  in++;
                  int val = 0;
                  while (*in>='0' && *in<='9') {
                    val *= 10;
                    val += (*in)-'0';
                    in++;
                  }


                  if (count==0) buttons = val;
                  if (count==1) mouse_x = val-1;
                  if (count==2) mouse_y = val-1;
                }

                if (*in=='m') {
                  buttons = 3;
                }
              } else {
                buttons = data[3]-32;
                mouse_x = data[4]-33;
                mouse_y = data[5]-33;
              }

              if (buttons&4) key |= TIPPSE_KEY_MOD_SHIFT;
              if (buttons&8) key |= TIPPSE_KEY_MOD_ALT;
              if (buttons&16) key |= TIPPSE_KEY_MOD_CTRL;
              buttons &= ~(4+8+16);

              mouse_buttons_old = mouse_buttons;
              if (buttons==3) {
                mouse_buttons &= ~TIPPSE_MOUSE_LBUTTON & ~TIPPSE_MOUSE_RBUTTON & ~TIPPSE_MOUSE_MBUTTON;
              } else if (buttons==0) {
                mouse_buttons |= TIPPSE_MOUSE_LBUTTON;
              } else if (buttons==1) {
                mouse_buttons |= TIPPSE_MOUSE_RBUTTON;
              } else if (buttons==2) {
                mouse_buttons |= TIPPSE_MOUSE_MBUTTON;
              }

              mouse_buttons &= ~TIPPSE_MOUSE_WHEEL_UP & ~TIPPSE_MOUSE_WHEEL_DOWN;
              if (buttons==64) {
                mouse_buttons |= TIPPSE_MOUSE_WHEEL_UP;
              } else if (buttons==65) {
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

          used = p;
          break;
        }
      }

      if (used==0) {
        struct stream stream;
        stream_from_plain(&stream, data, (size_t)input_pos-check);
        codepoint_t cp = (*screen->encoding->decode)(NULL, &stream, &used);
        if (!bracket_paste && cp<UNICODE_CODEPOINT_MAX) {
          editor_keypress(editor, TIPPSE_KEY_CHARACTER, cp, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
        }
        stream_destroy(&stream);

        if (cp==UNICODE_CODEPOINT_BAD) {
          used = 0;
        }
      }

      if (used==0) {
        break;
      }

      check += used;
    }

    input_pos -= check;
    if (check>0) {
      memmove(&input_buffer[0], &input_buffer[check], input_pos);
    }

#ifdef _TESTSUITE
    editor_test_read(editor);
#endif
  }
#else
  {
    int64_t tick = tick_count();
    struct stream stream;
    stream_from_page(&stream, range_tree_node_first(editor->document->file->buffer), 0);
    size_t length = range_tree_node_length(editor->document->file->buffer);
    uint8_t sum = 0;
    while (length-- >0) {
      sum += stream_read_forward(&stream);
    }
    stream_destroy(&stream);
    fprintf(stderr, "       Stream: %d   / %d\r\n", (int)(tick_count()-tick), (int)sum);
  }
  {
    int64_t tick = tick_count();
    struct stream stream;
    stream_from_page(&stream, range_tree_node_first(editor->document->file->buffer), 0);
    size_t length = range_tree_node_length(editor->document->file->buffer);
    codepoint_t sum = 0;
    struct encoding* encoding = editor->document->file->encoding;
    while (length >0) {
      size_t size;
      sum += (*encoding->decode)(encoding, &stream, &size);
      if (size>=length) {
        break;
      }

      length -= size;
    }
    stream_destroy(&stream);
    fprintf(stderr, "       Decode: %d   / %d\r\n", (int)(tick_count()-tick), (int)sum);
  }
  {
    int64_t tick = tick_count();
    struct stream stream;
    stream_from_page(&stream, range_tree_node_first(editor->document->file->buffer), 0);
    size_t length = range_tree_node_length(editor->document->file->buffer);
    codepoint_t sum = 0;
    struct encoding* encoding = editor->document->file->encoding;
    struct unicode_sequencer sequencer;
    unicode_sequencer_clear(&sequencer, encoding, &stream);
    while (length >0) {
      struct unicode_sequence* sequence = unicode_sequencer_find(&sequencer, 0);
      sum += sequence->cp[0];
      if (sequence->size>=length) {
        break;
      }

      length -= sequence->size;
      unicode_sequencer_advance(&sequencer, 1);
    }
    stream_destroy(&stream);
    fprintf(stderr, "    Sequenced: %d   / %d\r\n", (int)(tick_count()-tick), (int)sum);
  }
  {
    int64_t tick = tick_count();
    for (int n = 0; n<900; n++) {
      editor_keypress(editor, TIPPSE_KEY_RIGHT, 0, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
      //editor_tick(editor);
      //editor_draw(editor);
    }
    for (int n = 0; n<1000; n++) {
      editor_keypress(editor, (n&1)?TIPPSE_KEY_RIGHT:TIPPSE_KEY_LEFT, 0, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
      //editor_tick(editor);
      //editor_draw(editor);
    }
    fprintf(stderr, "    Move Time: %d\r\n", (int)(tick_count()-tick));
  }
  {
    int64_t tick = tick_count();
    editor_keypress(editor, TIPPSE_KEY_LAST|TIPPSE_KEY_MOD_CTRL, 0, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
    fprintf(stderr, "Last location: %d\r\n", (int)(tick_count()-tick));
  }
#endif

  editor_destroy(editor);
  screen_destroy(screen);
  clipboard_free();
  unicode_free();
  encoding_free();

  close(tippse_pipefd[0]);
  close(tippse_pipefd[1]);

  return 0;
}
#endif