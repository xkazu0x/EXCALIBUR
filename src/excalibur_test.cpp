#include "excalibur.h"
#include "excalibur.cpp"

internal void
output_sound(ex_sound_buffer *sound_buffer, int32 tone_hz) {
    local float32 t_sine;
    int16 tone_volume = 3000;
    int32 wave_period = sound_buffer->samples_per_second / tone_hz;

    int16 *sample_out = sound_buffer->samples;
    for (int32 sample_index = 0;
         sample_index < sound_buffer->sample_count;
         ++sample_index) {
        float32 sine_value = sin_f32(t_sine);
        int16 sample_value = (int16)(sine_value * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
            
        t_sine += 2.0f * pi_f32 * 1.0f / (float32)wave_period;
    }
}

internal void
update_and_render(ex_input *input, ex_sound_buffer *sound_buffer) {
    local int32 tone_hz = 256;
    
    ex_gamepad_input *gamepad = &input->gamepads[0];
    if (gamepad->a.down) tone_hz++;
    if (gamepad->b.down && tone_hz > 100) tone_hz--;
    if (gamepad->left_stick.y > 0) {
        ++tone_hz;
        EXINFO("TONE HZ: %d", tone_hz);
    }
    if (gamepad->left_stick.y < 0 && tone_hz > 1) {
        --tone_hz;
        EXINFO("TONE HZ: %d", tone_hz);
    }
    
    if (input->mouse.left.pressed) EXINFO("LM PRESSED");
    if (input->mouse.left.released) EXINFO("LM RELEASED");
    
    if (input->keyboard[EX_KEY_F1].pressed) EXINFO("F1 PRESSED");
    if (input->keyboard[EX_KEY_F1].released) EXINFO("F1 RELEASED");
    if (input->keyboard[EX_KEY_F2].down) EXINFO("F2 DOWN");

    output_sound(sound_buffer, tone_hz);
}

int main(void) {
    excalibur_desc description = {};
    description.update_and_render = update_and_render;
    if (!excalibur_initialize(&description)) return(1);
    return(0);
}
