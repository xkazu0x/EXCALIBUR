#include "base/excalibur_base.h"
#include "os/excalibur_os.h"
#include "os/excalibur_os_helper.h"
#include "os/excalibur_os_win32.h"

#include "base/excalibur_base.cpp"
#include "os/excalibur_os_helper.cpp"

global b32 quit;
global b32 pause;

global Win32_State win32_state;

global XInput_Get_State *xinput_get_state;
global XInput_Set_State *xinput_set_state;

global Direct_Sound_Create *direct_sound_create;

XINPUT_GET_STATE(_xinput_get_state) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}

XINPUT_SET_STATE(_xinput_set_state) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}

internal void
win32_init_xinput(void) {
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
}

internal void
win32_init_dsound(HWND window, s32 samples_per_second, s32 buffer_size, LPDIRECTSOUNDBUFFER *buffer) {
    HMODULE dsound_library = LoadLibraryA("dsound.dll");
    if (dsound_library) {
        log_debug("successfully loaded 'dsound.dll'");
        WIN32_GET_PROC_ADDR(direct_sound_create, dsound_library, "DirectSoundCreate");

        LPDIRECTSOUND direct_sound;
        if (direct_sound_create && SUCCEEDED(direct_sound_create(0, &direct_sound, 0))) {
            WAVEFORMATEX wave_format = {};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.wBitsPerSample = 16;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.nBlockAlign = (wave_format.nChannels*wave_format.wBitsPerSample)/8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec*wave_format.nBlockAlign;
            wave_format.cbSize = 0;

            if (SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
                DSBUFFERDESC buffer_description = {};
                buffer_description.dwSize = sizeof(DSBUFFERDESC);
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;
                        
                LPDIRECTSOUNDBUFFER primary_buffer;
                if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0))) {
                    log_debug("direct sound: primary buffer created");
                    if (SUCCEEDED(primary_buffer->SetFormat(&wave_format))) {
                        log_debug("direct sound: primary buffer format was set");
                    } else {
                        log_error("direct sound: FAILED to set primary buffer format");
                    }
                } else {
                    log_error("direct sound: FAILED to create primary buffer");
                }
            } else {
                log_error("direct sound: FAILED \"SetCooperativeLevel\"");
            }
            
            DSBUFFERDESC buffer_description = {};
            buffer_description.dwSize = sizeof(DSBUFFERDESC);
            buffer_description.dwFlags = 0;
            buffer_description.dwBufferBytes = buffer_size;
            buffer_description.lpwfxFormat = &wave_format;

            if (SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_description, buffer, 0))) {
                log_debug("direct sound: secondary buffer created");
            } else {
                log_error("direct sound: FAILED to create secondary buffer");
            }
        } else {
            log_error("direct sound: FAILED to create object");
        }
    } else {
        log_error("failed to load 'dsound.dll'");
    }
}

internal void
win32_clear_sound_buffer(Win32_Sound_Output *sound_output) {
    VOID *region1;
    DWORD region1_size;
    VOID *region2;
    DWORD region2_size;
                        
    if (SUCCEEDED(sound_output->buffer->Lock(0, sound_output->buffer_size, &region1, &region1_size, &region2, &region2_size, 0))) {
        u8 *dest_sample = (u8 *)region1;
        
        for (DWORD byte_index = 0;
             byte_index < region1_size;
             ++byte_index) {
            *dest_sample++ = 0;
        }
        
        dest_sample = (u8 *)region2;
        
        for (DWORD byte_index = 0;
             byte_index < region2_size;
             ++byte_index) {
            *dest_sample++ = 0;
        }

        sound_output->buffer->Unlock(region1, region1_size, region2, region2_size);
    }    
}

internal void
win32_fill_sound_buffer(Win32_Sound_Output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write, OS_Sound_Buffer *sound_buffer) {
    VOID *region1;
    DWORD region1_size;
    VOID *region2;
    DWORD region2_size;
                        
    if (SUCCEEDED(sound_output->buffer->Lock(byte_to_lock, bytes_to_write, &region1, &region1_size, &region2, &region2_size, 0))) {
        s16 *src_sample = sound_buffer->samples;
        
        DWORD sample_count = region1_size/sound_output->bytes_per_sample;
        s16 *dest_sample = (s16 *)region1;
        
        for (DWORD sample_index = 0;
             sample_index < sample_count;
             ++sample_index) {
            *dest_sample++ = *src_sample++;
            *dest_sample++ = *src_sample++;
            ++sound_output->running_sample_index;
        }

        sample_count = region2_size/sound_output->bytes_per_sample;
        dest_sample = (s16 *)region2;
        
        for (DWORD sample_index = 0;
             sample_index < sample_count;
             ++sample_index) {
            *dest_sample++ = *src_sample++;
            *dest_sample++ = *src_sample++;
            ++sound_output->running_sample_index;
        }

        sound_output->buffer->Unlock(region1, region1_size, region2, region2_size);
    }
}

////////////////////////////////
// TODO(xkazu0x): move this to excalibur_arena.h
struct Arena {
    u64 size;
    u64 used;
    u8 *memory;
};

internal Arena
make_arena(u64 size, void *memory) {
    Arena result;
    result.size = size;
    result.used = 0;
    result.memory = (u8 *)memory;
    return(result);
}

#define push_array(arena, count, type) (type *)arena_push(arena, (count)*sizeof(type));
internal void *
arena_push(Arena *arena, u64 size) {
    assert((arena->used + size) <= arena->size);
    void *result = (void *)(arena->memory + arena->used);
    arena->used += size;
    return(result);
}
// {

////////////////////////////////
// TODO(xkazu0x): move this to excalibur_string.h
// {
internal u64
cstring8_length(u8 *cstr) {
    u8 *ptr = cstr;
    for (; *ptr != 0; ++ptr);
    return(ptr - cstr);
}

