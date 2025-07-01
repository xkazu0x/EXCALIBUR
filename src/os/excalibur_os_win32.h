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

struct Win32_Window_Size {
    s32 x;
    s32 y;
};

struct Win32_Game_Code {
    FILETIME last_write_time;
    HMODULE library;

    // NOTE(xkazu0x): the callback can be 0
    // you must check before calling
    Game_Update_And_Render *update_and_render;
    Game_Get_Sound_Samples *get_sound_samples;
};

struct Win32_Sound_Marker {
    DWORD output_play_cursor;
    DWORD output_write_cursor;
    DWORD output_location;
    DWORD output_byte_count;
    DWORD expected_flip_play_cursor;
    
    DWORD flip_play_cursor;
    DWORD flip_write_cursor;
};

struct Win32_Sound_Output {
    s32 samples_per_second;
    s32 bytes_per_sample;
    DWORD buffer_size;
    LPDIRECTSOUNDBUFFER buffer;
    
    u32 running_sample_index;
    s32 latency_sample_count;
    DWORD safety_bytes;
};

#define WIN32_FILENAME_MAX MAX_PATH
struct Win32_State {
    BITMAPINFO bitmap_info;
    WINDOWPLACEMENT window_placement;
    s64 time_frequency;

    u64 game_memory_size;
    void *game_memory;
    
    char *exe_fullpath;
    char *exe_filename;

    HANDLE recording_handle;
    u32 input_recording_index = 0;

    HANDLE playback_handle;
    u32 input_playback_index = 0;
};

#define WIN32_GET_PROC_ADDR(v, m, s) (*(PROC*)(&(v))) = GetProcAddress((m), (s))

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_STATE *)
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_VIBRATION *)
typedef XINPUT_GET_STATE(XInput_Get_State);
typedef XINPUT_SET_STATE(XInput_Set_State);

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND *ppDS, LPUNKNOWN  pUnkOuter);
typedef DIRECT_SOUND_CREATE(Direct_Sound_Create);

#endif // EXCALIBUR_OS_WIN32_H
