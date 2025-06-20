#include "base/excalibur_base.h"
#include "os/excalibur_os.h"
#include "os/excalibur_os_helper.h"
#include "os/excalibur_win32.h"

#include "base/excalibur_base.cpp"
#include "os/excalibur_os_helper.cpp"

#include <malloc.h>

/*
  TODO(xkazu0x):
  > key codes for keyboard input
  > sound system
  > Hardware acceleration (OpenGL)
  > ChangeDisplaySettings option
*/
global b32 quit;

global Win32_State win32_state;

global XInput_Get_State *xinput_get_state;
global XInput_Set_State *xinput_set_state;

XINPUT_GET_STATE(_xinput_get_state) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}

XINPUT_SET_STATE(_xinput_set_state) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}

////////////////////////////////
// TODO(xkazu0x): temp functions
internal inline u32
safe_truncate_u64(u64 value) {
    assert(value <= u32_max);
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
    Debug_OS_File result = {};
    
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
    b32 result = false;
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

internal Win32_Game
win32_load_game(char *source_dll_name, char *temp_dll_name) {
    CopyFile(source_dll_name, temp_dll_name, FALSE);
    
    Win32_Game result = {};
    result.last_write_time = win32_get_last_write_time(source_dll_name);
    result.library = LoadLibraryA(temp_dll_name);
    if (result.library) {
        WIN32_GET_PROC_ADDR(result.update_and_render, result.library, "game_update_and_render");
        if (result.update_and_render) {
            result.loaded = true;
            log_debug("game code loaded");
        }
    } else {
        log_error("failed to load game dll");
        //win32_load_game(source_dll_name, temp_dll_name);
    }
    
    if (!result.loaded) {
        result.update_and_render = 0;
        log_error("failed to load game code");
    }
    
    return(result);
}

internal void
win32_unload_game(Win32_Game *game) {
    if (game->library) {
        FreeLibrary(game->library);
        game->library = 0;
        log_debug("game code unloaded");
    }
    game->loaded = false;
    game->update_and_render = 0;
}

internal void
win32_build_exe_path_filename(Win32_State *state, char *filename,
                              u32 dest_count, char *dest) {
    cat_strings(state->one_past_last_exe_filename_slash - state->exe_filename, state->exe_filename,
                string_length(filename), filename,
                dest_count, dest);
}

internal void
win32_get_exe_filename(Win32_State *state) {
    //DWORD size_of_filename;
    GetModuleFileName(0, state->exe_filename, sizeof(state->exe_filename));
    state->one_past_last_exe_filename_slash = state->exe_filename;
    for (char *scan = state->exe_filename;
         *scan;
         ++scan) {
        if (*scan == '\\') {
            state->one_past_last_exe_filename_slash = scan + 1;
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
    f32 result = ((f32)(end.QuadPart - start.QuadPart)) / ((f32)(win32_state.time_frequency));
    return(result);
}

internal void
handle_debug_cycle_counters(OS_Memory *memory) {
#if EXCALIBUR_INTERNAL
    log_info("DEBUG CYCLE COUNTS:");
    for (u32 counter_index = 0;
         counter_index < array_count(memory->counters);
         ++counter_index) {
        Debug_Cycle_Counter *counter = memory->counters + counter_index;
        if (counter->hit_count > 0) {
            log_info("index %d: %I64ucy, %uh, %I64ucy/h",
                     counter_index,
                     counter->cycle_count,
                     counter->hit_count,
                     counter->cycle_count / counter->hit_count);
            counter->cycle_count = 0;
            counter->hit_count = 0;
        }
    }
#endif
}

internal Win32_Window_Size
win32_get_window_size(HWND window) {
    RECT client_rectangle;
    GetClientRect(window, &client_rectangle);
    
    Win32_Window_Size result = {};
    result.x = client_rectangle.right - client_rectangle.left;
    result.y = client_rectangle.bottom - client_rectangle.top;
    
    return(result);
}

#define align16(x) ((x + 15) & ~15)

internal void
win32_resize_framebuffer(OS_Framebuffer *framebuffer, s32 width, s32 height) {
    if (framebuffer->memory) {
        VirtualFree(framebuffer->memory, 0, MEM_RELEASE);
    }

    framebuffer->width = width;
    framebuffer->height = height;
    framebuffer->pitch = align16(framebuffer->width*BYTES_PER_PIXEL);
    
    win32_state.bitmap_info.bmiHeader.biSize = sizeof(win32_state.bitmap_info.bmiHeader);
    win32_state.bitmap_info.bmiHeader.biWidth = framebuffer->width;
    win32_state.bitmap_info.bmiHeader.biHeight = framebuffer->height;
    win32_state.bitmap_info.bmiHeader.biPlanes = 1;
    win32_state.bitmap_info.bmiHeader.biBitCount = 8*BYTES_PER_PIXEL;
    win32_state.bitmap_info.bmiHeader.biCompression = BI_RGB;

    s32 framebuffer_memory_size = (framebuffer->pitch*framebuffer->height);
    framebuffer->memory = VirtualAlloc(0, framebuffer_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void
win32_display_framebuffer(OS_Framebuffer framebuffer, HWND window_handle, s32 window_width, s32 window_height) {
    s32 display_width = framebuffer.width;
    s32 display_height = framebuffer.height;

    s32 offset = 16;
    s32 display_x = offset;
    s32 display_y = offset;
        
    HDC window_device = GetDC(window_handle);
    StretchDIBits(window_device,
                  display_x, display_y, display_width, display_height,
                  0, 0, framebuffer.width, framebuffer.height,
                  framebuffer.memory,
                  &win32_state.bitmap_info,
                  DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(window_handle, window_device);
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

struct OS_Work_Queue_Entry {
    OS_Work_Queue_Callback *callback;
    void *data;
};

struct OS_Work_Queue {
    HANDLE semaphore_handle;
    
    u32 volatile completion_goal;
    u32 volatile completion_count;
    u32 volatile next_entry_to_write;
    u32 volatile next_entry_to_read;
    
    OS_Work_Queue_Entry entries[256];
};

internal void
win32_work_queue_add_entry(OS_Work_Queue *queue, OS_Work_Queue_Callback *callback, void *data) {
    // TODO(xkazu0x): Switch to InterlockedCompareExchange eventually
    // so that any thread can add?
    u32 new_next_entry_to_write = (queue->next_entry_to_write + 1) % array_count(queue->entries);
    assert(new_next_entry_to_write != queue->next_entry_to_read);

    OS_Work_Queue_Entry *entry = queue->entries + queue->next_entry_to_write;
    entry->callback = callback;
    entry->data = data;
    ++queue->completion_goal;
    
    _WriteBarrier();
    
    queue->next_entry_to_write = new_next_entry_to_write;
    
    ReleaseSemaphore(queue->semaphore_handle, 1, 0);
}

internal b32
win32_do_next_work_queue_entry(OS_Work_Queue *queue) {
    b32 should_sleep = false;

    u32 original_next_entry_to_read = queue->next_entry_to_read;
    u32 new_next_entry_to_read = (original_next_entry_to_read + 1) % array_count(queue->entries);
    
    if (original_next_entry_to_read != queue->next_entry_to_write) {
        u32 entry_index = InterlockedCompareExchange((LONG volatile *)&queue->next_entry_to_read,
                                                     new_next_entry_to_read,
                                                     original_next_entry_to_read);
        if (entry_index == original_next_entry_to_read) {
            OS_Work_Queue_Entry *entry = queue->entries + entry_index;
            entry->callback(queue, entry->data);
            
            InterlockedIncrement((LONG volatile *)&queue->completion_count);
        }
    } else {
        should_sleep = true;
    }

    return(should_sleep);
}

internal void
win32_work_queue_complete_all_work(OS_Work_Queue *queue) {
    while (queue->completion_goal != queue->completion_count) {
        win32_do_next_work_queue_entry(queue);
    }

    queue->completion_goal = 0;
    queue->completion_count = 0;
}

DWORD WINAPI
thread_proc(LPVOID data) {
    OS_Work_Queue *queue = (OS_Work_Queue *)data;
    
    for (;;) {
        if (win32_do_next_work_queue_entry(queue)) {
            WaitForSingleObjectEx(queue->semaphore_handle, INFINITE, FALSE);
        }
    }
    
    //return(0);
}

internal
OS_WORK_QUEUE_CALLBACK(do_worker_work) {
    log_info("Thread %u: %s", GetCurrentThreadId(), (char *)data);
}

internal OS_Work_Queue
win32_make_queue(u32 thread_count) {
    OS_Work_Queue result;
    result.completion_goal = 0;
    result.completion_count = 0;
    result.next_entry_to_write = 0;
    result.next_entry_to_read = 0;
    
    u32 initial_count = 0;
    result.semaphore_handle = CreateSemaphoreEx(0, initial_count, thread_count, 0, 0, SEMAPHORE_ALL_ACCESS);
    
    for (u32 thread_index = 0;
         thread_index < thread_count;
         ++thread_index) {
        DWORD thread_id;
        HANDLE thread_handle = CreateThread(0, 0, thread_proc, &result, 0, &thread_id);
        CloseHandle(thread_handle);
    }

    return(result);
}

// NOTE(xkazu0x): When building with WinMain, It is not possible to print in the terminal. Build with the default main function if you want to print in the terminal :).
#if 0
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main(void)
#endif
{
    log_info("operating system: %s", string_from_operating_system(operating_system_from_context()));
    log_info("architecture: %s", string_from_architecture(architecture_from_context()));
    log_info("compiler: %s", string_from_compiler(compiler_from_context()));
    
    OS_Work_Queue high_priority_queue = win32_make_queue(4);
    OS_Work_Queue low_priority_queue = win32_make_queue(2);

#if 0
    win32_work_queue_add_entry(&queue, do_worker_work, "String A0");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A1");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A2");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A3");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A4");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A5");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A6");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A7");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A8");
    win32_work_queue_add_entry(&queue, do_worker_work, "String A9");
    
    win32_work_queue_add_entry(&queue, do_worker_work, "String B0");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B1");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B2");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B3");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B4");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B5");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B6");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B7");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B8");
    win32_work_queue_add_entry(&queue, do_worker_work, "String B9");
    
    win32_work_queue_complete_all_work(&queue);
