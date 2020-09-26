// Tippse - A fast simple editor - emscripten

#ifdef _EMSCRIPTEN
#include <emscripten.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "types.h"

#include "clipboard.h"
#include "documentfile.h"
#include "editor.h"
#include "encoding/utf8.h"
#include "file.h"
#include "misc.h"
#include "screen.h"
#include "search.h"
#include "unicode.h"

struct screen* screen;
struct editor* editor;
char* base_path;

void tippse_update_signal(struct document_file* file) {
}

int EMSCRIPTEN_KEEPALIVE main(int argc, const char** argv) {
  EM_ASM(tippse_load());
}

void EMSCRIPTEN_KEEPALIVE tippse_init() {
  encoding_init();
  base_path = realpath(".", NULL);

  unicode_init();

  screen = screen_create();
  editor = editor_create(base_path, screen, 0, NULL, tippse_update_signal);
}

void EMSCRIPTEN_KEEPALIVE tippse_tick() {
  editor_tick(editor);
  editor_draw(editor);
}

void EMSCRIPTEN_KEEPALIVE tippse_keypress(int key, codepoint_t cp, int button, int button_old, int x, int y) {
  editor_keypress(editor, key, cp, button, button_old, x, y);
  editor_draw(editor);
}

void EMSCRIPTEN_KEEPALIVE tippse_resize(int width, int height) {
  printf("%d / %d\r\n", width, height);
  screen_resize(screen, width, height);
  editor_draw(editor);
}

void EMSCRIPTEN_KEEPALIVE tippse_free() {
  editor_destroy(editor);
  screen_destroy(screen);
  clipboard_free();
  unicode_free();
  free(base_path);
  encoding_free();
}
#endif
