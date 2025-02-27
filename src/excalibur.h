#ifndef EXCALIBUR_GAME_H
#define EXCALIBUR_GAME_H

#include "excalibur_base.h"
#include "excalibur_math.h"
#include "excalibur_logger.h"

#include "excalibur_base.cpp"
#include "excalibur_math.cpp"
#include "excalibur_logger.cpp"

#define KEY_MAX 256

inline u32
safe_truncate_u64(u64 value) {
    EX_ASSERT(value <= u32_max);
    u32 result = (u32)value;
    return(result);
}

///////////////////////////////////////////////////////////////////////
// NOTE(xkazu0x): services that the platform layer provides to the game
#if EXCALIBUR_INTERNAL
/* IMPORTANT(xkazu0x):
   These are NOT for doing anything in the shipping game - they are
   blocking and the write doesn't protect against lost data
 */
struct debug_file_handle {
    u32 size;
    void *memory;
};

#define DEBUG_PLATFORM_FREE_FILE(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE(DEBUGPLATFORMFREEFILE);

#define DEBUG_PLATFORM_READ_FILE(name) debug_file_handle name(char *filename)
typedef DEBUG_PLATFORM_READ_FILE(DEBUGPLATFORMREADFILE);

#define DEBUG_PLATFORM_WRITE_FILE(name) b32 name(char *filename, u32 memory_size, void *memory)
typedef DEBUG_PLATFORM_WRITE_FILE(DEBUGPLATFORMWRITEFILE);
#endif

///////////////////////////////////////////////////////////////////////
// NOTE(xkazu0x): services that the game provides to the platform layer
// TODO(xkazu0x): key codes for keyboard input
enum keyboard_key {
    KEY_ESCAPE = 0x1B,
    KEY_LEFT   = 0x25,
    KEY_UP     = 0x26,
    KEY_RIGHT  = 0x27,
    KEY_DOWN   = 0x28,
    
    KEY_0 = 0x30,
    KEY_1 = 0x31,
    KEY_2 = 0x32,
    KEY_3 = 0x33,
    KEY_4 = 0x34,
    KEY_5 = 0x35,
    KEY_6 = 0x36,
    KEY_7 = 0x37,
    KEY_8 = 0x38,
    KEY_9 = 0x39,

    KEY_A = 0x41,
    KEY_B = 0x42,
    KEY_C = 0x43,
    KEY_D = 0x44,
    KEY_E = 0x45,
    KEY_F = 0x46,
    KEY_G = 0x47,
    KEY_H = 0x48,
    KEY_I = 0x49,
    KEY_J = 0x4A,
    KEY_K = 0x4B,
    KEY_L = 0x4C,
    KEY_M = 0x4D,
    KEY_N = 0x4E,
    KEY_O = 0x4F,
    KEY_P = 0x50,
    KEY_Q = 0x51,
    KEY_R = 0x52,
    KEY_S = 0x53,
    KEY_T = 0x54,
    KEY_U = 0x55,
    KEY_V = 0x56,
    KEY_W = 0x57,
    KEY_X = 0x58,
    KEY_Y = 0x59,
    KEY_Z = 0x5A,
};

struct digital_button {
    b32 down;
    b32 pressed;
    b32 released;
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

struct mouse_input {
    digital_button left;
    digital_button right;
    digital_button middle;
    s32 wheel;
    s32 delta_wheel;
    vec2i position;
    vec2i delta_position;
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

struct game_memory {
    b32 initialized;
    u64 permanent_storage_size;
    u64 transient_storage_size;
    void *permanent_storage;
    void *transient_storage;

    DEBUGPLATFORMFREEFILE *debug_platform_free_file;
    DEBUGPLATFORMREADFILE *debug_platform_read_file;
    DEBUGPLATFORMWRITEFILE *debug_platform_write_file;
};

struct game_clock {
    // TODO(xkazu0x):
    f32 delta_seconds;
};

struct game_input {
    digital_button keyboard[KEY_MAX];
    mouse_input mouse;
    gamepad_input gamepads[4];
};

struct game_backbuffer {
    vec2i size;
    s32 pitch;
    void *memory;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *memory, game_input *input, game_backbuffer *backbuffer)
typedef GAME_UPDATE_AND_RENDER(GAMEUPDATEANDRENDER);

extern "C" {
    GAME_UPDATE_AND_RENDER(_game_update_and_render) {}
    GAME_UPDATE_AND_RENDER(game_update_and_render);
}

///////////////////
///////////////////
struct game_state {
    vec2i offset;
};

#endif // EXCALIBUR_GAME_H
