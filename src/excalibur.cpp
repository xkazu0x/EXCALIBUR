#include "base/ex_base.h"
#include "base/ex_base.cpp"

#undef function
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define function static
#include <xinput.h>

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

enum mouse_button {
    EX_MOUSE_LEFT,
    EX_MOUSE_RIGHT,
    EX_MOUSE_MIDDLE,
    EX_MOUSE_MAX_BUTTON,
};

enum gamepad_button {
    EX_GAMEPAD_UP,
    EX_GAMEPAD_DOWN,
    EX_GAMEPAD_LEFT,
    EX_GAMEPAD_RIGHT,
    EX_GAMEPAD_START,
    EX_GAMEPAD_BACK,
    EX_GAMEPAD_LEFT_THUMB,
    EX_GAMEPAD_RIGHT_THUMB,
    EX_GAMEPAD_LEFT_SHOULDER,
    EX_GAMEPAD_RIGHT_SHOULDER,
    EX_GAMEPAD_A,
    EX_GAMEPAD_B,
    EX_GAMEPAD_X,
    EX_GAMEPAD_Y,
    EX_GAMEPAD_MAX_BUTTON,
};

#define X_INPUT_GET_STATE(name) unsigned long __stdcall name(unsigned long dwUseIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) unsigned long __stdcall name(unsigned long dwUseIndex, XINPUT_VIBRATION *pVibration)

typedef X_INPUT_GET_STATE(XINPUTGETSTATE);
typedef X_INPUT_SET_STATE(XINPUTSETSTATE);

X_INPUT_GET_STATE(_xinput_get_state) { return(ERROR_DEVICE_NOT_CONNECTED); }
X_INPUT_SET_STATE(_xinput_set_state) { return(ERROR_DEVICE_NOT_CONNECTED); }

struct win32_window_context {
    ATOM atom;
    HWND handle;
    HINSTANCE instance;
    WINDOWPLACEMENT placement;
};

struct win32_input_context {
    XINPUTGETSTATE *xinput_get_state;
    XINPUTSETSTATE *xinput_set_state;
};

struct digital_button {
    bool8 down;
    bool8 pressed;
    bool8 released;
};

struct stick {
    float threshold;
    float x;
    float y;
};

struct analog_button {
    float32 threshold;
    float32 value;
    bool8 down;
    bool8 pressed;
    bool8 released;
};

struct mouse_context {
    union {
        digital_button buttons[EX_MOUSE_MAX_BUTTON];
        struct {
            digital_button left;
            digital_button right;
            digital_button middle;
        };
    };
    int32 wheel;
    int32 delta_wheel;
    vec2i position;
    vec2i delta_position;
};

struct gamepad_context {
    union {
        digital_button buttons[EX_GAMEPAD_MAX_BUTTON];
        struct {
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
        };
    };
    analog_button left_trigger;
    analog_button right_trigger;
    stick left_stick;
    stick right_stick;
};

#define EX_GAMEPAD_MAX_COUNT 4

struct input_context {
    mouse_context mouse;
    digital_button keyboard[EX_KEY_MAX];
    gamepad_context gamepads[EX_GAMEPAD_MAX_COUNT];
};

global win32_window_context win32_window;
global win32_input_context win32_input;

global input_context input;
global bool8 quit;