internal String8
string8_cstring(char *cstr) {
    String8 result;
    result.size = cstring8_length((u8 *)cstr);
    result.str = (u8 *)cstr;
    return(result);
}

internal String8
string8_cat(Arena *arena, String8 a, String8 b) {
    String8 result;
    result.size = a.size + b.size;
    result.str = push_array(arena, result.size + 1, u8);
    memmove(result.str, a.str, a.size);
    memmove(result.str + a.size, b.str, b.size);
    result.str[result.size] = 0;
    return(result);
}
// }

////////////////////////////////
// NOTE(xkazu0x): debug code only
// {
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
            u32 file_size32 = safe_cast_u32(file_size.QuadPart);
            void *file_memory = VirtualAlloc(0, file_size32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (file_memory) {
                DWORD bytes_read;
                if (ReadFile(file_handle, file_memory, file_size32, &bytes_read, 0) &&
                    (file_size32 == bytes_read)) {
                    result.size = file_size32;
                    result.data = file_memory;
                } else {
                    // TODO(xkazu0x): logging
                    debug_os_free_file(file_memory);
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
        if (WriteFile(file_handle, memory, size, &bytes_written, 0)) {
            result = (bytes_written == size);
        } else {
            // TODO(xkazu0x): logging
        }
        CloseHandle(file_handle);
    } else {
        // TODO(xkazu0x): logging
    }
    return(result);
}
// }

internal FILETIME
win32_get_last_write_time(char *filename) {
    FILETIME result = {};
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(filename, GetFileExInfoStandard, &data)) {
        result = data.ftLastWriteTime;
    }
    return(result);
}

internal void
win32_load_game_code(Win32_Game_Code *game, char *source_dll_name, char *temp_dll_name) {
    CopyFile(source_dll_name, temp_dll_name, FALSE);
    
    game->last_write_time = win32_get_last_write_time(source_dll_name);
    game->library = LoadLibraryA(temp_dll_name);
    
    if (game->library) {
        WIN32_GET_PROC_ADDR(game->update_and_render, game->library, "game_update_and_render");
        WIN32_GET_PROC_ADDR(game->get_sound_samples, game->library, "game_get_sound_samples");
    } else {
        log_error("failed to load game code");
    }
}

internal void
win32_unload_game_code(Win32_Game_Code *game) {
    if (game->library) {
        FreeLibrary(game->library);
        game->library = 0;
    }
    game->update_and_render = 0;
}

internal void
win32_get_exe_filename(Arena *arena, Win32_State *state) {
    state->exe_fullpath = push_array(arena, WIN32_FILENAME_MAX, char);
    GetModuleFileName(0, state->exe_fullpath, WIN32_FILENAME_MAX);

    state->exe_filename = state->exe_fullpath;
    for (char *scan = state->exe_fullpath;
         *scan;
         ++scan) {
        if (*scan == '\\') {
            state->exe_filename = scan + 1;
        }
    }
}

internal char *
win32_build_exe_path_filename(Arena *arena, Win32_State *state, char *filename) {
    String8 str0 = make_string8((u64)(win32_state.exe_filename - win32_state.exe_fullpath), (u8 *)win32_state.exe_fullpath);
    String8 str1 = string8_cstring(filename);
    char *result = (char *)string8_cat(arena, str0, str1).str;
    return(result);
}

internal LARGE_INTEGER
win32_get_wall_clock(void) {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

internal f32
win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    f32 result = ((f32)(end.QuadPart - start.QuadPart)) / ((f32)(win32_state.time_frequency));
    return(result);
}

internal Win32_Window_Size
win32_get_window_size(HWND window) {
    Win32_Window_Size result;
    
    RECT client_rectangle;
    GetClientRect(window, &client_rectangle);
    result.x = client_rectangle.right - client_rectangle.left;
    result.y = client_rectangle.bottom - client_rectangle.top;

    return(result);
}

