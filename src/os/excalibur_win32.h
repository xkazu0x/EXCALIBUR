#ifndef EXCALIBUR_WIN32_H
#define EXCALIBUR_WIN32_H

#define NO_MIN_MAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <xinput.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "winmm")

struct Win32_Window_Size {
    s32 x;
    s32 y;
};

struct Win32_Game {
    b32 loaded;
    HMODULE library;
    FILETIME last_write_time;

    // NOTE(xkazu0x): the callback can be 0
    // you must check before calling
    Game_Update_And_Render *update_and_render;
};

#define WIN32_FILENAME_MAX MAX_PATH
struct Win32_State {
    BITMAPINFO bitmap_info;
    WINDOWPLACEMENT window_placement;
    s64 time_frequency;

    u64 game_memory_size;
    void *game_memory_block;
    
    char exe_filename[WIN32_FILENAME_MAX];
    char *one_past_last_exe_filename_slash;
};

#define WIN32_GET_PROC_ADDR(v, m, s) (*(PROC*)(&(v))) = GetProcAddress((m), (s))

#define XINPUT_GET_STATE(x) DWORD WINAPI x(DWORD, XINPUT_STATE *)
#define XINPUT_SET_STATE(x) DWORD WINAPI x(DWORD, XINPUT_VIBRATION *)

typedef XINPUT_GET_STATE(XInput_Get_State);
typedef XINPUT_SET_STATE(XInput_Set_State);

#endif // EXCALIBUR_WIN32_H
