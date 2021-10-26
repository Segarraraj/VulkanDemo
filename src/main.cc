#include "render.h"

bool running = true;

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  switch (message)
  {
  case WM_CLOSE: {
    running = false;
    break;
  }
  default: {
    result = DefWindowProc(window, message, wParam, lParam);
    break;
  }
  }
  return result;
}

int main(int argc, char** argv)
{
  HINSTANCE instance = GetModuleHandle(NULL);
  // Creating Windows window
  WNDCLASS window_class = {};

  window_class.hInstance = instance;
  window_class.lpfnWndProc = WindowProc;
  window_class.lpszClassName = "Main Class";
  window_class.hIcon = (HICON) LoadImage(
    instance, "../../data/icon.ico", 
    IMAGE_ICON, 32, 32, LR_LOADFROMFILE
  );

  RegisterClass(&window_class);

  HWND window = CreateWindowEx(
    0,
    "Main Class",
    "Vulkan Demo",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL,
    NULL,
    instance,
    NULL
  );

  Render render;
  if (!render.init(window, instance)) {
    return 0;
  };

  ShowWindow(window, 1);

  while (running)
  {
    MSG message;
    while (PeekMessage(&message, window, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }
  }

  return 1;
}