#define align16(x) ((x + 15) & ~15)
internal void
win32_resize_back_buffer(OS_Back_Buffer *back_buffer, s32 width, s32 height) {
    if (back_buffer->memory) {
        VirtualFree(back_buffer->memory, 0, MEM_RELEASE);
    }

    back_buffer->width = width;
    back_buffer->height = height;
    back_buffer->pitch = align16(back_buffer->width*BYTES_PER_PIXEL);
    
    win32_state.bitmap_info.bmiHeader.biSize = sizeof(win32_state.bitmap_info.bmiHeader);
    win32_state.bitmap_info.bmiHeader.biWidth = back_buffer->width;
    win32_state.bitmap_info.bmiHeader.biHeight = back_buffer->height;
    win32_state.bitmap_info.bmiHeader.biPlanes = 1;
    win32_state.bitmap_info.bmiHeader.biBitCount = 8*BYTES_PER_PIXEL;
    win32_state.bitmap_info.bmiHeader.biCompression = BI_RGB;

    s32 back_buffer_memory_size = (back_buffer->pitch*back_buffer->height);
    back_buffer->memory = VirtualAlloc(0, back_buffer_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void
win32_display_back_buffer(OS_Back_Buffer back_buffer, HWND window, s32 window_width, s32 window_height) {
    s32 display_width = back_buffer.width;
    s32 display_height = back_buffer.height;

    s32 offset = 16;
    s32 display_x = offset;
    s32 display_y = offset;
        
    HDC window_device = GetDC(window);
    StretchDIBits(window_device,
                  display_x, display_y, display_width, display_height,
                  0, 0, back_buffer.width, back_buffer.height,
                  back_buffer.memory,
                  &win32_state.bitmap_info,
                  DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(window, window_device);
}

internal void
win32_debug_draw_vertical(OS_Back_Buffer *back_buffer, s32 x, s32 top, s32 bottom, u32 color) {
    if (top <= 0) top = 0;
    if (bottom > back_buffer->height) bottom = back_buffer->height;
    
    if ((x >= 0) && (x < back_buffer->width)) {
        u8 *pixel = ((u8 *)back_buffer->memory + x*BYTES_PER_PIXEL + top*back_buffer->pitch);
        for (s32 y = top;
             y < bottom;
             ++y) {
            *(u32 *)pixel = color;
            pixel += back_buffer->pitch;
        }
    }
}

internal void
win32_draw_sound_marker(OS_Back_Buffer *back_buffer, Win32_Sound_Output sound_output, f32 buffer_width, s32 pad, s32 top, s32 bottom, DWORD value, u32 color) {
    s32 x = pad + (s32)(buffer_width*(f32)value);
    win32_debug_draw_vertical(back_buffer, x, top, bottom, color);
}

internal void
win32_debug_draw_sound_markers(OS_Back_Buffer *back_buffer, u32 marker_count, Win32_Sound_Marker *markers, u32 current_marker_index, Win32_Sound_Output sound_output, f32 target_seconds_per_frame) {
    s32 pad = 16;
    s32 line_height = 64;
        
    f32 buffer_width = (f32)(back_buffer->width - 2*pad)/(f32)sound_output.buffer_size;
    
    for (u32 marker_index = 0;
         marker_index < marker_count;
         ++marker_index) {
        Win32_Sound_Marker *marker = markers + marker_index;
        assert(marker->output_play_cursor < sound_output.buffer_size);
        assert(marker->output_write_cursor < sound_output.buffer_size);
        assert(marker->output_location < sound_output.buffer_size);
        assert(marker->output_byte_count < sound_output.buffer_size);
        assert(marker->flip_play_cursor < sound_output.buffer_size);
        assert(marker->flip_write_cursor < sound_output.buffer_size);
        
        u32 play_color = 0xFFFFFFFF;
        u32 write_color = 0xFFFF0000;
        u32 expected_flip_color = 0xFFFFFF00;

        s32 top = pad;
        s32 bottom = pad + line_height;

        if (marker_index == current_marker_index) {
            top += line_height + pad;
            bottom += line_height + pad;

            int first_top = top;
            
            win32_draw_sound_marker(back_buffer, sound_output, buffer_width, pad, top, bottom, marker->output_play_cursor, play_color);
            win32_draw_sound_marker(back_buffer, sound_output, buffer_width, pad, top, bottom, marker->output_write_cursor, write_color);
            
            top += line_height + pad;
            bottom += line_height + pad;
            
            win32_draw_sound_marker(back_buffer, sound_output, buffer_width, pad, top, bottom, marker->output_location, play_color);
            win32_draw_sound_marker(back_buffer, sound_output, buffer_width, pad, top, bottom, marker->output_location + marker->output_byte_count, write_color);

            top += line_height + pad;
            bottom += line_height + pad;

            win32_draw_sound_marker(back_buffer, sound_output, buffer_width, pad, first_top, bottom, marker->expected_flip_play_cursor, expected_flip_color);
        }
        
        win32_draw_sound_marker(back_buffer, sound_output, buffer_width, pad, top, bottom, marker->flip_play_cursor, play_color);
        win32_draw_sound_marker(back_buffer, sound_output, buffer_width, pad, top, bottom, marker->flip_write_cursor, write_color);
    }
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
win32_work_queue_do_next_entry(OS_Work_Queue *queue) {
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
win32_work_queue_complete(OS_Work_Queue *queue) {
    while (queue->completion_goal != queue->completion_count) {
        win32_work_queue_do_next_entry(queue);
    }
    
    queue->completion_goal = 0;
    queue->completion_count = 0;
}

DWORD WINAPI
win32_thread_proc(LPVOID data) {
    OS_Work_Queue *queue = (OS_Work_Queue *)data;
    for (;;) {
        if (win32_work_queue_do_next_entry(queue)) {
            WaitForSingleObjectEx(queue->semaphore_handle, INFINITE, FALSE);
        }
    }
    //return(0);
}

internal void
win32_init_work_queue(OS_Work_Queue *queue, u32 thread_count) {
    queue->completion_goal = 0;
    queue->completion_count = 0;
    queue->next_entry_to_write = 0;
    queue->next_entry_to_read = 0;
    
    queue->semaphore_handle = CreateSemaphoreEx(0, 0, thread_count, 0, 0, SEMAPHORE_ALL_ACCESS);
    
    for (u32 thread_index = 0;
         thread_index < thread_count;
         ++thread_index) {
        DWORD thread_id;
        HANDLE thread_handle = CreateThread(0, 0, win32_thread_proc, queue, 0, &thread_id);
        CloseHandle(thread_handle);
    }
}

internal u32
win32_process_key_code(u32 key_code) {
    switch (key_code) {
        case VK_ESCAPE: key_code = Key_Escape; break;
        case VK_RETURN: key_code = Key_Enter; break;
        case VK_SHIFT: key_code = Key_Shift; break;
        case VK_SPACE: key_code = Key_Space; break;
                            
        case VK_UP: key_code = Key_Up; break;
        case VK_DOWN: key_code = Key_Down; break;
        case VK_LEFT: key_code = Key_Left; break;
        case VK_RIGHT: key_code = Key_Right; break;
                            
        case '0': key_code = Key_0; break;
        case '1': key_code = Key_1; break;
        case '2': key_code = Key_2; break;
        case '3': key_code = Key_3; break;
        case '4': key_code = Key_4; break;
        case '5': key_code = Key_5; break;
        case '6': key_code = Key_6; break;
        case '7': key_code = Key_7; break;
        case '8': key_code = Key_8; break;
        case '9': key_code = Key_9; break;

        case 'A': key_code = Key_A; break;
        case 'B': key_code = Key_B; break;
        case 'C': key_code = Key_C; break;
        case 'D': key_code = Key_D; break;
        case 'E': key_code = Key_E; break;
        case 'F': key_code = Key_F; break;
        case 'G': key_code = Key_G; break;
        case 'H': key_code = Key_H; break;
        case 'I': key_code = Key_I; break;
        case 'J': key_code = Key_J; break;
        case 'K': key_code = Key_K; break;
        case 'L': key_code = Key_L; break;
        case 'M': key_code = Key_M; break;
        case 'N': key_code = Key_N; break;
        case 'O': key_code = Key_O; break;
        case 'P': key_code = Key_P; break;
        case 'Q': key_code = Key_Q; break;
        case 'R': key_code = Key_R; break;
        case 'S': key_code = Key_S; break;
        case 'T': key_code = Key_T; break;
        case 'U': key_code = Key_U; break;
        case 'V': key_code = Key_V; break;
        case 'W': key_code = Key_W; break;
        case 'X': key_code = Key_X; break;
        case 'Y': key_code = Key_Y; break;
        case 'Z': key_code = Key_Z; break;
                            
        case VK_F1: key_code = Key_F1; break;
        case VK_F2: key_code = Key_F2; break;
        case VK_F3: key_code = Key_F3; break;
        case VK_F4: key_code = Key_F4; break;
        case VK_F5: key_code = Key_F5; break;
        case VK_F6: key_code = Key_F6; break;
        case VK_F7: key_code = Key_F7; break;
        case VK_F8: key_code = Key_F8; break;
        case VK_F9: key_code = Key_F9; break;
        case VK_F10: key_code = Key_F10; break;
        case VK_F11: key_code = Key_F11; break;
        case VK_F12: key_code = Key_F12; break;
        case VK_F13: key_code = Key_F13; break;
        case VK_F14: key_code = Key_F14; break;
        case VK_F15: key_code = Key_F15; break;
        case VK_F16: key_code = Key_F16; break;
        case VK_F17: key_code = Key_F17; break;
        case VK_F18: key_code = Key_F18; break;
        case VK_F19: key_code = Key_F19; break;
        case VK_F20: key_code = Key_F20; break;
        case VK_F21: key_code = Key_F21; break;
        case VK_F22: key_code = Key_F22; break;
        case VK_F23: key_code = Key_F23; break;
        case VK_F24: key_code = Key_F24; break;

        default: key_code = Key_Null; break;
    }
    return(key_code);
}

internal void
win32_update_window_events(OS_Input *input) {
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
                b32 is_down = ((message.lParam & (1 << 31)) == 0);
                key_code = win32_process_key_code(key_code);
                os_process_digital_button(&input->keyboard[key_code], is_down);
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
                        input->mouse.dx += raw_input->data.mouse.lLastX;
                        input->mouse.dy += raw_input->data.mouse.lLastY;

                        USHORT button_flags = raw_input->data.mouse.usButtonFlags;
                        b32 left_button_is_down = input->mouse.left.down;
                        if (button_flags & RI_MOUSE_LEFT_BUTTON_DOWN) left_button_is_down = true;
                        if (button_flags & RI_MOUSE_LEFT_BUTTON_UP) left_button_is_down = false;
                        os_process_digital_button(&input->mouse.left, left_button_is_down);

                        b32 right_button_is_down = input->mouse.right.down;
                        if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_is_down = true;
                        if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_is_down = false;
                        os_process_digital_button(&input->mouse.right, right_button_is_down);
                            
                        b32 middle_button_is_down = input->mouse.middle.down;
                        if (button_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) middle_button_is_down = true;
                        if (button_flags & RI_MOUSE_MIDDLE_BUTTON_UP) middle_button_is_down = false;
                        os_process_digital_button(&input->mouse.middle, middle_button_is_down);

                        b32 x1_button_is_down = input->mouse.x1.down;
                        if (button_flags & RI_MOUSE_BUTTON_4_DOWN) x1_button_is_down = true;
                        if (button_flags & RI_MOUSE_BUTTON_4_UP) x1_button_is_down = false;
                        os_process_digital_button(&input->mouse.x1, x1_button_is_down);
                            
                        b32 x2_button_is_down = input->mouse.x2.down;
                        if (button_flags & RI_MOUSE_BUTTON_5_DOWN) x2_button_is_down = true;
                        if (button_flags & RI_MOUSE_BUTTON_5_UP) x2_button_is_down = false;
                        os_process_digital_button(&input->mouse.x2, x2_button_is_down);
                            
                        if (raw_input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                            input->mouse.delta_wheel += ((SHORT)raw_input->data.mouse.usButtonData) / WHEEL_DELTA;
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
}

internal void
win32_reset_input(OS_Input *input) {
    for (u32 i = 0; i < Key_Count; ++i) {
        input->keyboard[i].pressed = false;
        input->keyboard[i].released = false;
    }

    input->mouse.left.pressed = false;
    input->mouse.left.released = false;
    input->mouse.right.pressed = false;
    input->mouse.right.released = false;
    input->mouse.middle.pressed = false;
    input->mouse.middle.released = false;
    input->mouse.x1.pressed = false;
    input->mouse.x1.released = false;
    input->mouse.x2.pressed = false;
    input->mouse.x2.released = false;
    input->mouse.delta_wheel = 0;
    input->mouse.dx = 0;
    input->mouse.dy = 0;
}

internal void
win32_update_input(OS_Input *input, HWND window) {
    ////////////////////////////
    // NOTE(xkazu0x): upate mouse
    
    POINT mouse_position;
    GetCursorPos(&mouse_position);
    ScreenToClient(window, &mouse_position);

    if (mouse_position.x < 0) mouse_position.x = 0;
    if (mouse_position.y < 0) mouse_position.y = 0;
    
    Win32_Window_Size window_size = win32_get_window_size(window);
    
    if (mouse_position.x > window_size.x) mouse_position.x = window_size.x;
    if (mouse_position.y > window_size.y) mouse_position.y = window_size.y;
        
    input->mouse.x = mouse_position.x;
    input->mouse.y = mouse_position.y;
    
    input->mouse.wheel += input->mouse.delta_wheel;
    
    ////////////////////////////
    // NOTE(xkazu0x): update gamepad
                    
    u32 max_gamepad_count = XUSER_MAX_COUNT;
    if (max_gamepad_count > array_count(input->gamepads)) {
        max_gamepad_count = array_count(input->gamepads);
    }
        
    for (u32 gamepad_index = 0;
         gamepad_index < max_gamepad_count;
         gamepad_index++) {
        XINPUT_STATE xinput_state = {};
        DWORD xinput_result = xinput_get_state(gamepad_index, &xinput_state);
        if (xinput_result == ERROR_SUCCESS) {
            os_process_digital_button(&input->gamepads[gamepad_index].up, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].down, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].left, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].right, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].start, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].back, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].left_thumb,(xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].right_thumb, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].left_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].right_shoulder, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].a, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].b, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].x, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
            os_process_digital_button(&input->gamepads[gamepad_index].y, (xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
            os_process_analog_button(&input->gamepads[gamepad_index].left_trigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD/255.0f, xinput_state.Gamepad.bLeftTrigger/255.0f);
            os_process_analog_button(&input->gamepads[gamepad_index].right_trigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD/255.0f, xinput_state.Gamepad.bRightTrigger/255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
            os_process_stick(&input->gamepads[gamepad_index].left_stick, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE/32767.0f, CONVERT(xinput_state.Gamepad.sThumbLX), CONVERT(xinput_state.Gamepad.sThumbLY));
            os_process_stick(&input->gamepads[gamepad_index].right_stick, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE/32767.0f, CONVERT(xinput_state.Gamepad.sThumbRX), CONVERT(xinput_state.Gamepad.sThumbRY));
#undef CONVERT
        } else {
            break;
        }
    }
}

