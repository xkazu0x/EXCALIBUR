#ifndef EXCALIBUR_H
#define EXCALIBUR_H

#include "base/excalibur_base.h"

// TODO(xkazu0x): platform independent keycodes
enum keyboard_key {
    EX_KEY_ESCAPE = 0x1B,
    
    EX_KEY_F1 = 0x70,
    EX_KEY_F2 = 0x71,
    EX_KEY_F3 = 0x72,
    EX_KEY_F4 = 0x73,
    EX_KEY_F5 = 0x74,
    EX_KEY_F6 = 0x75,
    EX_KEY_F7 = 0x76,
    EX_KEY_F8 = 0x77,
    EX_KEY_F9 = 0x78,
    EX_KEY_F10 = 0x79,
    EX_KEY_F11 = 0x7A,
    EX_KEY_F12 = 0x7B,
    EX_KEY_F13 = 0x7C,
    EX_KEY_F14 = 0x7D,
    EX_KEY_F15 = 0x7E,
    EX_KEY_F16 = 0x7F,
    EX_KEY_F17 = 0x80,
    EX_KEY_F18 = 0x81,
    EX_KEY_F19 = 0x82,
    EX_KEY_F20 = 0x83,
    EX_KEY_F21 = 0x84,
    EX_KEY_F22 = 0x85,
    EX_KEY_F23 = 0x86,
    EX_KEY_F24 = 0x87,
    
    EX_KEY_MAX = 256,
};

struct ex_digital_button {
    bool8 down;
    bool8 pressed;
    bool8 released;
};

struct ex_analog_button {
    float32 threshold;
    float32 value;
    bool8 down;
    bool8 pressed;
    bool8 released;
};

struct ex_stick {
    float threshold;
    float x;
    float y;
};

struct ex_gamepad_input {
    ex_digital_button up;
    ex_digital_button down;
    ex_digital_button left;
    ex_digital_button right;
    ex_digital_button start;
    ex_digital_button back;
    ex_digital_button left_thumb;
    ex_digital_button right_thumb;
    ex_digital_button left_shoulder;
    ex_digital_button right_shoulder;
    ex_digital_button a;
    ex_digital_button b;
    ex_digital_button x;
    ex_digital_button y;
    ex_analog_button left_trigger;
    ex_analog_button right_trigger;
    ex_stick left_stick;
    ex_stick right_stick;
};

struct ex_mouse_input {
    ex_digital_button left;
    ex_digital_button right;
    ex_digital_button middle;
    int32 wheel;
    int32 delta_wheel;
    vec2i position;
    vec2i delta_position;
};

struct ex_input {
    ex_mouse_input mouse;
    ex_gamepad_input gamepads[4];
    ex_digital_button keyboard[EX_KEY_MAX];
};

struct ex_sound_buffer {
    int32 samples_per_second;
    int32 sample_count;
    int16 *samples;
};

struct excalibur_desc {
    void (*update_and_render)(ex_input *, ex_sound_buffer *);
};

internal bool32 excalibur_initialize(excalibur_desc *description);

#endif // EXCALIBUR_H
