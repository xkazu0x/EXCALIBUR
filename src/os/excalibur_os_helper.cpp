internal void
os_process_digital_button(Digital_Button *button, b32 down) {
    b32 was_down = button->down;
    button->pressed = !was_down && down;
    button->released = was_down && !down;
    button->down = down;
}

internal void
os_process_analog_button(Analog_Button *button, f32 threshold, f32 value) {
    button->value = value;
    b32 was_down = button->down;
    button->down = (value >= threshold);
    button->pressed = !was_down && button->down;
    button->released = was_down && !button->down;
}

internal void
os_process_stick(Stick *stick, f32 threshold, f32 x, f32 y) {
    if (abs_f32(x) <= threshold) x = 0.0f;
    if (abs_f32(y) <= threshold) y = 0.0f;
    stick->x = x;
    stick->y = y;
}