internal void
win32_begin_input_recording(Win32_State *state, u32 input_recording_index) {
    state->input_recording_index = input_recording_index;

    // TODO(xkazu0x): these files must go in the build directory!!!
    char *filename = "input_recording.excalibur";
    state->recording_handle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    DWORD bytes_to_write = (DWORD)state->game_memory_size;
    assert(state->game_memory_size == bytes_to_write);
    DWORD bytes_written;
    WriteFile(state->recording_handle, state->game_memory, bytes_to_write, &bytes_written, 0);
}

internal void
win32_end_input_recording(Win32_State *state) {
    CloseHandle(state->recording_handle);
    state->input_recording_index = 0;
}

internal void
win32_begin_input_playback(Win32_State *state, u32 input_playback_index) {
    state->input_playback_index = input_playback_index;
    
    // TODO(xkazu0x): these files must go in the build directory!!!
    char *filename = "input_recording.excalibur";
    state->playback_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    DWORD bytes_to_read = (DWORD)state->game_memory_size;
    assert(state->game_memory_size == bytes_to_read);
    
    DWORD bytes_read;
    ReadFile(state->playback_handle, state->game_memory, bytes_to_read, &bytes_read, 0);
}

internal void
win32_end_input_playback(Win32_State *state) {
    CloseHandle(state->playback_handle);
    state->input_playback_index = 0;
}

