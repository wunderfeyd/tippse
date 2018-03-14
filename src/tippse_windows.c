// Tippse - A fast simple editor - Windows - GUI

#ifdef _WINDOWS
#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#include "screen.h"
#include "encoding/utf8.h"
#include "unicode.h"
#include "editor.h"
#include "search.h"

struct tippse_window {
  struct screen* screen;    // screen handler for painting
  struct editor* editor;    // editor handler for high level events
  HFONT font;               // console font
  uint8_t keystate[256];    // keyboard state of virtual keys
  HKL keyboard_layout;      // keyboard layout needed for translation (unused)
};

void tippse_detect_keyboard_layout(struct tippse_window* base) {
  base->keyboard_layout = GetKeyboardLayout(GetCurrentThreadId());
}

LRESULT CALLBACK tippse_wndproc(HWND window, UINT message, WPARAM param1, LPARAM param2) {
  struct tippse_window* base = (struct tippse_window*)GetWindowLongPtr(window, GWLP_USERDATA);
  if (message==WM_CREATE) {
    CREATESTRUCT* data = (CREATESTRUCT*)param2;
    base = (struct tippse_window*)data->lpCreateParams;
    base->font = CreateFont(16, 8, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, /*"Lucida Console"*/"FixedSys");
    tippse_detect_keyboard_layout(base);
    memset(&base->keystate[0], 0, sizeof(base->keystate));
    SetTimer(window, 100, 100, NULL);
    return 0;
  //} else if (message==WM_ERASEBKGND) {
  //  return 0;
  } else if (message==WM_INPUTLANGCHANGE) {
    tippse_detect_keyboard_layout(base);
  } else if (message==WM_TIMER) {
    editor_tick(base->editor);
  } else if (message==WM_SIZE) {
    screen_check(base->screen);
    editor_draw(base->editor);
  } else if (message==WM_PAINT) {
    PAINTSTRUCT ps;
    BeginPaint(window, &ps);
    SelectObject(ps.hdc, base->font);
    //TextOutA(ps.hdc, 0, 0, "Bla", 3);
    screen_draw(base->screen, ps.hdc, base->font, 8, 16);
    EndPaint(window, &ps);
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
    //printf("%d -> %d %d %d\r\n", param1, ret, key, (int)cp);
    editor_keypress(base->editor, key, (int)cp, 0, 0, 0, 0);
    editor_draw(base->editor);
  } else if (message==WM_KEYUP) {
    base->keystate[param1] = 0;
  } else if (message==WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(window, message, param1, param2);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, char* command_line, int show) {
  char* base_path = realpath(".", NULL);

  printf("Base: %s\r\n", base_path);
  unicode_init();

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
  base.editor = editor_create(base_path, base.screen, 0, NULL);

  HWND window = CreateWindowEx(0, wndclass.lpszClassName, "Tippse", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, wndclass.hInstance, &base);
  base.screen->window = window;
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

  return 0;
}
#endif
