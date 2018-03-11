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
};

LRESULT CALLBACK tippse_wndproc(HWND window, UINT message, WPARAM param1, LPARAM param2) {
  struct tippse_window* params = (struct tippse_window*)GetWindowLongPtr(window, GWLP_USERDATA);
  if (message==WM_CREATE) {
    CREATESTRUCT* data = (CREATESTRUCT*)param2;
    params = (struct tippse_window*)data->lpCreateParams;
    params->font = CreateFont(16, 8, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Lucida Console");
    return 0;
  //} else if (message==WM_ERASEBKGND) {
  //  return 0;
  } else if (message==WM_PAINT) {
    PAINTSTRUCT ps;
    BeginPaint(window, &ps);
    SelectObject(ps.hdc, params->font);
    TextOutA(ps.hdc, 0, 0, "Bla", 3);
    EndPaint(window, &ps);
  } else if (message==WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(window, message, param1, param2);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, char* command_line, int show) {
  char* base_path = realpath(".", NULL);

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

  struct tippse_window params;
  params.screen = screen_create();
  params.editor = editor_create(base_path, params.screen, 0, NULL);

  HWND window = CreateWindowEx(0, wndclass.lpszClassName, "Tippse", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, wndclass.hInstance, &params);
  SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)&params);
  ShowWindow(window, show);

  MSG msg;
  while (1) {
    BOOL ret = GetMessage(&msg, window, 0, 0);
    if (ret==-1 || ret==0) {
      break;
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  DestroyWindow(window);
  UnregisterClass(wndclass.lpszClassName, wndclass.hInstance);

  editor_destroy(params.editor);
  screen_destroy(params.screen);
  clipboard_free();
  unicode_free();
  free(base_path);

  return 0;
}
#endif
