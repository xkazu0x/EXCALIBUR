#include "excalibur.h"

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <timeapi.h>
#include <xinput.h>

/*
  TODO(xkazu0x):
  > key codes for keyboard input
  > sound system
  > hardware 
*/

struct win32_game_code {
    b32 loaded;
    HMODULE library;
    FILETIME last_write_time;
    GAMEUPDATEANDRENDER *update_and_render;
};

// TODO(xkazu0x): maybe split??
struct win32_state {
    BITMAPINFO bitmap_info;
    WINDOWPLACEMENT window_placement;
    s64 time_frequency;
};

global b32 global_quit;
global win32_state global_win32;

global game_memory global_memory;
global game_input global_input;
global game_backbuffer global_backbuffer;

#define WIN32_GET_PROC_ADDR(v, m, s) (*(PROC*)(&(v))) = GetProcAddress((m), (s))

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_STATE *)
X_INPUT_GET_STATE(_xinput_get_state) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
typedef X_INPUT_GET_STATE(XINPUTGETSTATE);
XINPUTGETSTATE *xinput_get_state;

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_VIBRATION *)
X_INPUT_SET_STATE(_xinput_set_state) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
typedef X_INPUT_SET_STATE(XINPUTSETSTATE);
XINPUTSETSTATE *xinput_set_state;

DEBUG_PLATFORM_FREE_FILE(debug_platform_free_file) {
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_FILE(debug_platform_read_file) {
    debug_file_handle result = {};
    
    HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file_handle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(file_handle, &file_size)) {
            u32 file_size32 = safe_truncate_u64(file_size.QuadPart);
            void *file_memory = VirtualAlloc(0, file_size32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (file_memory) {
                DWORD bytes_read;
                if (ReadFile(file_handle, file_memory, file_size32, &bytes_read, 0) &&
                    (file_size32 == bytes_read)) {
                    result.size = file_size32;
                    result.memory = file_memory;
                } else {
                    // TODO(xkazu0x): logging
                    debug_platform_free_file(file_memory);
                    file_memory = 0;
                }
            } else {
                // TODO(xkazu0x): logging
            }
        } else {
            // TODO(xkazu0x): logging
        }
        CloseHandle(file_handle);
    } else {
        // TODO(xkazu0x): logging
    }
    
    return(result);
}

DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file) {
    b32 result = EX_FALSE;
    HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (file_handle != INVALID_HANDLE_VALUE) {
        DWORD bytes_written;
        if (WriteFile(file_handle, memory, memory_size, &bytes_written, 0)) {
            result = (bytes_written == memory_size);
        } else {
            // TODO(xkazu0x): logging
        }
        CloseHandle(file_handle);
    } else {
        // TODO(xkazu0x): logging
    }
    return(result);
}

inline FILETIME
win32_get_last_write_time(char *filename) {
    FILETIME result = {};
    WIN32_FIND_DATA find_data;
    HANDLE file_handle = FindFirstFileA(filename, &find_data);
    if (file_handle != INVALID_HANDLE_VALUE) {
        result = find_data.ftLastWriteTime;
        FindClose(file_handle);
    }
    return(result);
}

internal win32_game_code
win32_load_game_code(char *source_dll_name, char *temp_dll_name) {
    CopyFile(source_dll_name, temp_dll_name, FALSE);
    
    win32_game_code result = {};
    result.last_write_time = win32_get_last_write_time(source_dll_name);
    result.library = LoadLibraryA(temp_dll_name);
    if (result.library) {
        WIN32_GET_PROC_ADDR(result.update_and_render,
                            result.library,
                            "game_update_and_render");
        if (result.update_and_render) {
            result.loaded = EX_TRUE;
            EXDEBUG("> Game code loaded");
        }
    } else {
        EXERROR("< Failed to load game dll ");
    }
    if (!result.loaded) {
        result.update_and_render = _game_update_and_render;
        EXERROR("< Failed to load game code");
    }
    return(result);
}

internal void
win32_unload_game_code(win32_game_code *game_code) {
    if (game_code->library) {
        FreeLibrary(game_code->library);
        game_code->library = 0;
        EXDEBUG("> Game code unloaded");
    }
    game_code->loaded = EX_FALSE;
    game_code->update_and_render = _game_update_and_render;
}

