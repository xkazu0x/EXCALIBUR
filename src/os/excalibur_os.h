#ifndef EXCALIBUR_OS_H
#define EXCALIBUR_OS_H

////////////////////////////////
// NOTE(xkazu0x): services that the os layer provides to the game

#if EXCALIBUR_INTERNAL
// IMPORTANT(xkazu0x): These are NOT for doing anything in the shipping game
// they are blocking and the write doesn't protect against lost data

typedef struct Debug_OS_File Debug_OS_File;
struct Debug_OS_File {
    u32 size;
    void *data;
};

# define DEBUG_OS_FREE_FILE(x)  void x(void *memory)
# define DEBUG_OS_READ_FILE(x)  Debug_OS_File x(char *filename)
# define DEBUG_OS_WRITE_FILE(x) b32 x(char *filename, u32 size, void *memory)

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

// TODO(xkazu0x): make a intrinsic file
// {
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

#if COMPILER_MSVC
# define complete_previous_write_before_future_write _WriteBarrier()
inline u32 atomic_compare_exchange_u32(u32 volatile *test_value, u32 expected_value, u32 new_value) {
    u32 result = _InterlockedCompareExchange((long *)test_value, expected_value, new_value);
    return(result);
}
#else
// TODO(xkazu0x): need GCC/LLVM equivalents!
# define complete_previous_write_before_future_write
#endif
// }

////////////////////////////////
// NOTE(xkazu0x): services that the game provides to the os layer
    
typedef u32 Key_Code;
enum {
    Key_Null,
    
    Key_Escape,
    Key_Enter,
    Key_Shift,
    Key_Space,
    
    Key_Up,
    Key_Down,
    Key_Left,
    Key_Right,
    
    Key_0,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    
    Key_A,
    Key_B,
    Key_C,
    Key_D,
    Key_E,
    Key_F,
    Key_G,
    Key_H,
    Key_I,
    Key_J,
    Key_K,
    Key_L,
    Key_M,
    Key_N,
    Key_O,
    Key_P,
    Key_Q,
    Key_R,
    Key_S,
    Key_T,
    Key_U,
    Key_V,
    Key_W,
    Key_X,
    Key_Y,
    Key_Z,
    
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    Key_F13,
    Key_F14,
    Key_F15,
    Key_F16,
    Key_F17,
    Key_F18,
    Key_F19,
    Key_F20,
    Key_F21,
    Key_F22,
    Key_F23,
    Key_F24,

    Key_Count,
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

typedef struct OS_Work_Queue OS_Work_Queue;

#define OS_WORK_QUEUE_CALLBACK(x) void x(OS_Work_Queue *queue, void *data)
typedef OS_WORK_QUEUE_CALLBACK(OS_Work_Queue_Callback);

typedef void OS_Work_Queue_Add_Entry(OS_Work_Queue *queue, OS_Work_Queue_Callback *callback, void *data);
typedef void OS_Work_Queue_Complete(OS_Work_Queue *queue);

typedef struct OS_Memory OS_Memory;
struct OS_Memory {
    u64 permanent_storage_size;
    u64 transient_storage_size;

    // NOTE(xkazu0x): required to be cleared to zero at startup
    void *permanent_storage;
    void *transient_storage;

    OS_Work_Queue *high_priority_queue;
    OS_Work_Queue *low_priority_queue;
    
    OS_Work_Queue_Add_Entry *os_work_queue_add_entry;
    OS_Work_Queue_Complete *os_work_queue_complete;
    
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

typedef struct OS_Sound_Buffer OS_Sound_Buffer;
struct OS_Sound_Buffer {
    s32 samples_per_second;
    s32 sample_count;
    s16 *samples;
};

#define GAME_UPDATE_AND_RENDER(x) void x(OS_Framebuffer *framebuffer, OS_Input *input, OS_Memory *memory, OS_Clock *clock, OS_Sound_Buffer *sound_buffer)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);

#endif // EXCALIBUR_OS_H
