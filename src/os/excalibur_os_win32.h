#ifndef EXCALIBUR_OS_WIN32_H
#define EXCALIBUR_OS_WIN32_H

#define NO_MIN_MAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#include <xinput.h>
#include <mmreg.h>
#include <dsound.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "winmm")

struct Win32_Sound_Output {
    s32 samples_per_second;
    s32 bytes_per_sample;
    s32 buffer_size;
    LPDIRECTSOUNDBUFFER buffer;
    
    u32 running_sample_index;
    s32 latency_sample_count;
    s16 wave_tone_volume;
    s32 wave_tone_hz;
    s32 wave_period;
};

struct Win32_Game {
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
    
    char *exe_fullpath;
    char *exe_filename;
};

#define WIN32_GET_PROC_ADDR(v, m, s) (*(PROC*)(&(v))) = GetProcAddress((m), (s))

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_STATE *)
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_VIBRATION *)
typedef XINPUT_GET_STATE(XInput_Get_State);
typedef XINPUT_SET_STATE(XInput_Set_State);

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND *ppDS, LPUNKNOWN  pUnkOuter);
typedef DIRECT_SOUND_CREATE(Direct_Sound_Create);

#endif // EXCALIBUR_OS_WIN32_H