inline LARGE_INTEGER
win32_get_time(void) {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

inline f32
win32_get_delta_seconds(LARGE_INTEGER start, LARGE_INTEGER end) {
    f32 result = ((f32)(end.QuadPart - start.QuadPart)) / ((f32)(global_win32.time_frequency));
    return(result);
}

internal void
win32_process_digital_button(digital_button *button, b32 down) {
    b32 was_down = button->down;
    button->pressed = !was_down && down;
    button->released = was_down && !down;
    button->down = down;
}

internal void
win32_process_analog_button(analog_button *button, f32 value) {
    button->value = value;
    b32 was_down = button->down;
    button->down = (value >= button->threshold);
    button->pressed = !was_down && button->down;
    button->released = was_down && !button->down;
}

internal void
win32_process_stick(stick *stick, f32 x, f32 y) {
    if (abs_f32(x) <= stick->threshold) x = 0.0f;
    if (abs_f32(y) <= stick->threshold) y = 0.0f;
    stick->x = x;
    stick->y = y;
}

internal void
win32_window_toggle_fullscreen(HWND window, WINDOWPLACEMENT *placement) {
    DWORD window_style = GetWindowLong(window, GWL_STYLE);
    if (window_style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO monitor_info = {};
        monitor_info.cbSize = sizeof(monitor_info);
        if (GetWindowPlacement(window, placement) &&
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) {
            SetWindowLong(window, GWL_STYLE, (window_style & ~(WS_OVERLAPPEDWINDOW)) | WS_POPUP);
            SetWindowPos(window, HWND_TOP,
                         monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLongPtr(window, GWL_STYLE, (window_style & ~(WS_POPUP)) | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, placement);
        SetWindowPos(window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal vec2i
win32_get_window_size(HWND window) {
    RECT client_rectangle;
    GetClientRect(window, &client_rectangle);
    vec2i result;
    result.x = client_rectangle.right - client_rectangle.left;
    result.y = client_rectangle.bottom - client_rectangle.top;
    return(result);
}

internal void
win32_resize_backbuffer(game_backbuffer *buffer, vec2i size) {
    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    s32 bytes_per_pixel = 4;
    buffer->size = size;
    buffer->pitch = buffer->size.x * bytes_per_pixel;
    
    global_win32.bitmap_info.bmiHeader.biSize = sizeof(global_win32.bitmap_info.bmiHeader);
    global_win32.bitmap_info.bmiHeader.biWidth = buffer->size.x;
    global_win32.bitmap_info.bmiHeader.biHeight = -buffer->size.y;
    global_win32.bitmap_info.bmiHeader.biPlanes = 1;
    global_win32.bitmap_info.bmiHeader.biBitCount = 32;
    global_win32.bitmap_info.bmiHeader.biCompression = BI_RGB;

    s32 bitmap_memory_size = (buffer->size.x * buffer->size.y) * bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void
win32_display_backbuffer(game_backbuffer buffer, vec2i window_size, HDC device_context) {
    StretchDIBits(device_context,
                  0, 0, window_size.x, window_size.y,
                  0, 0, buffer.size.x, buffer.size.y,
                  buffer.memory,
                  &global_win32.bitmap_info,
                  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK
win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch (message) {
        case WM_CLOSE:
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        case WM_SIZE: {
            vec2i window_size = win32_get_window_size(window);
            win32_resize_backbuffer(&global_backbuffer, window_size);
        } break;
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        }
    }
    return(result);
}

internal void
cat_strings(size_t src_a_count, char *src_a,
            size_t src_b_count, char *src_b,
            size_t dest_count, char *dest) {
    for (u32 i = 0; i < src_a_count; ++i) {
        *dest++ = *src_a++;
    }
    for (u32 i = 0; i < src_b_count; ++i) {
        *dest++ = *src_b++;
    }
    *dest++ = 0;
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR command_line, int show_command) {
    char exe_filename[MAX_PATH];
    DWORD size_of_filename = GetModuleFileName(0, exe_filename, sizeof(exe_filename));
    char *one_past_last_slash = exe_filename;
    for (char *scan = exe_filename; *scan; ++scan) {
        if (*scan == '\\') {
            one_past_last_slash = scan + 1;
        }
    }

    char source_game_code_dll_filename[] = "excalibur.dll";
    char source_game_code_dll_fullpath[MAX_PATH];
    cat_strings(one_past_last_slash - exe_filename, exe_filename,
                sizeof(source_game_code_dll_filename) - 1, source_game_code_dll_filename,
                sizeof(source_game_code_dll_fullpath), source_game_code_dll_fullpath);

    char temp_game_code_dll_filename[] = "excalibur_temp.dll";
    char temp_game_code_dll_fullpath[MAX_PATH];
    cat_strings(one_past_last_slash - exe_filename, exe_filename,
                sizeof(temp_game_code_dll_filename) - 1, temp_game_code_dll_filename,
                sizeof(temp_game_code_dll_fullpath), temp_game_code_dll_fullpath);
    
    EXINFO("EXCALIBUR : INITIALIZE");
    EXINFO("> Operating system: %s", string_from_operating_system(operating_system_from_context()));
    EXINFO("> Architecture: %s", string_from_architecture(architecture_from_context()));
    
    DEVMODE monitor_info;
    monitor_info.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &monitor_info);
    
    //vec2i screen_size = vec2i_create(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    vec2i monitor_size = vec2i_create(monitor_info.dmPelsWidth, monitor_info.dmPelsHeight);
    s32 monitor_frame_rate = monitor_info.dmDisplayFrequency;
    EXINFO("> Monitor size: %dx%d", monitor_size.x, monitor_size.y);
    EXINFO("> Monitor refresh rate: %dHz", monitor_frame_rate);
    
    EXINFO("EXCALIBUR :: WINDOW");
    char *window_title = "EXCALIBUR";
    vec2i window_size = vec2i_create(960, 540);
    vec2i window_position = (monitor_size - window_size) / 2;
    EXDEBUG("> Window size: %dx%d", window_size.x, window_size.y);
    
    RECT window_rectangle = {};
    window_rectangle.left = 0;
    window_rectangle.right = window_size.x;
    window_rectangle.top = 0;
    window_rectangle.bottom = window_size.y;
    if (AdjustWindowRect(&window_rectangle, WS_OVERLAPPEDWINDOW, 0)) {
        window_size.x = window_rectangle.right - window_rectangle.left;
        window_size.y = window_rectangle.bottom - window_rectangle.top;
    }

    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = "EXCALIBUR_WINDOW";
    ATOM window_atom = RegisterClassA(&window_class);
    if (!window_atom) {
        EXFATAL("< Failed to register win32 window");
        return(1);
    }
    EXDEBUG("> Win32 window registered");
    
    HWND window_handle = CreateWindowExA(0, MAKEINTATOM(window_atom),
                                         window_title, WS_OVERLAPPEDWINDOW,
                                         window_position.x, window_position.y,
                                         window_size.x, window_size.y,
                                         0, 0, instance, 0);
    if (!window_handle) {
        EXFATAL("< Failed to create win32 window");
        return(1);
    }
    EXDEBUG("> Win32 window created");
    HDC window_device = GetDC(window_handle);
    ShowWindow(window_handle, SW_SHOW);

    EXINFO("EXCALIBUR :: INPUT");
    RAWINPUTDEVICE raw_input_device = {};
    raw_input_device.usUsagePage = 0x01;
    raw_input_device.usUsage = 0x02;
    raw_input_device.hwndTarget = window_handle;
    if (!RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device))) {
        EXERROR("< Failed to register mouse as raw input device");
        return(1);
    }
    EXDEBUG("> Mouse registered as raw input device");
    
    HMODULE xinput_library = LoadLibraryA("xinput1_4.dll");
    if (!xinput_library) {
        EXERROR("< Failed to load 'xinput1_4.dll'");
        xinput_library = LoadLibraryA("xinput1_3.dll");
        if (!xinput_library) {
            EXERROR("< Failed to load 'xinput1_3.dll'");
            xinput_library = LoadLibraryA("xinput9_1_0.dll");
            if (!xinput_library) {
                EXERROR("< Failed to load 'xinput9_1_0.dll'");
            } else {
                EXDEBUG("> Successfully loaded 'xinput9_1_0.dll'");
            }
        } else {
            EXDEBUG("> Successfully loaded 'xinput1_3.dll'");
        }
    } else {
        EXDEBUG("> Successfully loaded 'xinput1_4.dll'");
    }
    
    if (xinput_library) {
        WIN32_GET_PROC_ADDR(xinput_get_state, xinput_library, "XInputGetState");
        WIN32_GET_PROC_ADDR(xinput_set_state, xinput_library, "XInputSetState");
    } else {
        xinput_get_state = _xinput_get_state;
        xinput_set_state = _xinput_set_state;
    }

    // TODO(xkazu0x): is that a better way to do this?
    // every gamepads has the same constant values
    f32 trigger_threshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;
    for (u32 i = 0; i < EX_ARRAY_COUNT(global_input.gamepads); i++) {
        global_input.gamepads[i].left_trigger.threshold = trigger_threshold;
        global_input.gamepads[i].right_trigger.threshold = trigger_threshold;
        global_input.gamepads[i].left_stick.threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
        global_input.gamepads[i].right_stick.threshold = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    }

    ///////////////////////////////////
    // NOTE(xkazu0x): memory initialize
    EXINFO("EXCALIBUR :: MEMORY");
#if EXCALIBUR_INTERNAL
    LPVOID base_address = (LPVOID)EX_TERABYTES(2);
#else
    LPVOID base_address = 0;
#endif
    global_memory.permanent_storage_size = EX_MEGABYTES(64);
    global_memory.transient_storage_size = EX_GIGABYTES(2);
    global_memory.debug_platform_free_file = debug_platform_free_file;
    global_memory.debug_platform_read_file = debug_platform_read_file;
    global_memory.debug_platform_write_file = debug_platform_write_file;
    
    u64 total_memory_size = global_memory.permanent_storage_size + global_memory.transient_storage_size;
    global_memory.permanent_storage = VirtualAlloc(base_address, total_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    global_memory.transient_storage = ((u8 *)global_memory.permanent_storage + global_memory.permanent_storage_size);
    if (!global_memory.permanent_storage) {
        EXFATAL("< Failed to allocate game memory");
        return(1);
    } else {
        EXDEBUG("> Memory allocated:");
        EXTRACE("\t^ Permanent storage: %llu", global_memory.permanent_storage_size);
        EXTRACE("\t^ Transient storage: %llu", global_memory.transient_storage_size);
    }
    
    /////////////////////////////////
    // NOTE(xkazu0x): time initialize
    LARGE_INTEGER large_integer;
    QueryPerformanceFrequency(&large_integer);
    global_win32.time_frequency = large_integer.QuadPart;

    // NOTE(xkazu0x): set the windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular
    UINT desired_scheduler_ms = 1;
    b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
    
    LARGE_INTEGER last_counter = win32_get_time();
    s64 last_cycle_count = __rdtsc();

    f32 target_seconds_per_frame = 1.0f / ((f32)(monitor_frame_rate));

    //////////////////////////////
    // NOTE(xkazu0x): game loading
    EXINFO("EXCALIBUR :: GAME");
    win32_game_code game_code = win32_load_game_code(source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
    if (!game_code.loaded) {
        return(1);
    }

    EXINFO("EXCALIBUR : RUN");
    while (!global_quit) {
        // NOTE(xkazu0x): reload game code
        FILETIME game_code_dll_write_time = win32_get_last_write_time(source_game_code_dll_fullpath);
        if (CompareFileTime(&game_code_dll_write_time, &game_code.last_write_time) != 0) {
            win32_unload_game_code(&game_code);
            game_code = win32_load_game_code(source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
        }

        // NOTE(xkazu0x): message loop
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            switch (message.message) {
                case WM_QUIT: {
                    global_quit = EX_TRUE;
                } break;
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP: {
                    u32 key_code = (u32)message.wParam;
                    b32 down = ((message.lParam & (1 << 31)) == 0);
                    win32_process_digital_button(&global_input.keyboard[key_code], down);
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                } break;
                case WM_INPUT: {
                    UINT size;
                    GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER));
                    char *buffer = (char *)_alloca(size);
                    if (GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size) {
                        RAWINPUT *raw_input = (RAWINPUT *)buffer;
                        if (raw_input->header.dwType == RIM_TYPEMOUSE && raw_input->data.mouse.usFlags == MOUSE_MOVE_RELATIVE){
                            global_input.mouse.delta_position.x += raw_input->data.mouse.lLastX;
                            global_input.mouse.delta_position.y += raw_input->data.mouse.lLastY;

                            USHORT button_flags = raw_input->data.mouse.usButtonFlags;
                            b32 left_button_down = global_input.mouse.left.down;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_DOWN) left_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_UP) left_button_down = EX_FALSE;
                            win32_process_digital_button(&global_input.mouse.left, left_button_down);

                            b32 right_button_down = global_input.mouse.right.down;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_down = EX_FALSE;
                            win32_process_digital_button(&global_input.mouse.right, right_button_down);
                            
                            b32 middle_button_down = global_input.mouse.middle.down;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) middle_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_UP) middle_button_down = EX_FALSE;
                            win32_process_digital_button(&global_input.mouse.middle, middle_button_down);

                            if (raw_input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                                global_input.mouse.delta_wheel += ((SHORT)raw_input->data.mouse.usButtonData) / WHEEL_DELTA;
                            }
                        }
                    }
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                } break;
                default: {
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }
            }
        }
        
        if (global_input.keyboard[VK_ESCAPE].pressed) {
            global_quit = EX_TRUE;
        }
        if (global_input.keyboard[VK_F11].pressed) {
            win32_window_toggle_fullscreen(window_handle, &global_win32.window_placement);
        }
        
        //////////////////////////////
        // NOTE(xkazu0x): mouse update
#if 0
        global_input.mouse.position.x += global_input.mouse.delta_position.x;
        global_input.mouse.position.y += global_input.mouse.delta_position.y;
        global_input.mouse.wheel += global_input.mouse.delta_wheel;

        POINT mouse_position;
        GetCursorPos(&mouse_position); // this slow down
        ScreenToClient(window_handle, &mouse_position);

        if (mouse_position.x < 0) mouse_position.x = 0;
        if (mouse_position.y < 0) mouse_position.y = 0;
        if (mouse_position.x > window_size.x) mouse_position.x = window_size.x;
        if (mouse_position.y > window_size.y) mouse_position.y = window_size.y;
        global_input.mouse.position.x = mouse_position.x;
        global_input.mouse.position.y = mouse_position.y;
#endif

        ////////////////////////////////
        // NOTE(xkazu0x): gamepad update
        u32 max_gamepad_count = XUSER_MAX_COUNT;
        if (max_gamepad_count > EX_ARRAY_COUNT(global_input.gamepads)) {
            max_gamepad_count = EX_ARRAY_COUNT(global_input.gamepads);
        }
        for (u32 i = 0; i < max_gamepad_count; i++) {
            XINPUT_STATE xinput_state = {};
            DWORD xinput_result = xinput_get_state(i, &xinput_state);
            if (xinput_result == ERROR_SUCCESS) {
                win32_process_digital_button(&global_input.gamepads[i].up, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
                win32_process_digital_button(&global_input.gamepads[i].down, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
                win32_process_digital_button(&global_input.gamepads[i].left, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
                win32_process_digital_button(&global_input.gamepads[i].right, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
                win32_process_digital_button(&global_input.gamepads[i].start, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
                win32_process_digital_button(&global_input.gamepads[i].back, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
                win32_process_digital_button(&global_input.gamepads[i].left_thumb,(xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
                win32_process_digital_button(&global_input.gamepads[i].right_thumb, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
                win32_process_digital_button(&global_input.gamepads[i].left_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
                win32_process_digital_button(&global_input.gamepads[i].right_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
                win32_process_digital_button(&global_input.gamepads[i].a, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
                win32_process_digital_button(&global_input.gamepads[i].b, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
                win32_process_digital_button(&global_input.gamepads[i].x, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
                win32_process_digital_button(&global_input.gamepads[i].y, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
                win32_process_analog_button(&global_input.gamepads[i].left_trigger, xinput_state.Gamepad.bLeftTrigger / 255.0f);
                win32_process_analog_button(&global_input.gamepads[i].right_trigger, xinput_state.Gamepad.bRightTrigger / 255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
                win32_process_stick(&global_input.gamepads[i].left_stick, CONVERT(xinput_state.Gamepad.sThumbLX), CONVERT(xinput_state.Gamepad.sThumbLY));
                win32_process_stick(&global_input.gamepads[i].right_stick, CONVERT(xinput_state.Gamepad.sThumbRX), CONVERT(xinput_state.Gamepad.sThumbRY));
#undef CONVERT
            } else {
                break;
            }
        }

        ///////////////////////////////////
        // NOTE(xkazu0x): update and render
        game_code.update_and_render(&global_memory, &global_input, &global_backbuffer);
        //window_size = win32_get_window_size(window_handle);
        //win32_display_backbuffer(global_backbuffer, window_size, window_device);
        
        /////////////////////////////
        // NOTE(xkazu0x): time update
        s64 end_cycle_count = __rdtsc();
        s64 cycles_per_frame = end_cycle_count - last_cycle_count;
        
        LARGE_INTEGER work_counter = win32_get_time();
        f32 work_seconds_per_frame = win32_get_delta_seconds(last_counter, work_counter);

        // TODO(xkazu0x): PROBRABLY BUGGY
        f32 seconds_elapsed_for_frame = work_seconds_per_frame;
        if (seconds_elapsed_for_frame < target_seconds_per_frame) {
            if (sleep_is_granular) {
                DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - seconds_elapsed_for_frame));
                if (sleep_ms > 0) {
                    Sleep(sleep_ms);
                }
            }
            while (seconds_elapsed_for_frame < target_seconds_per_frame) {
                seconds_elapsed_for_frame = win32_get_delta_seconds(last_counter, win32_get_time());
            }
        } else {
            EXWARN("< FRAME RATE MISSED");
        }
        
        ////////////////////////////////////////////
        // TODO(xkazu0x): display backbuffer here????
        window_size = win32_get_window_size(window_handle);
        win32_display_backbuffer(global_backbuffer, window_size, window_device);
        
        LARGE_INTEGER end_counter = win32_get_time();
        f32 ms_per_frame = 1000.0f * win32_get_delta_seconds(last_counter, end_counter);
        
        s64 counts_per_frame = last_counter.QuadPart - end_counter.QuadPart;
        f32 frames_per_second = (f32)global_win32.time_frequency / (f32)counts_per_frame;

        f32 mega_cycles_per_frame = ((f32)cycles_per_frame / (1000.0f * 1000.0f));
        
        last_counter = end_counter;
        last_cycle_count = end_cycle_count;
                
#if 1
        // TODO(xkazu0x): this is debug code only
        local f64 time_seconds = 0.0;
        local f64 last_print_time = 0.0;
        time_seconds += work_seconds_per_frame;
        if (time_seconds - last_print_time > 0.1f) {
            EXDEBUG("%.02fms, %.02ffps, %.02fmc", ms_per_frame, frames_per_second, mega_cycles_per_frame);
            char new_window_title[512];
            sprintf(new_window_title, "%s - %.02fms, %.02ffps, %.02fmc", window_title, ms_per_frame, frames_per_second, mega_cycles_per_frame);
            SetWindowTextA(window_handle, new_window_title);
            last_print_time = time_seconds;
        }
#endif

        /////////////////////////////
        // NOTE(xkazu0x): input reset
        for (u32 i = 0; i < KEY_MAX; i++) {
            global_input.keyboard[i].pressed = EX_FALSE;
            global_input.keyboard[i].released = EX_FALSE;
        }
        
        global_input.mouse.left.pressed = EX_FALSE;
        global_input.mouse.left.released = EX_FALSE;
        global_input.mouse.right.pressed = EX_FALSE;
        global_input.mouse.right.released = EX_FALSE;
        global_input.mouse.middle.pressed = EX_FALSE;
        global_input.mouse.middle.released = EX_FALSE;
        global_input.mouse.delta_position.x = 0;
        global_input.mouse.delta_position.y = 0;
        global_input.mouse.delta_wheel = 0;
    }
    
    EXINFO("EXCALIBUR : SHUTDOWN");
    ReleaseDC(window_handle, window_device);
    UnregisterClassA(MAKEINTATOM(window_atom), instance);
    DestroyWindow(window_handle);
    return(0);
}
