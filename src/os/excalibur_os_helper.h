#ifndef EXCALIBUR_OS_HELPER_H
#define EXCALIBUR_OS_HELPER_H

internal void os_process_digital_button(Digital_Button *button, b32 down);
internal void os_process_analog_button(Analog_Button *button, f32 threshold, f32 value);
internal void os_process_stick(Stick *stick, f32 threshold, f32 x, f32 y);

#endif // EXCALIBUR_OS_HELPER_H
