#include "render.h"

bool running = true;

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT result = 0;
  switch (message)
  {
  // WINDOW RESIZED
  case WM_SIZE: {
    Render* render = (Render*) GetWindowLongPtr(window, GWLP_USERDATA);
    //render->_resize = true;
    break;
  }
  // WINDOW CLOSED
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
  // For fully win32 inmersive experience see: WinMain.
  // Since we are not using WinMain we have to get the instance by our own. 
  HINSTANCE instance = GetModuleHandle(NULL);

  WNDCLASS window_class = {};

  window_class.hInstance = instance;
  window_class.lpfnWndProc = WindowProc;
  window_class.lpszClassName = "Main Class";

  // Optional field
  window_class.hIcon = (HICON) LoadImage(
    instance, "../../data/icon.ico", 
    IMAGE_ICON, 32, 32, LR_LOADFROMFILE
  );

  RegisterClass(&window_class);

  HWND window = CreateWindowEx(
    0,
    "Main Class",
    "Vulkan Demo",
    /*(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX),*/ WS_OVERLAPPEDWINDOW,
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

  SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) &render);

  ShowWindow(window, 1);

  while (running)
  {
    MSG message;
    while (PeekMessage(&message, window, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    render.drawFrame();
  }

  vkDeviceWaitIdle(render._device);

  return 1;
}