internal void
win32_record_input(Win32_State *state, OS_Input *input) {
    DWORD bytes_written;
    WriteFile(state->recording_handle, input, sizeof(*input), &bytes_written, 0);
}

internal void
win32_playback_input(Win32_State *state, OS_Input *input) {
    DWORD bytes_read = 0;
    if (ReadFile(state->playback_handle, input, sizeof(*input), &bytes_read, 0)) {
        if (bytes_read == 0) {
            u32 playback_index = state->input_playback_index;
            win32_end_input_playback(state);
            win32_begin_input_playback(state, playback_index);
            ReadFile(state->playback_handle, input, sizeof(*input), &bytes_read, 0);
        }
    }
}

#if 0
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main(void)
#endif
{
    ////////////////////////////////
    // NOTE(xkazu0x): context cracking
    
    String8 operating_system_string = string_from_operating_system(operating_system_from_context());
    String8 architecture_string = string_from_architecture(architecture_from_context());
    String8 compiler_string = string_from_compiler(compiler_from_context());
    
    log_info("operating system: %s", operating_system_string.str);
    log_info("architecture: %s", architecture_string.str);
    log_info("compiler: %s", compiler_string.str);

    ////////////////////////////////
    // NOTE(xkazu0x): init threads
    
    OS_Work_Queue high_priority_queue;
    win32_init_work_queue(&high_priority_queue, 4);
    
    OS_Work_Queue low_priority_queue;
    win32_init_work_queue(&low_priority_queue, 2);

    ////////////////////////////////
    // NOTE(xkazu0x): load game code
    
    u64 string_arena_size = MB(1);
    void *string_arena_memory = VirtualAlloc(0, string_arena_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Arena string_arena = make_arena(string_arena_size, string_arena_memory);

    win32_get_exe_filename(&string_arena, &win32_state);
    log_debug("exec filename: %s", win32_state.exe_filename);
    log_debug("exec fullpath: %s", win32_state.exe_fullpath);
    
    char *source_game_code_dll_fullpath = win32_build_exe_path_filename(&string_arena, &win32_state, "excalibur_game.dll");
    char *temp_game_code_dll_fullpath = win32_build_exe_path_filename(&string_arena, &win32_state, "excalibur_game_temp.dll");
    log_debug("src dll fullpath: %s", source_game_code_dll_fullpath);
    log_debug("tmp dll fullpath: %s", temp_game_code_dll_fullpath);
    
    Win32_Game_Code game = {};
    win32_load_game_code(&game, source_game_code_dll_fullpath, temp_game_code_dll_fullpath);

    ////////////////////////////////
    // NOTE(xkazu0x): monitor info
    
    DEVMODE monitor_info;
    monitor_info.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &monitor_info);
    
    s32 monitor_width = monitor_info.dmPelsWidth;
    s32 monitor_height = monitor_info.dmPelsHeight;
    s32 monitor_frame_rate = monitor_info.dmDisplayFrequency;
    log_info("monitor size: %dx%d", monitor_width, monitor_height);
    log_info("monitor refresh rate: %dHz", monitor_frame_rate);
    
    s32 game_update_hz = 30;
    //s32 game_update_hz = 60;
    //s32 game_update_hz = monitor_frame_rate;
    f32 target_seconds_per_frame = 1.0f / (f32)game_update_hz;

    ////////////////////////////////
    // NOTE(xkazu0x): init back buffer
    
    OS_Back_Buffer back_buffer = {};
    //win32_resize_back_buffer(&back_buffer, 320, 180);
    win32_resize_back_buffer(&back_buffer, 960, 540);
    //win32_resize_back_buffer(&back_buffer, 1920, 1080);
    //win32_resize_back_buffer(&back_buffer, 1279, 719);
    log_info("back buffer size: %dx%d", back_buffer.width, back_buffer.height);

    ////////////////////////////////
    // NOTE(xkazu0x): init window
    
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
    if (window_atom) {
        HWND window = CreateWindowExA(window_style_ex, MAKEINTATOM(window_atom),
                                             window_title, window_style,
                                             window_x, window_y,
                                             fixed_window_width, fixed_window_height,
                                             0, 0, window_instance, 0);
        if (window) {
            ShowWindow(window, SW_SHOW);            

            ////////////////////////////
            // NOTE(xkazu0x): init input
            OS_Input input = {};
    
            RAWINPUTDEVICE raw_input_device = {};
            raw_input_device.usUsagePage = 0x01;
            raw_input_device.usUsage = 0x02;
            raw_input_device.hwndTarget = window;
            if (!RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device))) {
                log_error("failed to register mouse as raw input device");
            }
            log_debug("mouse registered as raw input device");

            win32_init_xinput();

            ////////////////////////////
            // NOTE(xkazu0x): init sound
            
            Win32_Sound_Output sound_output = {};
            sound_output.samples_per_second = 48000;
            sound_output.bytes_per_sample = sizeof(s16)*2;
            sound_output.buffer_size = sound_output.samples_per_second*sound_output.bytes_per_sample;

            win32_init_dsound(window, sound_output.samples_per_second, sound_output.buffer_size, &sound_output.buffer);
            
            sound_output.running_sample_index = 0;
            sound_output.latency_sample_count = 3*(sound_output.samples_per_second/game_update_hz);
            // TODO(xkazu0x): actually compute this variable and see what the lowest reasonable value is.
            sound_output.safety_bytes = (sound_output.samples_per_second*sound_output.bytes_per_sample/game_update_hz)/3;
            
            win32_clear_sound_buffer(&sound_output);
            sound_output.buffer->Play(0, 0, DSBPLAY_LOOPING);
            
            s16 *sound_samples = (s16 *)VirtualAlloc(0, sound_output.buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            b32 sound_is_valid = false;
            DWORD audio_latency_bytes = 0;
            f32 audio_latency_seconds = 0.0f;
            
#if EXCALIBUR_INTERNAL
            u32 debug_sound_marker_index = 0;
            Win32_Sound_Marker debug_sound_markers[16] = {};
#endif
            
            ////////////////////////////////
            // NOTE(xkazu0x): init memory
            
            OS_Memory memory = {};
            
            memory.high_priority_queue = &high_priority_queue;
            memory.low_priority_queue = &low_priority_queue;
            
            memory.os_work_queue_add_entry = win32_work_queue_add_entry;
            memory.os_work_queue_complete = win32_work_queue_complete;
            
            memory.debug_os_free_file = debug_os_free_file;
            memory.debug_os_read_file = debug_os_read_file;
            memory.debug_os_write_file = debug_os_write_file;
            
#if EXCALIBUR_INTERNAL
            LPVOID base_address = (LPVOID)TB(2);
#else
            LPVOID base_address = 0;
#endif
            
            memory.permanent_storage_size = MB(64);
            memory.transient_storage_size = GB(1);
            
            win32_state.game_memory_size = memory.permanent_storage_size + memory.transient_storage_size;
            win32_state.game_memory = VirtualAlloc(base_address, win32_state.game_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            memory.permanent_storage = win32_state.game_memory;
            memory.transient_storage = ((u8 *)memory.permanent_storage + memory.permanent_storage_size);
            
            if (sound_samples && memory.permanent_storage && memory.transient_storage) {
                ////////////////////////////
                // NOTE(xkazu0x): init clock
                
                OS_Clock clock = {};
    
                LARGE_INTEGER large_integer;
                QueryPerformanceFrequency(&large_integer);
                win32_state.time_frequency = large_integer.QuadPart;

                // NOTE(xkazu0x): Set the windows scheduler granularity to 1ms so that our Sleep() can be more granular.
                UINT desired_scheduler_ms = 1;
                b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
                
                LARGE_INTEGER last_counter = win32_get_wall_clock();
                LARGE_INTEGER flip_wall_clock = win32_get_wall_clock();
                u64 last_cycle_count = __rdtsc();
                
                while (!quit) {
                    ////////////////////////////
                    // NOTE(xkazu0x): reload game
                    
                    FILETIME game_new_write_time = win32_get_last_write_time(source_game_code_dll_fullpath);
                    if (CompareFileTime(&game_new_write_time, &game.last_write_time) != 0) {
                        win32_unload_game_code(&game);
                        win32_load_game_code(&game, source_game_code_dll_fullpath, temp_game_code_dll_fullpath);
                    }

                    ////////////////////////////
                    // NOTE(xkazu0x): update window events and input
                    
                    win32_reset_input(&input);
                    win32_update_window_events(&input);
                    win32_update_input(&input, window);
                    
                    ////////////////////////////
                    // NOTE(xkazu0x): game update and render

                    if (input.keyboard[Key_Escape].pressed) quit = true;
                    if (input.keyboard[Key_F11].pressed) win32_window_toggle_fullscreen(window, &win32_state.window_placement);

#if EXCALIBUR_INTERNAL
                    if (input.keyboard[Key_F1].pressed) pause = !pause;
#endif

                    if (input.keyboard[Key_L].pressed) {
                        if (win32_state.input_recording_index == 0) {
                            win32_begin_input_recording(&win32_state, 1);
                        } else {
                            win32_end_input_recording(&win32_state);
                            win32_begin_input_playback(&win32_state, 1);
                        }
                    }
                    
                    if (!pause) {
                        clock.dt = target_seconds_per_frame;

                        if (win32_state.input_recording_index) {
                            win32_record_input(&win32_state, &input);
                        }

                        if (win32_state.input_playback_index) {
                            win32_playback_input(&win32_state, &input);
                        }
                        
                        if (game.update_and_render) {
                            game.update_and_render(&memory, &back_buffer, &input, &clock);
                            handle_debug_cycle_counters(&memory);
                        }

                        ////////////////////////////
                        // NOTE(xkazu0x): compute and output sound

                        LARGE_INTEGER audio_wall_clock = win32_get_wall_clock();
                        f32 from_begin_to_audio_seconds = win32_get_seconds_elapsed(flip_wall_clock, audio_wall_clock);
                        
                        DWORD play_cursor;
                        DWORD write_cursor;
                        if (sound_output.buffer->GetCurrentPosition(&play_cursor, &write_cursor) == DS_OK) {
                            // NOTE(xkauz0x):
                        
                            // Here is how sound output computation works.

                            // We define a safety value that is a number
                            // of samples we think our game update loop
                            // may very by (let's say up to 2ms).
                    
                            // When we wake up to write audio, we will look
                            // and see what the play cursor position is and we
                            // will forecast ahead where we think the play
                            // cursor will be on the next frame boundary.

                            // We will then look to see if the write cursor is
                            // before that by at least our safety value. If it is, the target
                            // fill position is that frame boundary plus one frame.
                            // This giving us perfect audio sync in the case of
                            // a card that has low enough latency.

                            // If the write cursor if _after_ that safety
                            // margin, then we assume we can never sync the
                            // audio perfectly, so we will write one frame's
                            // worth of audio plus the safety margin's worth
                            // of guard samples.

                            if (!sound_is_valid) {
                                sound_output.running_sample_index = write_cursor/sound_output.bytes_per_sample;
                                sound_is_valid = true;
                            }

                            DWORD byte_to_lock = ((sound_output.running_sample_index*sound_output.bytes_per_sample) % sound_output.buffer_size);

                            DWORD expected_sound_bytes_per_frame = (sound_output.samples_per_second*sound_output.bytes_per_sample)/game_update_hz;

                            f32 seconds_left_until_flip = (target_seconds_per_frame - from_begin_to_audio_seconds);
                            DWORD expected_bytes_until_flip = (DWORD)((seconds_left_until_flip/target_seconds_per_frame)*(f32)expected_sound_bytes_per_frame);
                            
                            DWORD expected_frame_boundary_byte = play_cursor + expected_sound_bytes_per_frame;
                            //DWORD expected_frame_boundary_byte = play_cursor + expected_bytes_until_flip;

                            DWORD safe_write_cursor = write_cursor;
                            if (safe_write_cursor < play_cursor) {
                                safe_write_cursor += sound_output.buffer_size;
                            }
                            assert(safe_write_cursor >= play_cursor);
                            safe_write_cursor += sound_output.safety_bytes;
                        
                            b32 audio_card_is_low_latency = (safe_write_cursor < expected_frame_boundary_byte);
                        
                            DWORD target_cursor = 0;
                            if (audio_card_is_low_latency) {
                                target_cursor = expected_frame_boundary_byte + expected_sound_bytes_per_frame;
                            } else {
                                target_cursor = write_cursor + expected_sound_bytes_per_frame + sound_output.safety_bytes;
                            }
                            target_cursor = target_cursor % sound_output.buffer_size;
                        
                            DWORD bytes_to_write = 0;                            
                            if (byte_to_lock > target_cursor) {
                                bytes_to_write = sound_output.buffer_size - byte_to_lock;
                                bytes_to_write += target_cursor;
                            } else {
                                bytes_to_write = target_cursor - byte_to_lock;
                            }
                    
                            OS_Sound_Buffer sound_buffer = {};
                            sound_buffer.samples_per_second = sound_output.samples_per_second;
                            sound_buffer.sample_count = bytes_to_write/sound_output.bytes_per_sample;
                            sound_buffer.samples = sound_samples;

                            if (game.get_sound_samples) {
                                game.get_sound_samples(&memory, &sound_buffer);
                            }
                        
                            win32_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);

#if EXCALIBUR_INTERNAL
                            Win32_Sound_Marker *marker = debug_sound_markers + debug_sound_marker_index;
                            marker->output_play_cursor = play_cursor;
                            marker->output_write_cursor = write_cursor;
                            marker->output_location = byte_to_lock;
                            marker->output_byte_count = bytes_to_write;
                            marker->expected_flip_play_cursor = expected_frame_boundary_byte;
                        
                            DWORD unwrapped_write_cursor = write_cursor;
                            if (unwrapped_write_cursor < play_cursor) {
                                unwrapped_write_cursor += sound_output.buffer_size;
                            }
                            audio_latency_bytes = unwrapped_write_cursor - play_cursor;
                            audio_latency_seconds = ((f32)audio_latency_bytes/(f32)sound_output.bytes_per_sample)/(f32)sound_output.samples_per_second;
#endif
                        } else {
                            sound_is_valid = false;
                        }

                        ////////////////////////////
                        // NOTE(xkazu0x): limit video frame rate
                    
                        u64 raw_end_cycle_count = __rdtsc();
                    
                        u64 raw_cycles_per_frame = raw_end_cycle_count - last_cycle_count;
                        f32 raw_mega_cycles_per_frame = ((f32)raw_cycles_per_frame/(1000.0f*1000.0f));
        
                        LARGE_INTEGER raw_end_counter = win32_get_wall_clock();
                    
                        f32 raw_seconds_per_frame = win32_get_seconds_elapsed(last_counter, raw_end_counter);
                        f32 raw_ms_per_frame = 1000.0f*raw_seconds_per_frame;
                        f32 raw_frames_per_second = (f32)win32_state.time_frequency/(f32)(raw_end_counter.QuadPart - last_counter.QuadPart);

                        f32 seconds_elapsed_for_frame = raw_seconds_per_frame;
                        if (seconds_elapsed_for_frame < target_seconds_per_frame) {
                            if (sleep_is_granular) {
                                DWORD sleep_ms = (DWORD)(1000.0f*(target_seconds_per_frame - seconds_elapsed_for_frame));
                                if (sleep_ms > 0) {
                                    Sleep(sleep_ms);
                                }
                            }
                        
                            f32 test_seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
                            if (test_seconds_elapsed_for_frame < target_seconds_per_frame) {
                                // TODO(xkazu0x): LOG MISSED SLEEP HERE!
                            }
                        
                            while (seconds_elapsed_for_frame < target_seconds_per_frame) {
                                seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter, win32_get_wall_clock());
                            }
                        } else {
                            // TODO(xkazu0x): MISSED FRAME RATE!
                        }

                        ////////////////////////////
                        // NOTE(xkazu0x): compute time
                    
                        u64 end_cycle_count = __rdtsc(); 
                    
                        u64 cycles_per_frame = end_cycle_count - last_cycle_count;
                        f32 mega_cycles_per_frame = ((f32)cycles_per_frame/(1000.0f*1000.0f));
                    
                        LARGE_INTEGER end_counter = win32_get_wall_clock();
                    
                        f32 ms_per_frame = 1000.0f*win32_get_seconds_elapsed(last_counter, end_counter);
                        f32 frames_per_second = (f32)win32_state.time_frequency/(f32)(end_counter.QuadPart - last_counter.QuadPart);
                    
                        last_counter = end_counter;
                        last_cycle_count = end_cycle_count;

                        ////////////////////////////
                        // NOTE(xkazu0x): display back buffer
                    
#if EXCALIBUR_INTERNAL 
                        if (!pause) {
                            win32_debug_draw_sound_markers(&back_buffer, array_count(debug_sound_markers), debug_sound_markers, debug_sound_marker_index - 1, sound_output, target_seconds_per_frame);
                        }
#endif
                    
                        Win32_Window_Size window_size = win32_get_window_size(window);
                        win32_display_back_buffer(back_buffer, window, window_size.x, window_size.y);

                        flip_wall_clock = win32_get_wall_clock();
#if EXCALIBUR_INTERNAL
                        {
                            DWORD play_cursor;
                            DWORD write_cursor;
                            if (sound_output.buffer->GetCurrentPosition(&play_cursor, &write_cursor) == DS_OK) {
                                assert(debug_sound_marker_index < array_count(debug_sound_markers));
                                Win32_Sound_Marker *marker = debug_sound_markers + debug_sound_marker_index;
                                marker->flip_play_cursor = play_cursor;
                                marker->flip_write_cursor = write_cursor;
                            }
                        }
#endif
                    
#if 1 // NOTE(xkazu0x): debug loggin time measurements
                        {
                            local f64 time_ms = 0.0;
                            local f64 last_print_time = 0.0;
                            time_ms += ms_per_frame;
                            if (time_ms - last_print_time > 500.0f) {
                                char new_window_title[512];
                                sprintf(new_window_title, "%s [fixed: %.02ffps, %.02fms, %.02fmc] [raw: %.02ffps, %.02fms, %.02fmc]",
                                        window_title,
                                        frames_per_second, ms_per_frame, mega_cycles_per_frame,
                                        raw_frames_per_second, raw_ms_per_frame, raw_mega_cycles_per_frame);
                                SetWindowTextA(window, new_window_title);
                                last_print_time = time_ms;
                            }
                        }
#endif
                    
#if EXCALIBUR_INTERNAL                    
                        ++debug_sound_marker_index;
                        if (debug_sound_marker_index == array_count(debug_sound_markers)) {
                            debug_sound_marker_index = 0;
                        }
#endif
                    }
                }
            }
        }
    }
    
    return(0);
}