#endif
    
    ///////////////////////////
    // NOTE(xkazu0x): game load
    win32_get_exe_filename(&win32_state);
    
    char source_game_code_dll_fullpath[WIN32_FILENAME_MAX];
    win32_build_exe_path_filename(&win32_state, "excalibur_game.dll",
                                  sizeof(source_game_code_dll_fullpath),
                                  source_game_code_dll_fullpath);
    
    char temp_game_code_dll_fullpath[WIN32_FILENAME_MAX];
    win32_build_exe_path_filename(&win32_state, "excalibur_game_temp.dll",
                                  sizeof(temp_game_code_dll_fullpath),
                                  temp_game_code_dll_fullpath);

    Win32_Game game = win32_load_game(source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
    if (!game.loaded) return(1);
    
    //////////////////////////////
    // NOTE(xkazu0x): monitor info
    DEVMODE monitor_info;
    monitor_info.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &monitor_info);
    
    s32 monitor_width = monitor_info.dmPelsWidth;
    s32 monitor_height = monitor_info.dmPelsHeight;
    s32 monitor_frame_rate = monitor_info.dmDisplayFrequency;
    log_info("monitor size: %dx%d", monitor_width, monitor_height);
    log_info("monitor refresh rate: %dHz", monitor_frame_rate);
    
    //////////////////////////////////
    // NOTE(xkazu0x): framebuffer init
    OS_Framebuffer framebuffer = {};
    //win32_resize_framebuffer(&framebuffer, 320, 180);
    win32_resize_framebuffer(&framebuffer, 960, 540);
    //win32_resize_framebuffer(&framebuffer, 1920, 1080);
    //win32_resize_framebuffer(&framebuffer, 1279, 719);
    log_info("framebuffer size: %dx%d", framebuffer.width, framebuffer.height);
    
    /////////////////////////////
    // NOTE(xkazu0x): window init
    s32 window_width = 1280;
    s32 window_height = 720;
    s32 window_x = (monitor_width - window_width)/2;
    s32 window_y = (monitor_height - window_height)/2;
    log_info("window size: %dx%d", window_width, window_height);
    
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

    HINSTANCE window_instance = GetModuleHandleA(0);
    
    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = window_instance;
    window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = "excalibur_window_class";
    ATOM window_atom = RegisterClassA(&window_class);
    if (!window_atom) {
        log_fatal("failed to register win32 window");
        return(1);
    }
    log_debug("win32 window registered");
    
    HWND window_handle = CreateWindowExA(window_style_ex, MAKEINTATOM(window_atom),
                                         window_title, window_style,
                                         window_x, window_y,
                                         fixed_window_width, fixed_window_height,
                                         0, 0, window_instance, 0);
    if (!window_handle) {
        log_fatal("failed to create win32 window");
        return(1);
    }
    log_debug("win32 window created");
    ShowWindow(window_handle, SW_SHOW);

    ////////////////////////////
    // NOTE(xkazu0x): input init
    OS_Input input = {};
    
    RAWINPUTDEVICE raw_input_device = {};
    raw_input_device.usUsagePage = 0x01;
    raw_input_device.usUsage = 0x02;
    raw_input_device.hwndTarget = window_handle;
    if (!RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device))) {
        log_error("failed to register mouse as raw input device");
        return(1);
    }
    log_debug("mouse registered as raw input device");
    
    HMODULE xinput_library = LoadLibraryA("xinput1_4.dll");
    if (!xinput_library) {
        log_error("failed to load 'xinput1_4.dll'");
        xinput_library = LoadLibraryA("xinput1_3.dll");
        if (!xinput_library) {
            log_error("failed to load 'xinput1_3.dll'");
            xinput_library = LoadLibraryA("xinput9_1_0.dll");
            if (!xinput_library) {
                log_error("failed to load 'xinput9_1_0.dll'");
            } else {
                log_debug("successfully loaded 'xinput9_1_0.dll'");
            }
        } else {
            log_debug("successfully loaded 'xinput1_3.dll'");
        }
    } else {
        log_debug("successfully loaded 'xinput1_4.dll'");
    }
    
    if (xinput_library) {
        WIN32_GET_PROC_ADDR(xinput_get_state, xinput_library, "XInputGetState");
        WIN32_GET_PROC_ADDR(xinput_set_state, xinput_library, "XInputSetState");
    } else {
        xinput_get_state = _xinput_get_state;
        xinput_set_state = _xinput_set_state;
    }

    /////////////////////////////
    // NOTE(xkazu0x): memory init
    OS_Memory memory = {};
    
