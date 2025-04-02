#ifndef EXCALIBUR_PLATFORM_HELPER_H
#define EXCALIBUR_PLATFORM_HELPER_H

internal void platform_process_digital_button(digital_button_t *button, b32 down);
internal void platform_process_analog_button(analog_button_t *button, f32 value);
internal void platform_process_stick(stick_t *stick, f32 x, f32 y);

#endif // EXCALIBUR_PLATFORM_HELPER_H
