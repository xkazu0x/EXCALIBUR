#include "excalibur.h"
#include "excalibur_win32.h"

/*
  TODO(xkazu0x):
  > key codes for keyboard input
  > sound system
  > hardware 
*/

global win32_state g_win32;

global game_memory g_memory;
global game_input g_input;
global game_bitmap g_bitmap;

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
                    debug_platform_free_file(thread, file_memory);
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
    FILETIME last_write_time = {};
    
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data)) {
        last_write_time = data.ftLastWriteTime;
    }
    
    return(last_write_time);
}

internal win32_game_code
win32_load_game_code(char *source_dll_name, char *temp_dll_name) {
    win32_game_code result = {};
    result.last_write_time = win32_get_last_write_time(source_dll_name);

    CopyFile(source_dll_name, temp_dll_name, FALSE);
    result.library = LoadLibraryA(temp_dll_name);
    if (result.library) {
        WIN32_GET_PROC_ADDR(result.update_and_render, result.library, "game_update_and_render");
        if (result.update_and_render) {
            result.loaded = EX_TRUE;
            EXDEBUG("> Game code loaded");
        }
    } else {
        EXERROR("< Failed to load game dll");
    }
    
    if (!result.loaded) {
        result.update_and_render = 0;
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
    game_code->update_and_render = 0;
}

inline LARGE_INTEGER
win32_get_time(void) {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

inline f32
win32_get_delta_seconds(LARGE_INTEGER start, LARGE_INTEGER end) {
    f32 result = ((f32)(end.QuadPart - start.QuadPart)) / ((f32)(g_win32.time_frequency));
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
win32_resize_bitmap(game_bitmap *bitmap, vec2i size) {
    if (bitmap->memory) {
        VirtualFree(bitmap->memory, 0, MEM_RELEASE);
    }

    bitmap->size = size;
    bitmap->bytes_per_pixel = 4;
    bitmap->pitch = bitmap->size.x * bitmap->bytes_per_pixel;
    
    g_win32.bitmap_info.bmiHeader.biSize = sizeof(g_win32.bitmap_info.bmiHeader);
    g_win32.bitmap_info.bmiHeader.biWidth = bitmap->size.x;
    g_win32.bitmap_info.bmiHeader.biHeight = -bitmap->size.y;
    g_win32.bitmap_info.bmiHeader.biPlanes = 1;
    g_win32.bitmap_info.bmiHeader.biBitCount = 32;
    g_win32.bitmap_info.bmiHeader.biCompression = BI_RGB;

    s32 bitmap_memory_size = (bitmap->size.x * bitmap->size.y) * bitmap->bytes_per_pixel;
    bitmap->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void
win32_display_bitmap(game_bitmap bitmap, vec2i window_size, HDC device_context) {
    StretchDIBits(device_context,
                  //0, 0, window_size.x, window_size.y, NOTE(xkazu0x): stretch
                  0, 0, bitmap.size.x, bitmap.size.y,
                  0, 0, bitmap.size.x, bitmap.size.y,
                  bitmap.memory,
                  &g_win32.bitmap_info,
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
        case WM_ACTIVATEAPP: {
            if (wparam == TRUE) {
                SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);
            } else {
                SetLayeredWindowAttributes(window, RGB(0, 0, 0), 128, LWA_ALPHA);
            }
        } break;
        case WM_SIZE: {
            //vec2i window_size = win32_get_window_size(window);
            //win32_resize_bitmap(&g_bitmap, window_size);
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

internal void
win32_get_exe_filename(win32_state *state) {
    DWORD size_of_filename = GetModuleFileName(0, state->exe_filename, sizeof(state->exe_filename));
    state->one_past_last_exe_filename_slash = state->exe_filename;
    for (char *scan = state->exe_filename; *scan; ++scan) {
        if (*scan == '\\') {
            state->one_past_last_exe_filename_slash = scan + 1;
        }
    }
}

internal void
win32_build_exe_path_filename(win32_state *state, char *filename,
                              u32 dest_count, char *dest) {
    cat_strings(state->one_past_last_exe_filename_slash - state->exe_filename, state->exe_filename,
                string_length(filename), filename,
                dest_count, dest);
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR command_line, int show_command) {
    win32_get_exe_filename(&g_win32);
    
    char source_game_code_dll_fullpath[WIN32_STATE_FILENAME_MAX];
    win32_build_exe_path_filename(&g_win32, "excalibur.dll",
                                  sizeof(source_game_code_dll_fullpath),
                                  source_game_code_dll_fullpath);
    
    char temp_game_code_dll_fullpath[WIN32_STATE_FILENAME_MAX];
    win32_build_exe_path_filename(&g_win32, "excalibur_temp.dll",
                                  sizeof(temp_game_code_dll_fullpath),
                                  temp_game_code_dll_fullpath);
    
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
    window_class.style = CS_HREDRAW | CS_VREDRAW;
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

    u32 window_style = WS_OVERLAPPEDWINDOW;
#if 1
    u32 window_style_ex = 0;
#else
    u32 window_style_ex = WS_EX_TOPMOST | WS_EX_LAYERED;
#endif
    
    HWND window_handle = CreateWindowExA(window_style_ex, MAKEINTATOM(window_atom),
                                         window_title, window_style,
                                         window_position.x, window_position.y,
                                         window_size.x, window_size.y,
                                         0, 0, instance, 0);
    if (!window_handle) {
        EXFATAL("< Failed to create win32 window");
        return(1);
    }
    EXDEBUG("> Win32 window created");
    ShowWindow(window_handle, SW_SHOW);

    vec2i bitmap_size = win32_get_window_size(window_handle);
    win32_resize_bitmap(&g_bitmap, bitmap_size);
    
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
    for (u32 i = 0; i < EX_ARRAY_COUNT(g_input.gamepads); i++) {
        g_input.gamepads[i].left_trigger.threshold = trigger_threshold;
        g_input.gamepads[i].right_trigger.threshold = trigger_threshold;
        g_input.gamepads[i].left_stick.threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
        g_input.gamepads[i].right_stick.threshold = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    }

    ///////////////////////////////////
    // NOTE(xkazu0x): memory initialize
    EXINFO("EXCALIBUR :: MEMORY");
#if EXCALIBUR_INTERNAL
    LPVOID base_address = (LPVOID)EX_TERABYTES(2);
#else
    LPVOID base_address = 0;
#endif
    g_memory.permanent_storage_size = EX_MEGABYTES(64);
    g_memory.transient_storage_size = EX_GIGABYTES(1);
    g_memory.debug_platform_free_file = debug_platform_free_file;
    g_memory.debug_platform_read_file = debug_platform_read_file;
    g_memory.debug_platform_write_file = debug_platform_write_file;
    
    g_win32.game_memory_size = g_memory.permanent_storage_size + g_memory.transient_storage_size;
    g_win32.game_memory_block = VirtualAlloc(base_address, g_win32.game_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    g_memory.permanent_storage = g_win32.game_memory_block;
    g_memory.transient_storage = ((u8 *)g_memory.permanent_storage + g_memory.permanent_storage_size);
    if (!g_memory.permanent_storage) {
        EXFATAL("< Failed to allocate game memory");
        return(1);
    }
    EXDEBUG("> Memory allocated:");
    EXTRACE("\t> Permanent storage: %llu", g_memory.permanent_storage_size);
    EXTRACE("\t> Transient storage: %llu", g_memory.transient_storage_size);
    
    /////////////////////////////////
    // NOTE(xkazu0x): time initialize
    LARGE_INTEGER large_integer;
    QueryPerformanceFrequency(&large_integer);
    g_win32.time_frequency = large_integer.QuadPart;

    // NOTE(xkazu0x): set the windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular
    UINT desired_scheduler_ms = 1;
    b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
    
    LARGE_INTEGER last_counter = win32_get_time();
    s64 last_cycle_count = __rdtsc();

    f32 target_seconds_per_frame = 1.0f / ((f32)(monitor_frame_rate));

    ///////////////////////////
    // NOTE(xkazu0x): game load
    EXINFO("EXCALIBUR :: GAME");
    win32_game_code game = win32_load_game_code(source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
    if (!game.loaded) {
        return(1);
    }
    s32 game_load_count = 0;
    
    EXINFO("EXCALIBUR : RUN");
    b32 quit = EX_FALSE;
    while (!quit) {
        // NOTE(xkazu0x): game reload
        // NOTE(xkazu0x): the game load count prevents a reloading error
        if (game_load_count++ > 60) {
            FILETIME game_new_write_time = win32_get_last_write_time(source_game_code_dll_fullpath);
            if (CompareFileTime(&game_new_write_time, &game.last_write_time) != 0) {
                win32_unload_game_code(&game);
                game = win32_load_game_code(source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
            }
            game_load_count = 0;
        }

        // NOTE(xkazu0x): window message loop
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            switch (message.message) {
                case WM_QUIT: {
                    quit = EX_TRUE;
                } break;
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP: {
                    u32 key_code = (u32)message.wParam;
                    b32 down = ((message.lParam & (1 << 31)) == 0);
                    win32_process_digital_button(&g_input.keyboard[key_code], down);
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
                            g_input.mouse.delta_position.x += raw_input->data.mouse.lLastX;
                            g_input.mouse.delta_position.y += raw_input->data.mouse.lLastY;

                            USHORT button_flags = raw_input->data.mouse.usButtonFlags;
                            b32 left_button_down = g_input.mouse.left.down;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_DOWN) left_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_UP) left_button_down = EX_FALSE;
                            win32_process_digital_button(&g_input.mouse.left, left_button_down);

                            b32 right_button_down = g_input.mouse.right.down;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_down = EX_FALSE;
                            win32_process_digital_button(&g_input.mouse.right, right_button_down);
                            
                            b32 middle_button_down = g_input.mouse.middle.down;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) middle_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_UP) middle_button_down = EX_FALSE;
                            win32_process_digital_button(&g_input.mouse.middle, middle_button_down);

                            b32 x1_button_down = g_input.mouse.x1.down;
                            if (button_flags & RI_MOUSE_BUTTON_4_DOWN) x1_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_BUTTON_4_UP) x1_button_down = EX_FALSE;
                            win32_process_digital_button(&g_input.mouse.x1, x1_button_down);
                            
                            b32 x2_button_down = g_input.mouse.x2.down;
                            if (button_flags & RI_MOUSE_BUTTON_5_DOWN) x2_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_BUTTON_5_UP) x2_button_down = EX_FALSE;
                            win32_process_digital_button(&g_input.mouse.x2, x2_button_down);
                            
                            if (raw_input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                                g_input.mouse.delta_wheel += ((SHORT)raw_input->data.mouse.usButtonData) / WHEEL_DELTA;
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
        
        if (g_input.keyboard[VK_ESCAPE].pressed) {
            quit = EX_TRUE;
        }
        if (g_input.keyboard[VK_F11].pressed) {
            win32_window_toggle_fullscreen(window_handle, &g_win32.window_placement);
        }

        // NOTE(xkazu0x): mouse update
        // g_input.mouse.position.x += g_input.mouse.delta_position.x;
        // g_input.mouse.position.y += g_input.mouse.delta_position.y;
        g_input.mouse.wheel += g_input.mouse.delta_wheel;

        POINT mouse_position;
        GetCursorPos(&mouse_position);
        ScreenToClient(window_handle, &mouse_position);

        if (mouse_position.x < 0) mouse_position.x = 0;
        if (mouse_position.y < 0) mouse_position.y = 0;
        if (mouse_position.x > window_size.x) mouse_position.x = window_size.x;
        if (mouse_position.y > window_size.y) mouse_position.y = window_size.y;
        
        g_input.mouse.position.x = mouse_position.x;
        g_input.mouse.position.y = mouse_position.y;
        
        // NOTE(xkazu0x): gamepad update
        u32 max_gamepad_count = XUSER_MAX_COUNT;
        if (max_gamepad_count > EX_ARRAY_COUNT(g_input.gamepads)) {
            max_gamepad_count = EX_ARRAY_COUNT(g_input.gamepads);
        }
        for (u32 i = 0; i < max_gamepad_count; i++) {
            XINPUT_STATE xinput_state = {};
            DWORD xinput_result = xinput_get_state(i, &xinput_state);
            if (xinput_result == ERROR_SUCCESS) {
                win32_process_digital_button(&g_input.gamepads[i].up, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
                win32_process_digital_button(&g_input.gamepads[i].down, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
                win32_process_digital_button(&g_input.gamepads[i].left, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
                win32_process_digital_button(&g_input.gamepads[i].right, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
                win32_process_digital_button(&g_input.gamepads[i].start, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
                win32_process_digital_button(&g_input.gamepads[i].back, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
                win32_process_digital_button(&g_input.gamepads[i].left_thumb,(xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
                win32_process_digital_button(&g_input.gamepads[i].right_thumb, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
                win32_process_digital_button(&g_input.gamepads[i].left_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
                win32_process_digital_button(&g_input.gamepads[i].right_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
                win32_process_digital_button(&g_input.gamepads[i].a, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
                win32_process_digital_button(&g_input.gamepads[i].b, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
                win32_process_digital_button(&g_input.gamepads[i].x, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
                win32_process_digital_button(&g_input.gamepads[i].y, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
                win32_process_analog_button(&g_input.gamepads[i].left_trigger, xinput_state.Gamepad.bLeftTrigger / 255.0f);
                win32_process_analog_button(&g_input.gamepads[i].right_trigger, xinput_state.Gamepad.bRightTrigger / 255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
                win32_process_stick(&g_input.gamepads[i].left_stick, CONVERT(xinput_state.Gamepad.sThumbLX), CONVERT(xinput_state.Gamepad.sThumbLY));
                win32_process_stick(&g_input.gamepads[i].right_stick, CONVERT(xinput_state.Gamepad.sThumbRX), CONVERT(xinput_state.Gamepad.sThumbRY));
#undef CONVERT
            } else {
                break;
            }
        }

        // NOTE(xkazu0x): update and render
        thread_context thread = {};
        if (game.update_and_render) {
            game.update_and_render(&thread, &g_memory, &g_input, &g_bitmap);
        }
        
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
            EXWARN("< Frame rate missed");
        }
        
        // NOTE(xkazu0x): display bitmap
        window_size = win32_get_window_size(window_handle);
        HDC window_device = GetDC(window_handle);
        win32_display_bitmap(g_bitmap, window_size, window_device);
        ReleaseDC(window_handle, window_device);

        // NOTE(xkazu0x): compute time
        LARGE_INTEGER end_counter = win32_get_time();
        f32 ms_per_frame = 1000.0f * win32_get_delta_seconds(last_counter, end_counter);
        
        s64 counts_per_frame = end_counter.QuadPart - last_counter.QuadPart;
        f32 frames_per_second = (f32)g_win32.time_frequency / (f32)counts_per_frame;

        f32 mega_cycles_per_frame = ((f32)cycles_per_frame / (1000.0f * 1000.0f));
        
        last_counter = end_counter;
        last_cycle_count = end_cycle_count;
                
#if 1
        // NOTE(xkazu0x): log time
        // TODO(xkazu0x): this is debug code only
        local f64 time_seconds = 0.0;
        local f64 last_print_time = 0.0;
        time_seconds += work_seconds_per_frame;
        if (time_seconds - last_print_time > 0.1f) {
            //EXDEBUG("%.02fms, %.02ffps, %.02fmc", ms_per_frame, frames_per_second, mega_cycles_per_frame);
            char new_window_title[512];
            sprintf(new_window_title, "%s - %.02ff/s, %.02fms/f, %.02fmc/f", window_title, frames_per_second, ms_per_frame, mega_cycles_per_frame);
            SetWindowTextA(window_handle, new_window_title);
            last_print_time = time_seconds;
        }
#endif
        
        // NOTE(xkazu0x): input reset
        for (u32 i = 0; i < KK_MAX; i++) {
            g_input.keyboard[i].pressed = EX_FALSE;
            g_input.keyboard[i].released = EX_FALSE;
        }

        g_input.mouse.left.pressed = EX_FALSE;
        g_input.mouse.left.released = EX_FALSE;
        g_input.mouse.right.pressed = EX_FALSE;
        g_input.mouse.right.released = EX_FALSE;
        g_input.mouse.middle.pressed = EX_FALSE;
        g_input.mouse.middle.released = EX_FALSE;
        g_input.mouse.delta_position.x = 0;
        g_input.mouse.delta_position.y = 0;
        g_input.mouse.delta_wheel = 0;
    }
    EXINFO("EXCALIBUR : SHUTDOWN");
    UnregisterClassA(MAKEINTATOM(window_atom), instance);
    DestroyWindow(window_handle);
    return(0);
}
