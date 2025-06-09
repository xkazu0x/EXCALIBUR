#ifndef EXCALIBUR_OS_H
#define EXCALIBUR_OS_H
    
typedef struct OS_Thread OS_Thread;
struct OS_Thread {
    u32 handle;
};

#if EXCALIBUR_INTERNAL
// IMPORTANT(xkazu0x): These are NOT for doing anything in the shipping game
// they are blocking and the write doesn't protect against lost data

typedef struct Debug_OS_File Debug_OS_File;
struct Debug_OS_File {
    u32 size;
    void *data;
};

# define DEBUG_OS_FREE_FILE(x)  void x(OS_Thread *thread, void *memory)
# define DEBUG_OS_READ_FILE(x)  Debug_OS_File x(OS_Thread *thread, char *filename)
# define DEBUG_OS_WRITE_FILE(x) b32 x(OS_Thread *thread, char *filename, u32 memory_size, void *memory)

typedef DEBUG_OS_FREE_FILE(Debug_OS_Free_File);
typedef DEBUG_OS_READ_FILE(Debug_OS_Read_File);
typedef DEBUG_OS_WRITE_FILE(Debug_OS_Write_File);

enum {
    /* 0 */ DebugCycleCounter_game_update_and_render,
    /* 1 */ DebugCycleCounter_render_group_draw,
    /* 2 */ DebugCycleCounter_draw_rect_slowly,
    /* 3 */ DebugCycleCounter_process_pixel,
    /* 4 */ DebugCycleCounter_draw_rect_quickly,
    DebugCycleCounter_Count,
};

typedef struct Debug_Cycle_Counter Debug_Cycle_Counter;
struct Debug_Cycle_Counter {
    u64 cycle_count;
    u32 hit_count;
};

extern struct OS_Memory *debug_global_memory;

# if COMPILER_MSVC
#  include <intrin.h>
# elif COMPILER_CLANG || COMPILER GCC
#  include <x86intrin.h>
# else
#  error SSE/NEON optimizations are not available for this compiler.
# endif

# define BEGIN_TIMED_BLOCK(id) u64 start_cycle_count_##id = __rdtsc();
# define END_TIMED_BLOCK(id) debug_global_memory->counters[DebugCycleCounter_##id].cycle_count += __rdtsc() - start_cycle_count_##id; ++debug_global_memory->counters[DebugCycleCounter_##id].hit_count;
// TODO(xkazu0x): Clamp END_TIMED_BLOCK_COUNTED so that if the calc is wrong, it won't overflow!
# define END_TIMED_BLOCK_COUNTED(id, count) debug_global_memory->counters[DebugCycleCounter_##id].cycle_count += __rdtsc() - start_cycle_count_##id; debug_global_memory->counters[DebugCycleCounter_##id].hit_count += (count);
#endif

///////////////////////////////////////////////////////////////////////
// NOTE(xkazu0x): services that the game provides to the platform layer
// TODO(xkazu0x): key codes for keyboard input
    
typedef u32 Key_Code;
enum {
    Key_Enter  = 0x0D,
    Key_Shift  = 0x10,
    Key_Escape = 0x1B,
    Key_Space  = 0x20,
    
    Key_Left   = 0x25,
    Key_Up     = 0x26,
    Key_Right  = 0x27,
    Key_Down   = 0x28,
    
    Key_0      = 0x30,
    Key_1      = 0x31,
    Key_2      = 0x32,
    Key_3      = 0x33,
    Key_4      = 0x34,
    Key_5      = 0x35,
    Key_6      = 0x36,
    Key_7      = 0x37,
    Key_8      = 0x38,
    Key_9      = 0x39,
    
    Key_A      = 0x41,
    Key_B      = 0x42,
    Key_C      = 0x43,
    Key_D      = 0x44,
    Key_E      = 0x45,
    Key_F      = 0x46,
    Key_G      = 0x47,
    Key_H      = 0x48,
    Key_I      = 0x49,
    Key_J      = 0x4A,
    Key_K      = 0x4B,
    Key_L      = 0x4C,
    Key_M      = 0x4D,
    Key_N      = 0x4E,
    Key_O      = 0x4F,
    Key_P      = 0x50,
    Key_Q      = 0x51,
    Key_R      = 0x52,
    Key_S      = 0x53,
    Key_T      = 0x54,
    Key_U      = 0x55,
    Key_V      = 0x56,
    Key_W      = 0x57,
    Key_X      = 0x58,
    Key_Y      = 0x59,
    Key_Z      = 0x5A,
    
