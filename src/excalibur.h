#ifndef EXCALIBUR_H
#define EXCALIBUR_H

#include "excalibur_base.h"
#include "excalibur_math.h"
#include "excalibur_logger.h"

#include "excalibur_base.cpp"
#include "excalibur_math.cpp"
#include "excalibur_logger.cpp"

struct thread_context {
    u32 place_holder;
};

///////////////////////////////////////////////////////////////////////
// NOTE(xkazu0x): services that the platform layer provides to the game

#if EXCALIBUR_INTERNAL
// IMPORTANT(xkazu0x):
// These are NOT for doing anything in the shipping game - they are
// blocking and the write doesn't protect against lost data
struct debug_file_handle {
    u32 size;
    void *memory;
};

#define DEBUG_PLATFORM_FREE_FILE(name) void name(thread_context *thread, void *memory)
typedef DEBUG_PLATFORM_FREE_FILE(DEBUGPLATFORMFREEFILE);

#define DEBUG_PLATFORM_READ_FILE(name) debug_file_handle name(thread_context *thread, char *filename)
typedef DEBUG_PLATFORM_READ_FILE(DEBUGPLATFORMREADFILE);

#define DEBUG_PLATFORM_WRITE_FILE(name) b32 name(thread_context *thread, char *filename, u32 memory_size, void *memory)
typedef DEBUG_PLATFORM_WRITE_FILE(DEBUGPLATFORMWRITEFILE);
#endif

///////////////////////////////////////////////////////////////////////
// NOTE(xkazu0x): services that the game provides to the platform layer
// TODO(xkazu0x): key codes for keyboard input

enum keyboard_key {
    KK_ESCAPE = 0x1B,
    KK_LEFT   = 0x25,
    KK_UP     = 0x26,
    KK_RIGHT  = 0x27,
    KK_DOWN   = 0x28,
    
    KK_0 = 0x30,
    KK_1 = 0x31,
    KK_2 = 0x32,
    KK_3 = 0x33,
    KK_4 = 0x34,
    KK_5 = 0x35,
    KK_6 = 0x36,
    KK_7 = 0x37,
    KK_8 = 0x38,
    KK_9 = 0x39,

    KK_A = 0x41,
    KK_B = 0x42,
    KK_C = 0x43,
    KK_D = 0x44,
    KK_E = 0x45,
    KK_F = 0x46,
    KK_G = 0x47,
    KK_H = 0x48,
    KK_I = 0x49,
    KK_J = 0x4A,
    KK_K = 0x4B,
    KK_L = 0x4C,
    KK_M = 0x4D,
    KK_N = 0x4E,
    KK_O = 0x4F,
    KK_P = 0x50,
    KK_Q = 0x51,
    KK_R = 0x52,
    KK_S = 0x53,
    KK_T = 0x54,
    KK_U = 0x55,
    KK_V = 0x56,
    KK_W = 0x57,
    KK_X = 0x58,
    KK_Y = 0x59,
    KK_Z = 0x5A,

    KK_MAX = 256,
};

struct digital_button {
    b32 down;
    b32 pressed;
    b32 released;
};

enum mouse_button {
    MB_LEFT,
    MB_RIGHT,
    MB_MIDDLE,
    MB_X1,
    MB_X2,
    MB_MAX,
};

struct mouse_input {
    union {
        struct {
            digital_button left;
            digital_button right;
            digital_button middle;
            digital_button x1;
            digital_button x2;
        };
        digital_button buttons[MB_MAX];
    };
    s32 wheel;
    s32 delta_wheel;
    vec2i position;
    vec2i delta_position;
};

struct analog_button {
    f32 threshold;
    f32 value;
    b32 down;
    b32 pressed;
    b32 released;
};

struct stick {
    f32 threshold;
    f32 x;
    f32 y;
};

struct gamepad_input {
    digital_button up;
    digital_button down;
    digital_button left;
    digital_button right;
    digital_button start;
    digital_button back;
    digital_button left_thumb;
    digital_button right_thumb;
    digital_button left_shoulder;
    digital_button right_shoulder;
    digital_button a;
    digital_button b;
    digital_button x;
    digital_button y;
    analog_button left_trigger;
    analog_button right_trigger;
    stick left_stick;
    stick right_stick;
};

#define GAMEPAD_COUNT_MAX 4
struct game_input {
    mouse_input mouse;
    gamepad_input gamepads[GAMEPAD_COUNT_MAX];
    digital_button keyboard[KK_MAX];
};

struct game_bitmap {
    vec2i size;
    s32 bytes_per_pixel;
    s32 pitch;
    void *memory;
};

struct game_clock {
    // TODO(xkazu0x):
    f32 delta_seconds;
};

struct game_memory {
    b32 initialized;
    
    u64 permanent_storage_size;
    u64 transient_storage_size;
    
    void *permanent_storage; // NOTE(xkazu0x): required to be cleared to zero at startup
    void *transient_storage; // NOTE(xkazu0x): required to be cleared to zero at startup

    DEBUGPLATFORMFREEFILE *debug_platform_free_file;
    DEBUGPLATFORMREADFILE *debug_platform_read_file;
    DEBUGPLATFORMWRITEFILE *debug_platform_write_file;
};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *thread, game_memory *memory, game_input *input, game_bitmap *bitmap)
typedef GAME_UPDATE_AND_RENDER(GAMEUPDATEANDRENDER);

extern "C" {
    GAME_UPDATE_AND_RENDER(game_update_and_render);
}

///////////////////
///////////////////
struct game_state {
    vec2i offset;
    vec2i player_pos;
};

#endif // EXCALIBUR_H
