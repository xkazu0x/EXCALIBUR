#include "excalibur_base.h"
#include "excalibur_intrinsics.h"
#include "excalibur_math.h"
#include "excalibur_log.h"

#include "excalibur_base.cpp"
#include "excalibur_intrinsics.cpp"
#include "excalibur_math.cpp"
#include "excalibur_log.cpp"

#include "excalibur_os.h"

////////////////////////////////////////////////
// NOTE(xkazu0x): exclusive includes

#include "excalibur_os_helper.h"
#include "excalibur_os_helper.cpp"

#include "excalibur_win32.h"

/*
  TODO(xkazu0x):
  > key codes for keyboard input
  > sound system
  > Hardware acceleration (OpenGL)
  > ChangeDisplaySettings option
*/

global win32_state_t global_win32;

global xinput_get_state_t *xinput_get_state;
global xinput_set_state_t *xinput_set_state;

XINPUT_GET_STATE(xinput_get_state_) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}

XINPUT_SET_STATE(xinput_set_state_) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}

////////////////////////////////
// TODO(xkazu0x): temp functions
internal inline u32
safe_truncate_u64(u64 value) {
    EX_ASSERT(value <= u32_max);
    u32 result = (u32)value;
    return(result);
}

internal u32
string_length(char *string) {
    u32 count = 0;
    while (*string++) {
        ++count;
    }
    return(count);
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
////////////////////////////////

DEBUG_OS_FREE_FILE(debug_os_free_file) {
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

DEBUG_OS_READ_FILE(debug_os_read_file) {
    debug_file_t result = {};
    
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
                    result.data = file_memory;
                } else {
                    // TODO(xkazu0x): logging
                    debug_os_free_file(thread, file_memory);
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

DEBUG_OS_WRITE_FILE(debug_os_write_file) {
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

internal win32_game_t
win32_load_game(char *source_dll_name, char *temp_dll_name) {
    win32_game_t result = {};
    result.last_write_time = win32_get_last_write_time(source_dll_name);

    CopyFile(source_dll_name, temp_dll_name, FALSE);
    result.library = LoadLibraryA(temp_dll_name);
    if (result.library) {
        WIN32_GET_PROC_ADDR(result.update_and_render, result.library, "game_update_and_render");
        if (result.update_and_render) {
            result.loaded = EX_TRUE;
            EXDEBUG("game code loaded");
        }
    } else {
        EXERROR("failed to load game dll");
    }
    
    if (!result.loaded) {
        result.update_and_render = 0;
        EXERROR("failed to load game code");
    }
    
    return(result);
}

internal void
win32_unload_game(win32_game_t *game) {
    if (game->library) {
        FreeLibrary(game->library);
        game->library = 0;
        EXDEBUG("game code unloaded");
    }
    game->loaded = EX_FALSE;
    game->update_and_render = 0;
}

internal void
win32_build_exe_path_filename(win32_state_t *win32, char *filename,
                              u32 dest_count, char *dest) {
    cat_strings(win32->one_past_last_exe_filename_slash - win32->exe_filename, win32->exe_filename,
                string_length(filename), filename,
                dest_count, dest);
}

internal void
win32_get_exe_filename(win32_state_t *win32) {
    DWORD size_of_filename = GetModuleFileName(0, win32->exe_filename, sizeof(win32->exe_filename));
    win32->one_past_last_exe_filename_slash = win32->exe_filename;
    for (char *scan = win32->exe_filename; *scan; ++scan) {
        if (*scan == '\\') {
            win32->one_past_last_exe_filename_slash = scan + 1;
        }
    }
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
win32_resize_framebuffer(os_framebuffer_t *framebuffer, s32 width, s32 height) {
    if (framebuffer->pixels) {
        VirtualFree(framebuffer->pixels, 0, MEM_RELEASE);
    }

    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->bytes_per_pixel = 4;
    framebuffer->pitch = framebuffer->width * framebuffer->bytes_per_pixel;
    
    global_win32.bitmap_info.bmiHeader.biSize = sizeof(global_win32.bitmap_info.bmiHeader);
    global_win32.bitmap_info.bmiHeader.biWidth = framebuffer->width;
    global_win32.bitmap_info.bmiHeader.biHeight = -framebuffer->height;
    global_win32.bitmap_info.bmiHeader.biPlanes = 1;
    global_win32.bitmap_info.bmiHeader.biBitCount = 8*framebuffer->bytes_per_pixel;
    global_win32.bitmap_info.bmiHeader.biCompression = BI_RGB;

    s32 framebuffer_memory_size = (framebuffer->width*framebuffer->height)*framebuffer->bytes_per_pixel;
    framebuffer->pixels = VirtualAlloc(0, framebuffer_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
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

internal void
win32_display_framebuffer(HWND window_handle, os_framebuffer_t framebuffer, vec2i window_size) {
    s32 display_width = ((f32)framebuffer.width/(f32)framebuffer.height)*window_size.y;
    s32 display_height = window_size.y;

    s32 display_x = (window_size.x - display_width)/2;
        
    HDC window_device = GetDC(window_handle);
    StretchDIBits(window_device,
                  display_x, 0, display_width, display_height,
                  0, 0, framebuffer.width, framebuffer.height,
                  framebuffer.pixels,
                  &global_win32.bitmap_info,
                  DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(window_handle, window_device);
}

internal LRESULT CALLBACK
win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch (message) {
        case WM_CLOSE:
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        case WM_SETCURSOR: {
#if EXCALIBUR_INTERNAL
            result = DefWindowProcA(window, message, wparam, lparam);
#else
            SetCursor(0);
#endif
        } break;
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        }
    }
    return(result);
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE previous_instance, LPSTR command_line, int show_command)  {
    EXINFO("operating system: %s", string_from_operating_system(operating_system_from_context()));
    EXINFO("architecture: %s", string_from_architecture(architecture_from_context()));

    ///////////////////////////
    // NOTE(xkazu0x): game load
    win32_get_exe_filename(&global_win32);
    
    char source_game_code_dll_fullpath[WIN32_FILENAME_MAX];
    win32_build_exe_path_filename(&global_win32, "excalibur.dll",
                                  sizeof(source_game_code_dll_fullpath),
                                  source_game_code_dll_fullpath);
    
    char temp_game_code_dll_fullpath[WIN32_FILENAME_MAX];
    win32_build_exe_path_filename(&global_win32, "excalibur_temp.dll",
                                  sizeof(temp_game_code_dll_fullpath),
                                  temp_game_code_dll_fullpath);

    win32_game_t game = win32_load_game(source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
    if (!game.loaded) return(1);
    
    //////////////////////////////
    // NOTE(xkazu0x): monitor info
    DEVMODE monitor_info;
    monitor_info.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &monitor_info);
    
    s32 monitor_width = monitor_info.dmPelsWidth;
    s32 monitor_height = monitor_info.dmPelsHeight;
    s32 monitor_frame_rate = monitor_info.dmDisplayFrequency;
    EXINFO("monitor size: %dx%d", monitor_width, monitor_height);
    EXINFO("monitor refresh rate: %dHz", monitor_frame_rate);
    
    //////////////////////////////////
    // NOTE(xkazu0x): framebuffer init
    s32 scale = 4;
    
    os_framebuffer_t framebuffer = {};
    win32_resize_framebuffer(&framebuffer, 320, 180);
    EXINFO("framebuffer size: %dx%d", framebuffer.width, framebuffer.height);
    
    /////////////////////////////
    // NOTE(xkazu0x): window init
    s32 window_width = framebuffer.width*scale;
    s32 window_height = framebuffer.height*scale;
    s32 window_x = (monitor_width - window_width)/2;
    s32 window_y = (monitor_height - window_height)/2;
    EXINFO("window size: %dx%d", window_width, window_height);
    
    char *window_title = "EXCALIBUR";
    s32 fixed_window_width = window_width;
    s32 fixed_window_height = window_height;
    u32 window_style = WS_OVERLAPPEDWINDOW;
    u32 window_style_ex = 0;
    
    RECT window_rectangle = {};
    window_rectangle.left = 0;
    window_rectangle.right = window_width;
    window_rectangle.top = 0;
    window_rectangle.bottom = window_height;
    if (AdjustWindowRect(&window_rectangle, window_style, 0)) {
        fixed_window_width = window_rectangle.right - window_rectangle.left;
        fixed_window_height = window_rectangle.bottom - window_rectangle.top;
    }

    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = instance;
    window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = "EXCALIBUR_WINDOW_CLASS";
    ATOM window_atom = RegisterClassA(&window_class);
    if (!window_atom) {
        EXFATAL("failed to register win32 window");
        return(1);
    }
    EXDEBUG("win32 window registered");
    
    HWND window_handle = CreateWindowExA(window_style_ex, MAKEINTATOM(window_atom),
                                         window_title, window_style,
                                         window_x, window_y,
                                         fixed_window_width, fixed_window_height,
                                         0, 0, instance, 0);
    if (!window_handle) {
        EXFATAL("failed to create win32 window");
        return(1);
    }
    EXDEBUG("win32 window created");
    ShowWindow(window_handle, SW_SHOW);

    ////////////////////////////
    // NOTE(xkazu0x): input init
    os_input_t input = {};
    
    RAWINPUTDEVICE raw_input_device = {};
    raw_input_device.usUsagePage = 0x01;
    raw_input_device.usUsage = 0x02;
    raw_input_device.hwndTarget = window_handle;
    if (!RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device))) {
        EXERROR("failed to register mouse as raw input device");
        return(1);
    }
    EXDEBUG("mouse registered as raw input device");
    
    HMODULE xinput_library = LoadLibraryA("xinput1_4.dll");
    if (!xinput_library) {
        EXERROR("failed to load 'xinput1_4.dll'");
        xinput_library = LoadLibraryA("xinput1_3.dll");
        if (!xinput_library) {
            EXERROR("failed to load 'xinput1_3.dll'");
            xinput_library = LoadLibraryA("xinput9_1_0.dll");
            if (!xinput_library) {
                EXERROR("failed to load 'xinput9_1_0.dll'");
            } else {
                EXDEBUG("successfully loaded 'xinput9_1_0.dll'");
            }
        } else {
            EXDEBUG("successfully loaded 'xinput1_3.dll'");
        }
    } else {
        EXDEBUG("successfully loaded 'xinput1_4.dll'");
    }
    
    if (xinput_library) {
        WIN32_GET_PROC_ADDR(xinput_get_state, xinput_library, "XInputGetState");
        WIN32_GET_PROC_ADDR(xinput_set_state, xinput_library, "XInputSetState");
    } else {
        xinput_get_state = xinput_get_state_;
        xinput_set_state = xinput_set_state_;
    }

    /////////////////////////////
    // NOTE(xkazu0x): memory init
    os_memory_t memory = {};
    
#if EXCALIBUR_INTERNAL
    LPVOID base_address = (LPVOID)EX_TERABYTES(2);
#else
    LPVOID base_address = 0;
#endif
    memory.permanent_storage_size = EX_MEGABYTES(64);
    memory.transient_storage_size = EX_GIGABYTES(1);
    memory.debug_os_free_file = debug_os_free_file;
    memory.debug_os_read_file = debug_os_read_file;
    memory.debug_os_write_file = debug_os_write_file;
    
    global_win32.game_memory_size = memory.permanent_storage_size + memory.transient_storage_size;
    global_win32.game_memory_block = VirtualAlloc(base_address, global_win32.game_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    memory.permanent_storage = global_win32.game_memory_block;
    memory.transient_storage = ((u8 *)memory.permanent_storage + memory.permanent_storage_size);
    if (!memory.permanent_storage) {
        EXFATAL("failed to allocate game memory");
        return(1);
    }
    EXDEBUG("memory allocated:");
    EXTRACE("permanent storage size: %llu", memory.permanent_storage_size);
    EXTRACE("transient storage size: %llu", memory.transient_storage_size);
    
    ////////////////////////////
    // NOTE(xkazu0x): clock init
    os_clock_t clock = {};
    
    f32 game_update_hz = (f32)monitor_frame_rate;
    //f32 game_update_hz = 60.0f;
    f32 target_seconds_per_frame = 1.0f / game_update_hz;

    LARGE_INTEGER large_integer;
    QueryPerformanceFrequency(&large_integer);
    global_win32.time_frequency = large_integer.QuadPart;

    // NOTE(xkazu0x): set the windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular
    UINT desired_scheduler_ms = 1;
    b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
    
    LARGE_INTEGER last_counter = win32_get_time();
    s64 last_cycle_count = __rdtsc();

    ///////////////////////////////
    // NOTE(xkazu0x): engine state
    b32 quit = EX_FALSE;
    b32 pause = EX_FALSE;
    
    while (!quit) {
        // NOTE(xkazu0x): game reload
        FILETIME game_new_write_time = win32_get_last_write_time(source_game_code_dll_fullpath);
        if (CompareFileTime(&game_new_write_time, &game.last_write_time) != 0) {
            win32_unload_game(&game);
            game = win32_load_game(source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
        }
                
        // NOTE(xkazu0x): input reset
        for (u32 i = 0; i < KEY_MAX; i++) {
            input.keyboard[i].pressed = EX_FALSE;
            input.keyboard[i].released = EX_FALSE;
        }

        input.mouse.left.pressed = EX_FALSE;
        input.mouse.left.released = EX_FALSE;
        input.mouse.right.pressed = EX_FALSE;
        input.mouse.right.released = EX_FALSE;
        input.mouse.middle.pressed = EX_FALSE;
        input.mouse.middle.released = EX_FALSE;
        input.mouse.x1.pressed = EX_FALSE;
        input.mouse.x1.released = EX_FALSE;
        input.mouse.x2.pressed = EX_FALSE;
        input.mouse.x2.released = EX_FALSE;
        input.mouse.delta_wheel = 0;
        input.mouse.delta_position.x = 0;
        input.mouse.delta_position.y = 0;

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
                    os_process_digital_button(&input.keyboard[key_code], down);
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
                            input.mouse.delta_position.x += raw_input->data.mouse.lLastX;
                            input.mouse.delta_position.y += raw_input->data.mouse.lLastY;

                            USHORT button_flags = raw_input->data.mouse.usButtonFlags;
                            b32 left_button_down = input.mouse.left.down;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_DOWN) left_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_UP) left_button_down = EX_FALSE;
                            os_process_digital_button(&input.mouse.left, left_button_down);

                            b32 right_button_down = input.mouse.right.down;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_down = EX_FALSE;
                            os_process_digital_button(&input.mouse.right, right_button_down);
                            
                            b32 middle_button_down = input.mouse.middle.down;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) middle_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_UP) middle_button_down = EX_FALSE;
                            os_process_digital_button(&input.mouse.middle, middle_button_down);

                            b32 x1_button_down = input.mouse.x1.down;
                            if (button_flags & RI_MOUSE_BUTTON_4_DOWN) x1_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_BUTTON_4_UP) x1_button_down = EX_FALSE;
                            os_process_digital_button(&input.mouse.x1, x1_button_down);
                            
                            b32 x2_button_down = input.mouse.x2.down;
                            if (button_flags & RI_MOUSE_BUTTON_5_DOWN) x2_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_BUTTON_5_UP) x2_button_down = EX_FALSE;
                            os_process_digital_button(&input.mouse.x2, x2_button_down);
                            
                            if (raw_input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                                input.mouse.delta_wheel += ((SHORT)raw_input->data.mouse.usButtonData) / WHEEL_DELTA;
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

        // NOTE(xkazu0x): mouse update
        input.mouse.wheel += input.mouse.delta_wheel;

        POINT mouse_position;
        GetCursorPos(&mouse_position);
        ScreenToClient(window_handle, &mouse_position);

        if (mouse_position.x < 0) mouse_position.x = 0;
        if (mouse_position.y < 0) mouse_position.y = 0;
        
        // TODO(xkazu0x): query new window size before this happens
        if (mouse_position.x > window_width) mouse_position.x = window_width;
        if (mouse_position.y > window_height) mouse_position.y = window_height;
        
        input.mouse.position.x = mouse_position.x;
        input.mouse.position.y = mouse_position.y;
        
        // NOTE(xkazu0x): gamepad update
        u32 max_gamepad_count = XUSER_MAX_COUNT;
        if (max_gamepad_count > EX_ARRAY_COUNT(input.gamepads)) {
            max_gamepad_count = EX_ARRAY_COUNT(input.gamepads);
        }
        for (u32 i = 0; i < max_gamepad_count; i++) {
            XINPUT_STATE xinput_state = {};
            DWORD xinput_result = xinput_get_state(i, &xinput_state);
            if (xinput_result == ERROR_SUCCESS) {
                os_process_digital_button(&input.gamepads[i].up, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
                os_process_digital_button(&input.gamepads[i].down, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
                os_process_digital_button(&input.gamepads[i].left, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
                os_process_digital_button(&input.gamepads[i].right, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
                os_process_digital_button(&input.gamepads[i].start, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
                os_process_digital_button(&input.gamepads[i].back, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
                os_process_digital_button(&input.gamepads[i].left_thumb,(xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
                os_process_digital_button(&input.gamepads[i].right_thumb, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
                os_process_digital_button(&input.gamepads[i].left_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
                os_process_digital_button(&input.gamepads[i].right_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
                os_process_digital_button(&input.gamepads[i].a, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
                os_process_digital_button(&input.gamepads[i].b, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
                os_process_digital_button(&input.gamepads[i].x, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
                os_process_digital_button(&input.gamepads[i].y, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
                os_process_analog_button(&input.gamepads[i].left_trigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD/255.0f, xinput_state.Gamepad.bLeftTrigger/255.0f);
                os_process_analog_button(&input.gamepads[i].right_trigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD/255.0f, xinput_state.Gamepad.bRightTrigger/255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
                os_process_stick(&input.gamepads[i].left_stick, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/32767.0f, CONVERT(xinput_state.Gamepad.sThumbLX), CONVERT(xinput_state.Gamepad.sThumbLY));
                os_process_stick(&input.gamepads[i].right_stick, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE/32767.0f, CONVERT(xinput_state.Gamepad.sThumbRX), CONVERT(xinput_state.Gamepad.sThumbRY));
#undef CONVERT
            } else {
                break;
            }
        }

        // NOTE(xkazu0x): key presses
        if (input.keyboard[KEY_ESCAPE].pressed) quit = EX_TRUE;
        if (input.keyboard[KEY_F1].pressed) pause = !pause;
        if (input.keyboard[KEY_F11].pressed) win32_window_toggle_fullscreen(window_handle, &global_win32.window_placement);

        // NOTE(xkazu0x): update and render
        if (!pause) {
            os_thread_t thread = {};
            clock.delta_seconds = target_seconds_per_frame;
            if (game.update_and_render) {
                game.update_and_render(&framebuffer, &input, &memory, &clock, &thread);
            }
        }
        
        // NOTE(xkazu0x): limit frame rate
        s64 raw_end_cycle_count = __rdtsc();
        s64 raw_cycles_per_frame = raw_end_cycle_count - last_cycle_count;
        f32 raw_mega_cycles_per_frame = ((f32)raw_cycles_per_frame/(1000.0f*1000.0f));
        
        LARGE_INTEGER raw_end_counter = win32_get_time();
        f32 raw_seconds_per_frame = win32_get_delta_seconds(last_counter, raw_end_counter);
        f32 raw_ms_per_frame = 1000.0f*raw_seconds_per_frame;
        f32 raw_frames_per_second = (f32)global_win32.time_frequency/(f32)(raw_end_counter.QuadPart - last_counter.QuadPart);

        // TODO(xkazu0x): PROBRABLY BUGGY
        f32 seconds_elapsed_for_frame = raw_seconds_per_frame;
        if (seconds_elapsed_for_frame < target_seconds_per_frame) {
            if (sleep_is_granular) {
                DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - seconds_elapsed_for_frame));
                if (sleep_ms > 0) {
                    Sleep(sleep_ms);
                }
            }
            
            // f32 test_seconds_elapsed_for_frame = win32_get_delta_seconds(last_counter, win32_get_time());
            // while (test_seconds_elapsed_for_frame < target_seconds_per_frame) {
            //     // TODO(xkazu0x): LOG MISSED SLEEP HERE
            //     EXDEBUG("TEST");
            // }
            
            while (seconds_elapsed_for_frame < target_seconds_per_frame) {
                seconds_elapsed_for_frame = win32_get_delta_seconds(last_counter, win32_get_time());
            }
        } else {
            // TODO(xkazu0x): MISSED FRAME RATE
        }
        
        // NOTE(xkazu0x): compute time
        s64 end_cycle_count = __rdtsc();
        s64 cycles_per_frame = end_cycle_count - last_cycle_count;
        f32 mega_cycles_per_frame = ((f32)cycles_per_frame/(1000.0f*1000.0f));
        
        LARGE_INTEGER end_counter = win32_get_time();
        f32 ms_per_frame = 1000.0f*win32_get_delta_seconds(last_counter, end_counter);
        f32 frames_per_second = (f32)global_win32.time_frequency/(f32)(end_counter.QuadPart - last_counter.QuadPart);

        last_counter = end_counter;
        last_cycle_count = end_cycle_count;

        // NOTE(xkazu0x): display framebuffer
        vec2i window_size = win32_get_window_size(window_handle);
        window_width = window_size.x;
        window_height = window_size.y;
        win32_display_framebuffer(window_handle, framebuffer, window_size);
#if 1
        // NOTE(xkazu0x): log time
        // TODO(xkazu0x): this is debug code only
        local f64 time_ms = 0.0;
        local f64 last_print_time = 0.0;
        time_ms += ms_per_frame;
        if (time_ms - last_print_time > 500.0f) {
            char new_window_title[512];
            sprintf(new_window_title, "%s - fixed: %.02ff/s, %.02fms/f, %.02fmc/f | raw: %.02ff/s, %.02fms/f, %.02fmc/f", window_title, frames_per_second, ms_per_frame, mega_cycles_per_frame, raw_frames_per_second, raw_ms_per_frame, raw_mega_cycles_per_frame);
            SetWindowTextA(window_handle, new_window_title);
            last_print_time = time_ms;
        }
#endif        
    }
    
    return(0);
}
