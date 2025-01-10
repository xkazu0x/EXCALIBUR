#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct platform_internal {
    HINSTANCE instance;
    ATOM atom;
    HWND window;
};

internal LRESULT CALLBACK
win32_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
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

internal bool32
platform_initialize(platform_context *platform, platform_desc *description) {
    platform->internal_state = VirtualAlloc(0, sizeof(platform_internal), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    platform_internal *platform_state = (platform_internal *)platform->internal_state;

    if (!description->name) description->name = "EXCALIBUR";
    
    int32 window_width = 0;
    int32 window_height = 0;
    int32 window_x = 0;
    int32 window_y = 0;
    if (description->screen_size.x && description->screen_size.y) {
        window_width = description->screen_size.x;
        window_height = description->screen_size.y;
        window_x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;
        window_y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;
    } else {
        window_width = CW_USEDEFAULT;
        window_height = CW_USEDEFAULT;
        window_x = CW_USEDEFAULT;
        window_y = CW_USEDEFAULT;
    }
        
    platform_state->instance = GetModuleHandleA(0);
    
    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_window_procedure;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = platform_state->instance;
    window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = "EXCALIBUR_WINDOW_CLASS";
    platform_state->atom = RegisterClassA(&window_class);
    if (!platform_state->atom) {
        EXFATAL("Failed to register win32 window.");
        return(EX_FALSE);
    }

    platform_state->window = CreateWindow(MAKEINTATOM(platform_state->atom),
                                             description->name,
                                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                             window_x, window_y,
                                             window_width, window_height,
                                             0, 0, platform_state->instance, 0);
    if (!platform_state->window) {
        EXFATAL("Failed to create win32 window.");
        return(EX_FALSE);
    }

    platform->initialized = EX_TRUE;
    return(EX_TRUE);
}

internal void
platform_pull(platform_context *platform) {
    if (!platform->initialized) {
        EXERROR("< The platform needs to be initialized.");
        platform->quit = EX_TRUE;
        return;
    }
    platform_internal *platform_state = (platform_internal *)platform->internal_state;
    
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        switch (message.message) {
            case WM_QUIT: {
                UnregisterClassA(MAKEINTATOM(platform_state->atom), platform_state->instance);
                platform->quit = EX_TRUE;
            } break;
            default: {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }
    }
}