    Key_F1     = 0x70,
    Key_F2     = 0x71,
    Key_F3     = 0x72,
    Key_F4     = 0x73,
    Key_F5     = 0x74,
    Key_F6     = 0x75,
    Key_F7     = 0x76,
    Key_F8     = 0x77,
    Key_F9     = 0x78,
    Key_F10    = 0x79,
    Key_F11    = 0x7A,
    Key_F12    = 0x7B,
    Key_F13    = 0x7C,
    Key_F14    = 0x7D,
    Key_F15    = 0x7E,
    Key_F16    = 0x7F,
    Key_F17    = 0x80,
    Key_F18    = 0x81,
    Key_F19    = 0x82,
    Key_F20    = 0x83,
    Key_F21    = 0x84,
    Key_F22    = 0x85,
    Key_F23    = 0x86,
    Key_F24    = 0x87,

    Key_Count  = 0xFF,
};

typedef struct Digital_Button Digital_Button;
struct Digital_Button {
    b32 down;
    b32 pressed;
    b32 released;
};

typedef struct Analog_Button Analog_Button;
struct Analog_Button {
    f32 value;
    b32 down;
    b32 pressed;
    b32 released;
};

typedef struct Stick Stick;
struct Stick {
    f32 x;
    f32 y;
};

typedef struct Gamepad Gamepad;
struct Gamepad {
    Digital_Button up;
    Digital_Button down;
    Digital_Button left;
    Digital_Button right;
    Digital_Button start;
    Digital_Button back;
    Digital_Button left_thumb;
    Digital_Button right_thumb;
    Digital_Button left_shoulder;
    Digital_Button right_shoulder;
    Digital_Button a;
    Digital_Button b;
    Digital_Button x;
    Digital_Button y;
    Analog_Button left_trigger;
    Analog_Button right_trigger;
    Stick left_stick;
    Stick right_stick;
};

typedef struct Mouse Mouse;
struct Mouse {
    Digital_Button left;
    Digital_Button right;
    Digital_Button middle;
    Digital_Button x1;
    Digital_Button x2;
    s32 wheel;
    s32 delta_wheel;
    s32 x;
    s32 y;
    s32 dx;
    s32 dy;
};

#define Gamepad_Count 4
typedef struct OS_Input OS_Input;
struct OS_Input {
    Digital_Button keyboard[Key_Count];
    Gamepad gamepads[Gamepad_Count];
    Mouse mouse;

    // TODO(xkazu0x):
    b32 executable_reloaded;
};

#define BYTES_PER_PIXEL 4
typedef struct OS_Framebuffer OS_Framebuffer;
struct OS_Framebuffer {
    s32 width;
    s32 height;
    s32 pitch;
    void *memory;
};

typedef struct OS_Memory OS_Memory;
struct OS_Memory {
    b32 initialized;
    
    u64 permanent_storage_size;
    u64 transient_storage_size;
    
    void *permanent_storage; // NOTE(xkazu0x): required to be cleared to zero at startup
    void *transient_storage; // NOTE(xkazu0x): required to be cleared to zero at startup

#if EXCALIBUR_INTERNAL
    Debug_OS_Free_File *debug_os_free_file;
    Debug_OS_Read_File *debug_os_read_file;
    Debug_OS_Write_File *debug_os_write_file;

    Debug_Cycle_Counter counters[DebugCycleCounter_Count];
#endif
};

typedef struct OS_Clock OS_Clock;
struct OS_Clock {
    f32 dt;
};

#define GAME_UPDATE_AND_RENDER(x) void x(OS_Framebuffer *framebuffer, OS_Input *input, OS_Memory *memory, OS_Clock *clock, OS_Thread *thread)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);

#endif // EXCALIBUR_OS_H