#if EXCALIBUR_INTERNAL
    LPVOID base_address = (LPVOID)TB(2);
#else
    LPVOID base_address = 0;
#endif
    memory.permanent_storage_size = MB(64);
    memory.transient_storage_size = GB(1);
    
    memory.high_priority_queue = &high_priority_queue;
    memory.low_priority_queue = &low_priority_queue;
    
    memory.os_work_queue_add_entry = win32_work_queue_add_entry;
    memory.os_work_queue_complete_all_work = win32_work_queue_complete_all_work;

    memory.debug_os_free_file = debug_os_free_file;
    memory.debug_os_read_file = debug_os_read_file;
    memory.debug_os_write_file = debug_os_write_file;
    
    win32_state.game_memory_size = memory.permanent_storage_size + memory.transient_storage_size;
    win32_state.game_memory_block = VirtualAlloc(base_address, win32_state.game_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    memory.permanent_storage = win32_state.game_memory_block;
    memory.transient_storage = ((u8 *)memory.permanent_storage + memory.permanent_storage_size);
    if (!memory.permanent_storage) {
        log_fatal("failed to allocate game memory");
        return(1);
    }
    log_debug("permanent storage size: %llu", memory.permanent_storage_size);
    log_debug("transient storage size: %llu", memory.transient_storage_size);
    
    ////////////////////////////
    // NOTE(xkazu0x): init clock
    OS_Clock clock = {};
    
    //f32 game_update_hz = 30.0f;
    //f32 game_update_hz = 60.0f;
    f32 game_update_hz = (f32)monitor_frame_rate;
    f32 target_seconds_per_frame = 1.0f / game_update_hz;

    LARGE_INTEGER large_integer;
    QueryPerformanceFrequency(&large_integer);
    win32_state.time_frequency = large_integer.QuadPart;

    // NOTE(xkazu0x): set the windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular
    UINT desired_scheduler_ms = 1;
    b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
    
    LARGE_INTEGER last_counter = win32_get_time();
    u64 last_cycle_count = __rdtsc();

    ///////////////////////////////
    // NOTE(xkazu0x): engine state
    b32 pause = false;
    
    while (!quit) {
        // NOTE(xkazu0x): game reload
        input.executable_reloaded = false;
        FILETIME game_new_write_time = win32_get_last_write_time(source_game_code_dll_fullpath);
        if (CompareFileTime(&game_new_write_time, &game.last_write_time) != 0) {
            win32_unload_game(&game);
            game = win32_load_game(source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
            input.executable_reloaded = true;
        }
                
        // NOTE(xkazu0x): input reset
        for (u32 i = 0; i < Key_Count; ++i) {
            input.keyboard[i].pressed = false;
            input.keyboard[i].released = false;
        }

        input.mouse.left.pressed = false;
        input.mouse.left.released = false;
        input.mouse.right.pressed = false;
        input.mouse.right.released = false;
        input.mouse.middle.pressed = false;
        input.mouse.middle.released = false;
        input.mouse.x1.pressed = false;
        input.mouse.x1.released = false;
        input.mouse.x2.pressed = false;
        input.mouse.x2.released = false;
        input.mouse.delta_wheel = 0;
        input.mouse.dx = 0;
        input.mouse.dy = 0;

        // NOTE(xkazu0x): window message loop
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            switch (message.message) {
                case WM_QUIT: {
                    quit = true;
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
                    char *buffer = (char *)_malloca(size);
                    if (GetRawInputData((HRAWINPUT)message.lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size) {
                        RAWINPUT *raw_input = (RAWINPUT *)buffer;
                        if (raw_input->header.dwType == RIM_TYPEMOUSE && raw_input->data.mouse.usFlags == MOUSE_MOVE_RELATIVE){
                            input.mouse.dx += raw_input->data.mouse.lLastX;
                            input.mouse.dy += raw_input->data.mouse.lLastY;

                            USHORT button_flags = raw_input->data.mouse.usButtonFlags;
                            b32 left_button_down = input.mouse.left.down;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_DOWN) left_button_down = true;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_UP) left_button_down = false;
                            os_process_digital_button(&input.mouse.left, left_button_down);

                            b32 right_button_down = input.mouse.right.down;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_down = true;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_down = false;
                            os_process_digital_button(&input.mouse.right, right_button_down);
                            
                            b32 middle_button_down = input.mouse.middle.down;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) middle_button_down = true;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_UP) middle_button_down = false;
                            os_process_digital_button(&input.mouse.middle, middle_button_down);

                            b32 x1_button_down = input.mouse.x1.down;
                            if (button_flags & RI_MOUSE_BUTTON_4_DOWN) x1_button_down = true;
                            if (button_flags & RI_MOUSE_BUTTON_4_UP) x1_button_down = false;
                            os_process_digital_button(&input.mouse.x1, x1_button_down);
                            
                            b32 x2_button_down = input.mouse.x2.down;
                            if (button_flags & RI_MOUSE_BUTTON_5_DOWN) x2_button_down = true;
                            if (button_flags & RI_MOUSE_BUTTON_5_UP) x2_button_down = false;
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
        
        input.mouse.x = mouse_position.x;
        input.mouse.y = mouse_position.y;
        
        // NOTE(xkazu0x): gamepad update
        u32 max_gamepad_count = XUSER_MAX_COUNT;
        if (max_gamepad_count > array_count(input.gamepads)) {
            max_gamepad_count = array_count(input.gamepads);
        }
        
        for (u32 gamepad_index = 0;
             gamepad_index < max_gamepad_count;
             gamepad_index++) {
            XINPUT_STATE xinput_state = {};
            DWORD xinput_result = xinput_get_state(gamepad_index, &xinput_state);
            if (xinput_result == ERROR_SUCCESS) {
                os_process_digital_button(&input.gamepads[gamepad_index].up, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].down, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].left, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].right, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].start, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].back, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].left_thumb,(xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].right_thumb, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].left_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].right_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].a, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].b, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].x, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
                os_process_digital_button(&input.gamepads[gamepad_index].y, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
                os_process_analog_button(&input.gamepads[gamepad_index].left_trigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD/255.0f, xinput_state.Gamepad.bLeftTrigger/255.0f);
                os_process_analog_button(&input.gamepads[gamepad_index].right_trigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD/255.0f, xinput_state.Gamepad.bRightTrigger/255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
                os_process_stick(&input.gamepads[gamepad_index].left_stick, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/32767.0f, CONVERT(xinput_state.Gamepad.sThumbLX), CONVERT(xinput_state.Gamepad.sThumbLY));
                os_process_stick(&input.gamepads[gamepad_index].right_stick, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE/32767.0f, CONVERT(xinput_state.Gamepad.sThumbRX), CONVERT(xinput_state.Gamepad.sThumbRY));
#undef CONVERT
            } else {
                break;
            }
        }

        // NOTE(xkazu0x): key presses
        if (input.keyboard[Key_Escape].pressed) quit = true;
        if (input.keyboard[Key_F1].pressed) pause = !pause;
        if (input.keyboard[Key_F11].pressed) win32_window_toggle_fullscreen(window_handle, &win32_state.window_placement);

        // NOTE(xkazu0x): update and render
        if (!pause) {
            OS_Thread thread = {};
            clock.dt = target_seconds_per_frame;
            if (game.update_and_render) {
                game.update_and_render(&framebuffer, &input, &memory, &clock, &thread);
                handle_debug_cycle_counters(&memory);
            }
        }
        
        // NOTE(xkazu0x): limit frame rate
        u64 raw_end_cycle_count = __rdtsc();
        u64 raw_cycles_per_frame = raw_end_cycle_count - last_cycle_count;
        f32 raw_mega_cycles_per_frame = ((f32)raw_cycles_per_frame/(1000.0f*1000.0f));
        
        LARGE_INTEGER raw_end_counter = win32_get_time();
        f32 raw_seconds_per_frame = win32_get_delta_seconds(last_counter, raw_end_counter);
        f32 raw_ms_per_frame = 1000.0f*raw_seconds_per_frame;
        f32 raw_frames_per_second = (f32)win32_state.time_frequency/(f32)(raw_end_counter.QuadPart - last_counter.QuadPart);

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
            //     log_debug("TEST");
            // }
            
            while (seconds_elapsed_for_frame < target_seconds_per_frame) {
                seconds_elapsed_for_frame = win32_get_delta_seconds(last_counter, win32_get_time());
            }
        } else {
            // TODO(xkazu0x): MISSED FRAME RATE
        }
        
        // NOTE(xkazu0x): compute time
        u64 end_cycle_count = __rdtsc();
        u64 cycles_per_frame = end_cycle_count - last_cycle_count;
        f32 mega_cycles_per_frame = ((f32)cycles_per_frame/(1000.0f*1000.0f));
        
        LARGE_INTEGER end_counter = win32_get_time();
        f32 ms_per_frame = 1000.0f*win32_get_delta_seconds(last_counter, end_counter);
        f32 frames_per_second = (f32)win32_state.time_frequency/(f32)(end_counter.QuadPart - last_counter.QuadPart);

        last_counter = end_counter;
        last_cycle_count = end_cycle_count;

        // NOTE(xkazu0x): display framebuffer
        Win32_Window_Size window_size = win32_get_window_size(window_handle);
        window_width = window_size.x;
        window_height = window_size.y;
        win32_display_framebuffer(framebuffer, window_handle, window_width, window_height);
#if 1
        // NOTE(xkazu0x): log time
        // TODO(xkazu0x): this is debug code only
        local f64 time_ms = 0.0;
        local f64 last_print_time = 0.0;
        time_ms += ms_per_frame;
        if (time_ms - last_print_time > 500.0f) {
            char new_window_title[512];
            sprintf(new_window_title, "%s [fixed: %.02ffps, %.02fms, %.02fmc] [raw: %.02ffps, %.02fms, %.02fmc]", window_title, frames_per_second, ms_per_frame, mega_cycles_per_frame, raw_frames_per_second, raw_ms_per_frame, raw_mega_cycles_per_frame);
            SetWindowTextA(window_handle, new_window_title);
            last_print_time = time_ms;
        }
#endif        
    }
    
    return(0);
}
