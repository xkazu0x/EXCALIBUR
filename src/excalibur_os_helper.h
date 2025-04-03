#ifndef EXCALIBUR_OS_HELPER_H
#define EXCALIBUR_OS_HELPER_H

internal void os_process_digital_button(digital_button_t *button, b32 down);
internal void os_process_analog_button(analog_button_t *button, f32 threshold, f32 value);
internal void os_process_stick(stick_t *stick, f32 threshold, f32 x, f32 y);

#endif // EXCALIBUR_OS_HELPER_H
