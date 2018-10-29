// Tippse - A fast simple editor - Windows - GUI

#ifdef _WINDOWS
#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include "types.h"

#include "editor.h"
#include "encoding/utf8.h"
#include "screen.h"
#include "search.h"
#include "unicode.h"

struct tippse_window {
  struct screen* screen;    // screen handler for painting
  struct editor* editor;    // editor handler for high level events
  uint8_t keystate[256];    // keyboard state of virtual keys
  HKL keyboard_layout;      // keyboard layout needed for translation (unused)
  int mouse_buttons;        // Current mouse button state
  int64_t cursor_blink;     // Start tick of cursor blinking
  int cursor_blink_old;     // cursor blink state
};

void tippse_detect_keyboard_layout(struct tippse_window* base) {
  base->keyboard_layout = GetKeyboardLayout(GetCurrentThreadId());
}

int tippse_cursor_blink(struct tippse_window* base) {
  int cursor = 1;
  int64_t tick = tick_count();
  int64_t diff = (tick-base->cursor_blink)/500000;
  if (diff<6) {
    cursor = (diff+1)&1;
  }
  return cursor;
}

LRESULT CALLBACK tippse_wndproc(HWND window, UINT message, WPARAM param1, LPARAM param2) {
  struct tippse_window* base = (struct tippse_window*)GetWindowLongPtr(window, GWLP_USERDATA);
  if (message==WM_CREATE) {
    CREATESTRUCT* data = (CREATESTRUCT*)param2;
    base = (struct tippse_window*)data->lpCreateParams;
    base->screen->window = window;
    base->mouse_buttons = 0;
    base->screen->font = CreateFont(16, 8, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, /*"Lucida Console"*/"FixedSys");

    HDC context = GetDC(window);
    SIZE size;
    SelectObject(context, base->screen->font);
    GetTextExtentPoint32(context, " ", 1, &size);
    ReleaseDC(window, context);
    base->screen->font_width = (int)size.cx;
    base->screen->font_height = (int)size.cy;

    tippse_detect_keyboard_layout(base);
    memset(&base->keystate[0], 0, sizeof(base->keystate));
    SetTimer(window, 100, 100, NULL);
    return 0;
  } else if (message==WM_INPUTLANGCHANGE) {
    tippse_detect_keyboard_layout(base);
  } else if (message==WM_TIMER) {
    editor_tick(base->editor);
    int cursor = tippse_cursor_blink(base);
    if (base->cursor_blink_old!=cursor) {
      base->cursor_blink_old = cursor;
      screen_update(base->screen);
    }
  } else if (message==WM_SIZE) {
    screen_check(base->screen);
    editor_draw(base->editor);
  } else if (message==WM_PAINT) {
    PAINTSTRUCT ps;
    BeginPaint(window, &ps);
    int cursor = tippse_cursor_blink(base);
    screen_draw(base->screen, ps.hdc, ps.fErase, cursor);
    base->cursor_blink_old = cursor;
    EndPaint(window, &ps);
  } else if (message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN || message==WM_MBUTTONDOWN || message==WM_LBUTTONUP || message==WM_RBUTTONUP || message==WM_MBUTTONUP || message==WM_MOUSEMOVE || message==WM_MOUSEWHEEL) {
    int mouse_buttons = 0;
    if (param1&MK_LBUTTON) {
      mouse_buttons |= TIPPSE_MOUSE_LBUTTON;
    }
    if (param1&MK_RBUTTON) {
      mouse_buttons |= TIPPSE_MOUSE_RBUTTON;
    }
    if (param1&MK_MBUTTON) {
      mouse_buttons |= TIPPSE_MOUSE_MBUTTON;
    }

    if (message==WM_MOUSEWHEEL) {
      int delta = GET_WHEEL_DELTA_WPARAM(param1);
      if (delta>0) {
        mouse_buttons |= TIPPSE_MOUSE_WHEEL_UP;
      } else if (delta<0) {
        mouse_buttons |= TIPPSE_MOUSE_WHEEL_DOWN;
      }
    }

    int key = TIPPSE_KEY_MOUSE;
    if (param1&MK_CONTROL) {
      key |= TIPPSE_KEY_MOD_CTRL;
    }
    if (param1&MK_SHIFT) {
      key |= TIPPSE_KEY_MOD_SHIFT;
    }


    editor_keypress(base->editor, key, 0, mouse_buttons, base->mouse_buttons, (int)((int16_t)(param2&0xffff))/base->screen->font_width, (int)((int16_t)(param2>>16))/base->screen->font_height);
    editor_draw(base->editor);
    base->mouse_buttons = mouse_buttons;
  } else if (message==WM_KEYDOWN) {
    wchar_t output[256];
    int ret = ToUnicodeEx(param1, (param2>>16)&0xff, &base->keystate[0], &output[0], sizeof(output)/sizeof(wchar_t), 0, base->keyboard_layout);
    base->keystate[param1] = 0x80;

    codepoint_t cp = 0;
    int key = TIPPSE_KEY_CHARACTER;
    if (param1==VK_UP) {
      key = TIPPSE_KEY_UP;
    } else if (param1==VK_DOWN) {
      key = TIPPSE_KEY_DOWN;
    } else if (param1==VK_RIGHT) {
      key = TIPPSE_KEY_RIGHT;
    } else if (param1==VK_LEFT) {
      key = TIPPSE_KEY_LEFT;
    } else if (param1==VK_PRIOR) {
      key = TIPPSE_KEY_PAGEUP;
    } else if (param1==VK_NEXT) {
      key = TIPPSE_KEY_PAGEDOWN;
    } else if (param1==VK_BACK) {
      key = TIPPSE_KEY_BACKSPACE;
    } else if (param1==VK_DELETE) {
      key = TIPPSE_KEY_DELETE;
    } else if (param1==VK_INSERT) {
      key = TIPPSE_KEY_INSERT;
    } else if (param1==VK_HOME) {
      key = TIPPSE_KEY_FIRST;
    } else if (param1==VK_END) {
      key = TIPPSE_KEY_LAST;
    } else if (param1==VK_TAB) {
      key = TIPPSE_KEY_TAB;
    } else if (param1==VK_RETURN) {
      key = TIPPSE_KEY_RETURN;
    } else if (param1==VK_ESCAPE) {
      key = TIPPSE_KEY_ESCAPE;
    } else if (param1==VK_RETURN) {
      key = TIPPSE_KEY_RETURN;
    } else if (param1==VK_F1) {
      key = TIPPSE_KEY_F1;
    } else if (param1==VK_F2) {
      key = TIPPSE_KEY_F2;
    } else if (param1==VK_F3) {
      key = TIPPSE_KEY_F3;
    } else if (param1==VK_F4) {
      key = TIPPSE_KEY_F4;
    } else if (param1==VK_F5) {
      key = TIPPSE_KEY_F5;
    } else if (param1==VK_F6) {
      key = TIPPSE_KEY_F6;
    } else if (param1==VK_F7) {
      key = TIPPSE_KEY_F7;
    } else if (param1==VK_F8) {
      key = TIPPSE_KEY_F8;
    } else if (param1==VK_F9) {
      key = TIPPSE_KEY_F9;
    } else if (param1==VK_F10) {
      key = TIPPSE_KEY_F10;
    } else if (param1==VK_F11) {
      key = TIPPSE_KEY_F11;
    } else if (param1==VK_F12) {
      key = TIPPSE_KEY_F12;
    } else {
      if (ret>0 && !base->keystate[VK_MENU] && !base->keystate[VK_CONTROL]) {
        cp = output[0];
      } else {
        cp = MapVirtualKeyEx(param1, MAPVK_VK_TO_CHAR, base->keyboard_layout);
        if (cp>='A' && cp<='Z') {
          cp -= 'A';
          cp += 'a';
        }
      }
    }
    if (base->keystate[VK_MENU]) {
      key |= TIPPSE_KEY_MOD_ALT;
    }
    if (base->keystate[VK_CONTROL]) {
      key |= TIPPSE_KEY_MOD_CTRL;
    }
    if (base->keystate[VK_SHIFT]) {
      key |= TIPPSE_KEY_MOD_SHIFT;
    }

    editor_keypress(base->editor, key, (int)cp, 0, 0, 0, 0);
    editor_draw(base->editor);
    base->cursor_blink = tick_count();
  } else if (message==WM_KEYUP) {
    base->keystate[param1] = 0;
  } else if (message==WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(window, message, param1, param2);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, char* command_line, int show) {
  encoding_init();
  char* base_path = realpath(".", NULL);

  printf("Base: %s\r\n", base_path);
  unicode_init();

  char* cmdline = string_internal(GetCommandLineW());
  char* buffer = strdup(cmdline);
  char** argv = malloc(sizeof(char*)*strlen(buffer));
  int argc = 0;
  char* index = buffer;
  char* current = cmdline;
  int quotes = 0;
  while (1) {
    if ((*current<=0x20 && *current>0 && !quotes) || *current=='\0') {
      if (index) {
        *index++ = '\0';
        argv[argc++] = strdup(buffer);
      }

      if (*current=='\0') {
        break;
      }
      index = NULL;
      current++;
      continue;
    }

    if (!index) {
      index = buffer;
    }

    if (*current=='"' && *(current+1)=='"') {
      *index++ = '"';
      current+=2;
    } else if (*current=='"') {
      quotes = !quotes;
      current++;
    } else {
      *index++ = *current;
      current++;
    }
  }

  free(buffer);
  free(cmdline);

  WNDCLASS wndclass;
  memset(&wndclass, 0, sizeof(wndclass));
  wndclass.style = 0;
  wndclass.lpfnWndProc = tippse_wndproc;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = 0;
  wndclass.hInstance = GetModuleHandle(NULL);
  wndclass.hIcon = NULL;
  wndclass.hCursor = NULL;
  wndclass.hbrBackground = NULL;
  wndclass.lpszMenuName = NULL;
  wndclass.lpszClassName = "Tippse";
  RegisterClassA(&wndclass);

  struct tippse_window base;
  base.screen = screen_create();
  base.editor = editor_create(base_path, base.screen, argc, (const char**)argv);
  base.cursor_blink = 0;
  base.cursor_blink_old = 2;

  for (int n = 0; n<argc; n++) {
    free(argv[n]);
  }
  free(argv);

  HWND window = CreateWindowEx(0, wndclass.lpszClassName, "Tippse", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, wndclass.hInstance, &base);
  SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)&base);
  ShowWindow(window, show);

  MSG msg;
  while (!base.editor->close) {
    BOOL ret = GetMessage(&msg, window, 0, 0);
    if (ret==-1 || ret==0) {
      break;
    } else {
      DispatchMessage(&msg);
    }
  }

  DestroyWindow(window);
  UnregisterClass(wndclass.lpszClassName, wndclass.hInstance);

  editor_destroy(base.editor);
  screen_destroy(base.screen);
  clipboard_free();
  unicode_free();
  free(base_path);
  encoding_free();

  return 0;
}
#endif
