#ifndef EXCALIBUR_WIN32_H
#define EXCALIBUR_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <xinput.h>

struct win32_game_t {
    b32 loaded;
    HMODULE library;
    FILETIME last_write_time;

    // NOTE(xkazu0x): the callback can be 0
    // you must check before calling
    GAMEUPDATEANDRENDER *update_and_render;
};

#define WIN32_FILENAME_MAX MAX_PATH

struct win32_t {
    BITMAPINFO bitmap_info;
    WINDOWPLACEMENT window_placement;
    s64 time_frequency;

    u64 game_memory_size;
    void *game_memory_block;
    
    char exe_filename[WIN32_FILENAME_MAX];
    char *one_past_last_exe_filename_slash;
};

#define WIN32_GET_PROC_ADDR(v, m, s) (*(PROC*)(&(v))) = GetProcAddress((m), (s))

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_STATE *)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_VIBRATION *)

typedef X_INPUT_GET_STATE(XINPUTGETSTATE);
typedef X_INPUT_SET_STATE(XINPUTSETSTATE);

#endif // EXCALIBUR_WIN32_H