void
toggle_fullscreen() {
    DWORD window_style = GetWindowLong(win32_window.handle, GWL_STYLE);
    if (window_style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO monitor_info = {};
        monitor_info.cbSize = sizeof(monitor_info);
        if (GetWindowPlacement(win32_window.handle, &win32_window.placement) &&
            GetMonitorInfo(MonitorFromWindow(win32_window.handle, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) {
            SetWindowLong(win32_window.handle, GWL_STYLE, (window_style & ~(WS_OVERLAPPEDWINDOW)) | WS_POPUP);
            SetWindowPos(win32_window.handle, HWND_TOP,
                         monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLongPtr(win32_window.handle, GWL_STYLE, (window_style & ~(WS_POPUP)) | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(win32_window.handle, &win32_window.placement);
        SetWindowPos(win32_window.handle, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void
process_digital_button(digital_button *button, bool8 down) {
    bool8 was_down = button->down;
    button->pressed = !was_down && down;
    button->released = was_down && !down;
    button->down = down;
}

void
process_analog_button(analog_button *button, float32 value) {
    button->value = value;
    bool8 was_down = button->down;
    button->down = (value >= button->threshold);
    button->pressed = !was_down && button->down;
    button->released = was_down && !button->down;
}

void
process_stick(stick *stick, float32 x, float32 y) {
    if (abs_f32(x) <= stick->threshold) x = 0.0f;
    if (abs_f32(y) <= stick->threshold) y = 0.0f;
    stick->x = x;
    stick->y = y;
}

LRESULT CALLBACK
win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    switch (message) {
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        }
    }
    return(result);
}

int main(void) {
    EXINFO("-+=+EXCALIBUR : CONTEXT+=+-");
    EXINFO("\t> OPERATING SYSTEM: %s", string_from_operating_system(operating_system_from_context()));
    EXINFO("\t> OPERATING SYSTEM: %s", string_from_architecture(architecture_from_context()));

    // NOTE(xkazu0x): window configuration
    const char *window_title = "EXCALIBUR";
    int32 window_width = 960;
    int32 window_height = 540;
    int32 window_x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;
    int32 window_y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;

    // NOTE(xkazu0x): window initialize
    RECT window_rectangle = {};
    window_rectangle.left = 0;
    window_rectangle.right = window_width;
    window_rectangle.top = 0;
    window_rectangle.bottom = window_height;
    if (AdjustWindowRect(&window_rectangle, WS_OVERLAPPEDWINDOW, 0)) {
        window_width = window_rectangle.right - window_rectangle.left;
        window_height = window_rectangle.bottom - window_rectangle.top;
    }

    win32_window.instance = GetModuleHandleA(nullptr);
    
    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = win32_window.instance;
    window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = "EXCALIBUR_WINDOW_CLASS";
    win32_window.atom = RegisterClassA(&window_class);
    if (!win32_window.atom) {
        EXFATAL("Failed to register win32 window.");
        return(1);
    }
    
    win32_window.handle = CreateWindow(MAKEINTATOM(win32_window.atom),
                                window_title,
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                window_x, window_y,
                                window_width, window_height,
                                0, 0, win32_window.instance, 0);
    if (!win32_window.handle) {
        EXFATAL("Failed to create win32 window.");
        return(1);
    }
    
    // NOTE(xkazu0x): mouse initialize
    RAWINPUTDEVICE raw_input_device = {};
    raw_input_device.usUsagePage = 0x01;
    raw_input_device.usUsage = 0x02;
    raw_input_device.hwndTarget = win32_window.handle;
    if (!RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device))) {
        EXERROR("Failed to register mouse as raw input device.");
        return(1);
    }
    
    // NOTE(xkazu0x): gamepad initialize
    win32_input.xinput_get_state = _xinput_get_state;
    win32_input.xinput_set_state = _xinput_set_state;
    
    HMODULE xinput_module = LoadLibraryA("xinput1_4.dll");
    if (!xinput_module) xinput_module = LoadLibraryA("xinput1_3.dll");
    if (xinput_module) {
        win32_input.xinput_get_state = (XINPUTGETSTATE *)GetProcAddress(xinput_module, "XInputGetState");
        win32_input.xinput_set_state = (XINPUTSETSTATE *)GetProcAddress(xinput_module, "XInputSetState");
    }
    
    float32 trigger_threshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;
    for (uint32 i = 0; i < EX_GAMEPAD_MAX_COUNT; i++) {
        input.gamepads[i].left_trigger.threshold = trigger_threshold;
        input.gamepads[i].right_trigger.threshold = trigger_threshold;
        input.gamepads[i].left_stick.threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
        input.gamepads[i].right_stick.threshold = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    }
    
    while (!quit) {
        // NOTE(xkazu0x): mouse reset
        input.mouse.left.pressed = EX_FALSE;
        input.mouse.left.released = EX_FALSE;
        input.mouse.right.pressed = EX_FALSE;
        input.mouse.right.released = EX_FALSE;
        input.mouse.middle.pressed = EX_FALSE;
        input.mouse.middle.released = EX_FALSE;
        input.mouse.delta_position.x = 0;
        input.mouse.delta_position.y = 0;
        input.mouse.delta_wheel = 0;

        // NOTE(xkazu0x): window update
        MSG msg = {};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
                case WM_INPUT: {
                    UINT size;
                    GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER));
                    char *buffer = (char *)_alloca(size);
                    if (GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size) {
                        RAWINPUT *raw_input = (RAWINPUT *)buffer;
                        if (raw_input->header.dwType == RIM_TYPEMOUSE && raw_input->data.mouse.usFlags == MOUSE_MOVE_RELATIVE){
                            input.mouse.delta_position.x += raw_input->data.mouse.lLastX;
                            input.mouse.delta_position.y += raw_input->data.mouse.lLastY;

                            USHORT button_flags = raw_input->data.mouse.usButtonFlags;
                
                            bool8 left_button_down = input.mouse.left.down;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_DOWN) left_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_LEFT_BUTTON_UP) left_button_down = EX_FALSE;
                            process_digital_button(&input.mouse.left, left_button_down);

                            bool8 right_button_down = input.mouse.right.down;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_down = EX_FALSE;
                            process_digital_button(&input.mouse.right, right_button_down);
                            
                            bool8 middle_button_down = input.mouse.middle.down;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) middle_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_UP) middle_button_down = EX_FALSE;
                            process_digital_button(&input.mouse.middle, middle_button_down);

                            if (raw_input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                                input.mouse.delta_wheel += ((SHORT)raw_input->data.mouse.usButtonData) / WHEEL_DELTA;
                            }
                        }
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                } break;
                case WM_QUIT: {
                    UnregisterClassA(MAKEINTATOM(win32_window.atom), win32_window.instance);
                    quit = EX_TRUE;
                } break;
                default: {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        RECT client_rectangle;
        GetClientRect(win32_window.handle, &client_rectangle);
        window_width = client_rectangle.right - client_rectangle.left;
        window_height = client_rectangle.bottom - client_rectangle.top;

        POINT window_position = { client_rectangle.left, client_rectangle.top };
        ClientToScreen(win32_window.handle, &window_position);
        window_x = window_position.x;
        window_y = window_position.y;
        
        // NOTE(xkazu0x): keyboard update
        BYTE keyboard_state[EX_KEY_MAX];
        if (GetKeyboardState(keyboard_state)) {
            for (uint32 key = 0; key < EX_KEY_MAX; key++) {
                process_digital_button(input.keyboard + key, (keyboard_state[key] >> 7));
            }
        }
        if (input.keyboard[EX_KEY_ESCAPE].pressed) DestroyWindow(win32_window.handle);
        if (input.keyboard[EX_KEY_F11].pressed) toggle_fullscreen();
        
        // NOTE(xkazu0x): mouse update
        input.mouse.position.x += input.mouse.delta_position.x;
        input.mouse.position.y += input.mouse.delta_position.y;
        input.mouse.wheel += input.mouse.delta_wheel;

        POINT mouse_position;
        GetCursorPos(&mouse_position);
        ScreenToClient(win32_window.handle, &mouse_position);
        if (mouse_position.x < 0) mouse_position.x = 0;
        if (mouse_position.y < 0) mouse_position.y = 0;
        if (mouse_position.x > window_width) mouse_position.x = window_width;
        if (mouse_position.y > window_height) mouse_position.y = window_height;
        input.mouse.position.x = mouse_position.x;
        input.mouse.position.y = mouse_position.y;

        if (input.mouse.buttons[EX_MOUSE_LEFT].pressed) EXDEBUG("MOUSE LEFT PRESSED");
        if (input.mouse.buttons[EX_MOUSE_LEFT].released) EXDEBUG("MOUSE LEFT RELEASED");
        if (input.mouse.buttons[EX_MOUSE_RIGHT].pressed) EXDEBUG("MOUSE RIGHT PRESSED");
        if (input.mouse.buttons[EX_MOUSE_RIGHT].released) EXDEBUG("MOUSE RIGHT RELEASED");
        if (input.mouse.buttons[EX_MOUSE_MIDDLE].pressed) EXDEBUG("MOUSE MIDDLE PRESSED");
        if (input.mouse.buttons[EX_MOUSE_MIDDLE].released) EXDEBUG("MOUSE MIDDLE RELEASED");
        if (input.mouse.delta_wheel) EXDEBUG("WHEEL VALUE: %d", input.mouse.wheel);
        if (input.mouse.delta_position.x || input.mouse.delta_position.y)
            EXDEBUG("MOUSE POSITION: x: %d, y: %d", input.mouse.position.x, input.mouse.position.y);
                        
        // NOTE(xkazu0x): gamepad update
        uint32_t max_gamepad_count = XUSER_MAX_COUNT;
        if (max_gamepad_count > EX_GAMEPAD_MAX_COUNT) {
            max_gamepad_count = EX_GAMEPAD_MAX_COUNT;
        }
        for (uint32 i = 0; i < max_gamepad_count; i++) {
            XINPUT_STATE xinput_state;
            if (win32_input.xinput_get_state(i, &xinput_state) == ERROR_SUCCESS) {
                XINPUT_GAMEPAD *xgamepad = &xinput_state.Gamepad;
                gamepad_context *gamepad = &input.gamepads[i];
                process_digital_button(&gamepad->up,    (xgamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
                process_digital_button(&gamepad->down,  (xgamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
                process_digital_button(&gamepad->left,  (xgamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
                process_digital_button(&gamepad->right, (xgamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
                process_digital_button(&gamepad->start, (xgamepad->wButtons & XINPUT_GAMEPAD_START) != 0);
                process_digital_button(&gamepad->back,  (xgamepad->wButtons & XINPUT_GAMEPAD_BACK) != 0);
                process_digital_button(&gamepad->left_thumb,     (xgamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
                process_digital_button(&gamepad->right_thumb,    (xgamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
                process_digital_button(&gamepad->left_shoulder,  (xgamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
                process_digital_button(&gamepad->right_shoulder, (xgamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
                process_digital_button(&gamepad->a, (xgamepad->wButtons & XINPUT_GAMEPAD_A) != 0);
                process_digital_button(&gamepad->b, (xgamepad->wButtons & XINPUT_GAMEPAD_B) != 0);
                process_digital_button(&gamepad->x, (xgamepad->wButtons & XINPUT_GAMEPAD_X) != 0);
                process_digital_button(&gamepad->y, (xgamepad->wButtons & XINPUT_GAMEPAD_Y) != 0);
                process_analog_button(&gamepad->left_trigger, xgamepad->bLeftTrigger / 255.0f);
                process_analog_button(&gamepad->right_trigger, xgamepad->bRightTrigger / 255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
                process_stick(&gamepad->left_stick, CONVERT(xgamepad->sThumbLX), CONVERT(xgamepad->sThumbLY));
                process_stick(&gamepad->right_stick, CONVERT(xgamepad->sThumbRX), CONVERT(xgamepad->sThumbRY));
#undef CONVERT
            }
        }
        
        if (input.gamepads[0].up.pressed) EXDEBUG("UP");
        if (input.gamepads[0].down.pressed) EXDEBUG("DOWN");
        if (input.gamepads[0].left.pressed) EXDEBUG("LEFT");
        if (input.gamepads[0].right.pressed) EXDEBUG("RIGHT");
        if (input.gamepads[0].start.pressed) EXDEBUG("START");
        if (input.gamepads[0].back.pressed) EXDEBUG("BACK");
        if (input.gamepads[0].left_thumb.pressed) EXDEBUG("LEFT THUMB");
        if (input.gamepads[0].right_thumb.pressed) EXDEBUG("RIGHT THUMB");
        if (input.gamepads[0].left_shoulder.pressed) EXDEBUG("LEFT BUMPER");
        if (input.gamepads[0].right_shoulder.pressed) EXDEBUG("RIGHT BUMPER");
        if (input.gamepads[0].a.pressed) EXDEBUG("A");
        if (input.gamepads[0].b.pressed) EXDEBUG("B");
        if (input.gamepads[0].x.pressed) EXDEBUG("X");
        if (input.gamepads[0].y.pressed) EXDEBUG("Y");
        if (input.gamepads[0].left_trigger.pressed) EXDEBUG("LEFT TRIGGER PRESSED");
        if (input.gamepads[0].left_trigger.value) EXDEBUG("LEFT TRIGGER VALUE: %f", input.gamepads[0].left_trigger.value);
        if (input.gamepads[0].left_trigger.released) EXDEBUG("LEFT TRIGGER RELEASED");
        if (input.gamepads[0].right_trigger.pressed) EXDEBUG("RIGHT TRIGGER PRESSED");
        if (input.gamepads[0].right_trigger.value) EXDEBUG("RIGHT TRIGGER VALUE: %f", input.gamepads[0].right_trigger.value);
        if (input.gamepads[0].right_trigger.released) EXDEBUG("RIGHT TRIGGER RELEASED");
        if (input.gamepads[0].left_stick.x || input.gamepads[0].left_stick.y) {
            EXDEBUG("LEFT STICK: x: %f, y: %f", input.gamepads[0].left_stick.x, input.gamepads[0].left_stick.y);
        }
        if (input.gamepads[0].right_stick.x || input.gamepads[0].right_stick.y) {
            EXDEBUG("RIGHT STICK: x: %f, y: %f", input.gamepads[0].right_stick.x, input.gamepads[0].right_stick.y);
        }
    }
    return(0);
}
