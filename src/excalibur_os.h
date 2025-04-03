#ifndef EXCALIBUR_OS_H
#define EXCALIBUR_OS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct os_thread_t {
    u32 handle;
} os_thread_t;

#if EXCALIBUR_INTERNAL
// IMPORTANT(xkazu0x):
// These are NOT for doing anything in the shipping game - they are
// blocking and the write doesn't protect against lost data
typedef struct debug_file_t {
    u32 size;
    void *data;
} debug_file_t;

#define DEBUG_OS_FREE_FILE(name) void name(os_thread_t *thread, void *memory)
typedef DEBUG_OS_FREE_FILE(debug_os_free_file_t);

#define DEBUG_OS_READ_FILE(name) debug_file_t name(os_thread_t *thread, char *filename)
typedef DEBUG_OS_READ_FILE(debug_os_read_file_t);

#define DEBUG_OS_WRITE_FILE(name) b32 name(os_thread_t *thread, char *filename, u32 memory_size, void *memory)
typedef DEBUG_OS_WRITE_FILE(debug_os_write_file_t);
#endif

///////////////////////////////////////////////////////////////////////
// NOTE(xkazu0x): services that the game provides to the platform layer
// TODO(xkazu0x): key codes for keyboard input
    
typedef enum key_t {
    KEY_SHIFT  = 0x10,
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

    KEY_F1 = 0x70,
    KEY_F2 = 0x71,
    KEY_F3 = 0x72,
    KEY_F4 = 0x73,
    KEY_F5 = 0x74,
    KEY_F6 = 0x75,
    KEY_F7 = 0x76,
    KEY_F8 = 0x77,
    KEY_F9 = 0x78,
    KEY_F10 = 0x79,
    KEY_F11 = 0x7A,
    KEY_F12 = 0x7B,
    KEY_F13 = 0x7C,
    KEY_F14 = 0x7D,
    KEY_F15 = 0x7E,
    KEY_F16 = 0x7F,
    KEY_F17 = 0x80,
    KEY_F18 = 0x81,
    KEY_F19 = 0x82,
    KEY_F20 = 0x83,
    KEY_F21 = 0x84,
    KEY_F22 = 0x85,
    KEY_F23 = 0x86,
    KEY_F24 = 0x87,

    KEY_MAX = 256,
} key_t;

typedef struct digital_button_t {
    b32 down;
    b32 pressed;
    b32 released;
} digital_button_t;

typedef struct analog_button_t {
    f32 value;
    b32 down;
    b32 pressed;
    b32 released;
} analog_button_t;

typedef struct stick_t {
    f32 x;
    f32 y;
} stick_t;

typedef struct gamepad_t {
    digital_button_t up;
    digital_button_t down;
    digital_button_t left;
    digital_button_t right;
    digital_button_t start;
    digital_button_t back;
    digital_button_t left_thumb;
    digital_button_t right_thumb;
    digital_button_t left_shoulder;
    digital_button_t right_shoulder;
    digital_button_t a;
    digital_button_t b;
    digital_button_t x;
    digital_button_t y;
    analog_button_t left_trigger;
    analog_button_t right_trigger;
    stick_t left_stick;
    stick_t right_stick;
} gamepad_t;

typedef struct mouse_t {
    digital_button_t left;
    digital_button_t right;
    digital_button_t middle;
    digital_button_t x1;
    digital_button_t x2;
    s32 wheel;
    s32 delta_wheel;
    vec2i position;
    vec2i delta_position;
} mouse_t;

#define GAMEPAD_COUNT_MAX 4
typedef struct os_input_t {
    digital_button_t keyboard[KEY_MAX];
    mouse_t mouse;
    gamepad_t gamepads[GAMEPAD_COUNT_MAX];
} os_input_t;

typedef struct os_bitmap_t {
    vec2i size;
    s32 bytes_per_pixel;
    s32 pitch;
    void *pixels;
} os_bitmap_t;

typedef struct os_memory_t {
    b32 initialized;
    
    u64 permanent_storage_size;
    u64 transient_storage_size;
    
    void *permanent_storage; // NOTE(xkazu0x): required to be cleared to zero at startup
    void *transient_storage; // NOTE(xkazu0x): required to be cleared to zero at startup

    debug_os_free_file_t *debug_os_free_file;
    debug_os_read_file_t *debug_os_read_file;
    debug_os_write_file_t *debug_os_write_file;
} os_memory_t;

typedef struct os_clock_t {
    f32 delta_seconds;
} os_clock_t;

#define GAME_UPDATE_AND_RENDER(name) void name(os_bitmap_t *bitmap, os_input_t *input, os_memory_t *memory, os_clock_t *clock, os_thread_t *thread)
typedef GAME_UPDATE_AND_RENDER(GAMEUPDATEANDRENDER);

#ifdef __cplusplus
}
#endif

#endif // EXCALIBUR_OS_H
