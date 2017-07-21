/* Tippse - A fast simple editor */
/* Project is in early stage, use with care at your own risk, anything could happen */
/* Project may change substantially during this phase */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "list.h"
#include "fragment.h"
#include "rangetree.h"
#include "misc.h"
#include "screen.h"
#include "document.h"
#include "documentfile.h"
#include "documentview.h"
#include "documentundo.h"
#include "clipboard.h"
#include "encoding_utf8.h"

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
  {"\x10", TIPPSE_KEY_SHOWALL, 0},
  {"\x1b[6~", TIPPSE_KEY_PAGEDOWN, 0},
  {"\x1b[5~", TIPPSE_KEY_PAGEUP, 0},
  {"\x1b[6;2~", TIPPSE_KEY_PAGEDOWN, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[5;2~", TIPPSE_KEY_PAGEUP, TIPPSE_KEY_MOD_SHIFT},
  {"\x7f", TIPPSE_KEY_BACKSPACE, 0},
  {"\x1b[3~", TIPPSE_KEY_DELETE, 0},
  {"\x1b[2~", TIPPSE_KEY_INSERT, 0},
  {"\x1b[H", TIPPSE_KEY_FIRST, 0},
  {"\x1b[F", TIPPSE_KEY_LAST, 0},
  {"\x1b[1;2H", TIPPSE_KEY_FIRST, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;2F", TIPPSE_KEY_LAST, TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;5H", TIPPSE_KEY_HOME, TIPPSE_KEY_MOD_CTRL},
  {"\x1b[1;5F", TIPPSE_KEY_END, TIPPSE_KEY_MOD_CTRL},
  {"\x1b[1;6H", TIPPSE_KEY_HOME, TIPPSE_KEY_MOD_CTRL|TIPPSE_KEY_MOD_SHIFT},
  {"\x1b[1;6F", TIPPSE_KEY_END, TIPPSE_KEY_MOD_CTRL|TIPPSE_KEY_MOD_SHIFT},
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
  {"\r", '\n', 0},
  {NULL, 0, 0}
};

int main (int argc, const char** argv) {  
  unsigned char input_buffer[1024];
  size_t input_pos = 0;

  struct termios original, raw;
  struct screen* screen;

  const char* base_path = realpath(".", NULL);
  tcgetattr(STDIN_FILENO, &original);
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);

  write(STDOUT_FILENO, "\x1b[?47h", 6);
  write(STDOUT_FILENO, "\x1b[?25l", 6);
  write(STDOUT_FILENO, "\x1b[?7l", 5);
  write(STDOUT_FILENO, "\x1b[?2004h", 8);
  write(STDOUT_FILENO, "\x1b[?1003h", 8);
  write(STDOUT_FILENO, "\x1b[?1005h", 8);

  screen = screen_init();

  struct list* documents = list_create();
  
  struct document_file* tabs_doc = document_file_create(0);
  document_file_name(tabs_doc, "Open");
  struct splitter* tabs = splitter_create(0, 0, NULL, NULL, 231, 17, "Tabs");
  splitter_assign_document_file(tabs, tabs_doc, 0);

  struct document_file* browser_doc = document_file_create(0);
  document_file_name(browser_doc, base_path);
  struct splitter* browser = splitter_create(0, 0, NULL, NULL, 231, 17, "Browser");
  splitter_assign_document_file(browser, browser_doc, 0);

  struct document_file* document_doc = document_file_create(1);
  document_file_name(document_doc, "Untitled");
  struct splitter* document = splitter_create(0, 0, NULL, NULL, 231, 17, "Document");
  splitter_assign_document_file(document, document_doc, 1);
  
  struct document_file* search_doc = document_file_create(0);
  document_file_name(search_doc, "Search");
  struct splitter* search = splitter_create(0, 0, NULL, NULL, 231, 17, "Find");
  splitter_assign_document_file(search, search_doc, 0);
  
  struct range_tree_node* search_text_buffers[32];
  const char* search_text = " Search [\x7f]\n   CASE [\x7f] | SELECTION [\x7f] | REGEX [\x7f]\nReplace [\x7f]";
  search_doc->buffer = range_tree_insert_split(search_doc->buffer, search_doc->type, 0, search_text, strlen(search_text), TIPPSE_INSERTER_READONLY|TIPPSE_INSERTER_ESCAPE|TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER, &search_text_buffers[0]);
  
  if (argc>=2) {
    document_file_load(document_doc, argv[1]);
  }
 
  struct splitter* splitters_left = splitter_create(TIPPSE_SPLITTER_VERT, 50, tabs, browser, 0, 0, "");
  struct splitter* splitters_right = splitter_create(TIPPSE_SPLITTER_VERT|TIPPSE_SPLITTER_FIXED1, 5, document, search, 0, 0, "");
  struct splitter* splitters = splitter_create(TIPPSE_SPLITTER_HORZ, 12, splitters_left, splitters_right, 0, 0, "");
  list_insert(documents, NULL, tabs_doc);
  list_insert(documents, NULL, document_doc);
  list_insert(documents, NULL, search_doc);
  list_insert(documents, NULL, browser_doc);

  struct splitter* focus = document;
  struct splitter* last_document = document;
  focus->active = 1;

  int bracket_paste = 0;
  int mouse_buttons = 0;
  int mouse_buttons_old = 0;
  int mouse_x = 0;
  int mouse_y = 0;
  
  document_directory(&browser->document);

  int close = 0;

  // Performance tests
  int perf_test = 0;
  if (perf_test) {
    goto end;
  }

  while (!close) {
    tabs_doc->buffer = range_tree_delete(tabs_doc->buffer, tabs_doc->type, 0, tabs_doc->buffer?tabs_doc->buffer->length:0, TIPPSE_INSERTER_AUTO);
    struct list_node* doc = documents->first;
    while (doc) {
      struct document_file* file = (struct document_file*)doc->object;
      tabs_doc->buffer = range_tree_insert_split(tabs_doc->buffer, tabs_doc->type, tabs_doc->buffer?tabs_doc->buffer->length:0, file->modified?"+":" ", 1, TIPPSE_INSERTER_READONLY|TIPPSE_INSERTER_ESCAPE|TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      tabs_doc->buffer = range_tree_insert_split(tabs_doc->buffer, tabs_doc->type, tabs_doc->buffer?tabs_doc->buffer->length:0, file->filename, strlen(file->filename), TIPPSE_INSERTER_READONLY|TIPPSE_INSERTER_ESCAPE|TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      tabs_doc->buffer = range_tree_insert_split(tabs_doc->buffer, tabs_doc->type, tabs_doc->buffer?tabs_doc->buffer->length:0, "\n", 1, TIPPSE_INSERTER_READONLY|TIPPSE_INSERTER_ESCAPE|TIPPSE_INSERTER_BEFORE|TIPPSE_INSERTER_AFTER|TIPPSE_INSERTER_AUTO, NULL);
      doc = doc->next;
    }

    if (focus==search) {
      splitters_right->split = 5;
    } else {
      splitters_right->split = 0;
    }

    if (focus==browser || focus==tabs) {
      splitters->split = 12;
    } else {
      splitters->split = 0;
    }

    splitter_draw_multiple(screen, splitters, 0);
    int x;
    for (x = 0; x<screen->width; x++) {
      screen_setchar(screen, x, 0, 0x20, 102, 17);
    }

    screen_drawtext(screen, 0, 0, focus->name, screen->width, 102, 17);
    struct encoding_stream stream;
    encoding_stream_from_plain(&stream, focus->status, ~0);
    int length = encoding_utf8_strlen(NULL, &stream);
    screen_drawtext(screen, screen->width-length, 0, focus->status, screen->width, 102, 17);

    screen_draw(screen);
    int in = 0;
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
          input_pos += in;
        }
      } else {
        struct list_node* doc = documents->first;
        while (doc) {
          struct document_file* file = (struct document_file*)doc->object;
          document_undo_chain(file);
          doc = doc->next;
        }

        splitter_draw_multiple(screen, splitters, 1);
      }
    }

    size_t check = 0;
    while (1) {
      size_t used = 0;
      int keep = 0;
      size_t pos;
      for (pos = 0; ansi_keys[pos].cp!=0; pos++) {
        size_t c;
        for (c = 0; c<input_pos-check; c++) {
          if (ansi_keys[pos].text[c]==0 || (ansi_keys[pos].text[c]!=input_buffer[c+check] && ansi_keys[pos].text[c]!='?')) {
            break;
          }
        }

        if (c==input_pos-check || ansi_keys[pos].text[c]==0) {
          keep = 1;
          if (ansi_keys[pos].text[c]==0) {
            if (ansi_keys[pos].cp==TIPPSE_KEY_CLOSE) {
              close = 1;
            }

            if (ansi_keys[pos].cp==TIPPSE_KEY_SHOWALL) {
              focus->document.view.showall ^= 1;
            }

            if (ansi_keys[pos].cp==TIPPSE_KEY_SEARCH) {
              focus->active = 0;
              if (focus==document) {
                focus = search;
              } else {
                focus = document;
              }
              focus->active = 1;
            }

            if (ansi_keys[pos].cp==TIPPSE_KEY_BROWSER) {
              focus->active = 0;
              focus = browser;
              focus->active = 1;
            }

            if (ansi_keys[pos].cp==TIPPSE_KEY_TIPPSE_MOUSE_INPUT) {
              int buttons = input_buffer[3];
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
              
              if (mouse_buttons!=0 && mouse_buttons_old==0) {
                struct splitter* select = splitter_by_coordinate(splitters, mouse_x, mouse_y);
                if (select) {
                  focus->active = 0;
                  focus = select;
                  focus->active = 1;
                  if (select->document.content_document) {
                    document = select;
                  }

                  if (select!=search) {
                    last_document = select;
                  }
                }
              }
            }

            if (ansi_keys[pos].cp==TIPPSE_KEY_SEARCH_NEXT) {
              focus->active = 0;
              focus = document;
              focus->active = 1;
              document_search(last_document, &last_document->document, range_tree_next(search_text_buffers[1]), range_tree_distance_offset(search->document.file->buffer, search_text_buffers[1], search_text_buffers[2]), 1);
            } else if (ansi_keys[pos].cp==TIPPSE_KEY_SEARCH_PREV) {
/*              printf("\x1b[H\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
              range_tree_print(last_document->document.file->buffer, 0, 0);
              tcsetattr(STDIN_FILENO, TCSANOW, &original);
              exit(0);*/
              focus->active = 0;
              focus = document;
              focus->active = 1;
              document_search(last_document, &last_document->document, range_tree_next(search_text_buffers[1]), range_tree_distance_offset(search->document.file->buffer, search_text_buffers[1], search_text_buffers[2]), 0);
            } else if (ansi_keys[pos].cp==TIPPSE_KEY_OPEN) {
              if (focus->document.view.selection_low!=focus->document.view.selection_high) {
                struct list_node* views =document->document.file->views->first;
                while (views) {
                  struct document_view* view = (struct document_view*)views->object;
                  if (view==&document->document.view) {
                    list_remove(document->document.file->views, views);
                    break;
                  }

                  views = views->next;
                }

                char* name = range_tree_raw(focus->document.file->buffer, focus->document.view.selection_low, focus->document.view.selection_high);
                if (*name) {
                  char* path_only = (focus==browser)?strdup(focus->document.file->filename):strip_file_name(focus->document.file->filename);
                  char* combined = combine_path_file(path_only, name);
                  char* corrected = correct_path(combined);
                  char* relative = relativate_path(base_path, corrected);
//                  printf("                     \"%s -> %s\"\r\n", combined, corrected);

                  struct document_file* new_document_doc = NULL;
                  struct list_node* docs = documents->first;
                  while (docs) {
                    struct document_file* docs_document_doc = (struct document_file*)docs->object;
                    if (strcmp(docs_document_doc->filename, relative)==0 && docs_document_doc!=focus->document.file) {
                      new_document_doc = docs_document_doc;
                      break;
                    }

                    docs = docs->next;
                  }

                  if (!new_document_doc) {
                    if (is_directory(relative)) {
                      if (focus==browser) {
                        document_file_name(browser->document.file, relative);
                        document_view_reset(&browser->document.view);
                        document_directory(&browser->document);
                      }
                    } else {
                      new_document_doc = document_file_create(1);
                      list_insert(documents, NULL, new_document_doc);

                      document_file_load(new_document_doc, relative);
                    }
                  }

                  if (new_document_doc) {
                    document_view_reset(&document->document.view);
                    splitter_assign_document_file(document, new_document_doc, 1);
                  }

                  free(relative);
                  free(combined);
                  free(path_only);
                  free(corrected);
                }
                free(name);
              }
            } else if (ansi_keys[pos].cp==TIPPSE_KEY_NEW_VERT_TAB) {
              struct splitter* parent = document->parent;
              struct splitter* split = document;
              document = splitter_create(0, 0, NULL, NULL, 231, 17, "Document");
              
              splitter_assign_document_file(document, split->document.file, split->document.content_document);

              struct splitter* splitter = splitter_create(TIPPSE_SPLITTER_HORZ, 50, split, document, 0, 0, "");
              if (parent->side[0]==split) {
                parent->side[0] = splitter;
              } else {
                parent->side[1] = splitter;
              }
              splitter->parent = parent;
            } else if (ansi_keys[pos].cp==TIPPSE_KEY_SAVE) {
              struct list_node* docs = documents->first;
              while (docs) {
                struct document_file* docs_document_doc = (struct document_file*)docs->object;
                if (docs_document_doc->modified && docs_document_doc->save) {
                  document_file_save(docs_document_doc, docs_document_doc->filename);
                }

                docs = docs->next;
              }
            } else {
              if (!bracket_paste) {
                document_keypress(focus, ansi_keys[pos].cp, ansi_keys[pos].modifier, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
              }
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
        encoding_stream_from_plain(&stream, (const char*)&input_buffer[check], (const char*)&input_buffer[input_pos]-(const char*)&input_buffer[check]);
        int cp = encoding_utf8_decode(NULL, &stream, &used);
        if (cp==-1) {
          used = 0;
          break;
        }

        if (!bracket_paste) {
          document_keypress(focus, cp, 0, mouse_buttons, mouse_buttons_old, mouse_x, mouse_y);
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

end:;
  write(STDOUT_FILENO, "\x1b[?1005l", 8);
  write(STDOUT_FILENO, "\x1b[?1003l", 8);
  write(STDOUT_FILENO, "\x1b[?2004l", 8);
  write(STDOUT_FILENO, "\x1b[?7h", 5);
  write(STDOUT_FILENO, "\x1b[?25h", 6);
  write(STDOUT_FILENO, "\x1b[?47l", 6);
  write(STDOUT_FILENO, "\x1b[39;49m", 8);
  tcsetattr(STDIN_FILENO, TCSANOW, &original);

  if (perf_test) {
    int64_t time_start = tick_count();
    while (1) {
      if (document_incremental_update(document)==0) {
        break;
      }
    }
    printf("Tippse test - Runtime %lld\r\n", (long long)(tick_count()-time_start));
    printf("Node ratio %3.3f\r\n", (float)sizeof(struct range_tree_node)/(float)TREE_BLOCK_LENGTH_MAX);
//    range_tree_print(document->document.file->buffer, 0, 0);
  }

  screen_free(screen);

  return 0;
}
