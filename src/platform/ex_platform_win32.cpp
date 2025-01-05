#undef function
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define function static

///////////////////////////////////
// NOTE(xkazu0x): platform specific

struct win32_context {
    ATOM atom;
    HWND window;
    HINSTANCE instance;
    WINDOWPLACEMENT window_placement;
} global _win32;

LRESULT CALLBACK
win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch (message) {
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        }
    }
    return(result);
}

//////////////////////////////////
// NOTE(xkazu0x): window functions

function bool8
platform_window_create(platform_context *platform) {
    if (!platform->window.title) platform->window.title = "EXCALIBUR";
    
    int32 window_width = 0;
    if (platform->window.size.x) window_width = platform->window.size.x;
    else window_width = CW_USEDEFAULT;

    int32 window_height = 0;
    if (platform->window.size.y) window_height = platform->window.size.y;
    else window_height = CW_USEDEFAULT;

    int32 window_x = 0;
    if (platform->window.position.x) window_x = platform->window.position.x;
    else window_x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;

    int32 window_y = 0;
    if (platform->window.position.y) window_y = platform->window.position.y;
    else window_y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;

    RECT window_rectangle;
    window_rectangle.left = 0;
    window_rectangle.right = window_width;
    window_rectangle.top = 0;
    window_rectangle.bottom = window_height;
    if (AdjustWindowRect(&window_rectangle, WS_OVERLAPPEDWINDOW, 0)) {
        window_width = window_rectangle.right - window_rectangle.left;
        window_height = window_rectangle.bottom - window_rectangle.top;
    }

    _win32.instance = GetModuleHandleA(nullptr);
    
    WNDCLASSA window_class;
    EX_MEMORY_ZERO_STRUCT(&window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = _win32.instance;
    window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = "EXCALIBUR";
    _win32.atom = RegisterClassA(&window_class);
    if (!_win32.atom) {
        EXFATAL("Failed to register win32 window.");
        return(EX_FALSE);
    }
    //EX_ASSERT(_win32.atom);

    _win32.window = CreateWindow(MAKEINTATOM(_win32.atom),
                                 platform->window.title,
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 window_x, window_y,
                                 window_width, window_height,
                                 0, 0, _win32.instance, 0);
    if (!_win32.window) {
        EXFATAL("Failed to create win32 window.");
        return(EX_FALSE);
    }
    //EX_ASSERT(_win32.window);
    return(EX_TRUE);
}

function void
platform_window_pull(platform_context *platform) {
    MSG msg;
    EX_MEMORY_ZERO_STRUCT(&msg);
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        switch(msg.message) {
            case WM_QUIT: {
                UnregisterClassA(MAKEINTATOM(_win32.atom), _win32.instance);
                platform->quit = EX_TRUE;
            } break;
            case WM_KEYDOWN: {
                switch(msg.wParam) {
                    case VK_ESCAPE: {
                        DestroyWindow(_win32.window);
                    } break;
                    case VK_F11: {
                        platform_window_toggle_fullscreen(platform);
                    } break;
                }
            } break;
            default: {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    RECT client_rectangle;
    GetClientRect(_win32.window, &client_rectangle);
    platform->window.size.x = client_rectangle.right - client_rectangle.left;
    platform->window.size.y = client_rectangle.bottom - client_rectangle.top;

    POINT window_position = { client_rectangle.left, client_rectangle.top };
    ClientToScreen(_win32.window, &window_position);
    platform->window.position.x = window_position.x;
    platform->window.position.y = window_position.y;
}

function void
platform_window_toggle_fullscreen(platform_context *platform) {
    DWORD window_style = GetWindowLong(_win32.window, GWL_STYLE);
    if (window_style & WS_OVERLAPPEDWINDOW) {
        platform->window.fullscreen = EX_TRUE;
        MONITORINFO monitor_info;
        EX_MEMORY_ZERO_STRUCT(&monitor_info);
        monitor_info.cbSize = sizeof(monitor_info);
        if (GetWindowPlacement(_win32.window, &_win32.window_placement) &&
            GetMonitorInfo(MonitorFromWindow(_win32.window, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) {
            SetWindowLong(_win32.window, GWL_STYLE, (window_style & ~(WS_OVERLAPPEDWINDOW)) | WS_POPUP);
            SetWindowPos(_win32.window, HWND_TOP,
                         monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        platform->window.fullscreen = EX_FALSE;
        SetWindowLongPtr(_win32.window, GWL_STYLE, (window_style & ~(WS_POPUP)) | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(_win32.window, &_win32.window_placement);
        SetWindowPos(_win32.window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}
