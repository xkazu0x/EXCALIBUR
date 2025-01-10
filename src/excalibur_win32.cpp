#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#include <mmsystem.h>
#include <dsound.h>
#include <xaudio2.h>

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD /*dwUseIndex*/, XINPUT_STATE */*pState*/)
typedef X_INPUT_GET_STATE(XINPUTGETSTATE);

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD /*dwUseIndex*/, XINPUT_VIBRATION */*pVibration*/)
typedef X_INPUT_SET_STATE(XINPUTSETSTATE);

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID /*pcGuidDevice*/, LPDIRECTSOUND */*ppDS*/, LPUNKNOWN /*pUnkOuter*/)
typedef DIRECT_SOUND_CREATE(DIRECTSOUNDCREATE);

#define XAUDIO2_CREATE(name) HRESULT WINAPI name(IXAudio2 **/*ppXAudio2*/, UINT32 /*Flags*/, XAUDIO2_PROCESSOR /*XAudio2Processor*/)
typedef XAUDIO2_CREATE(XAUDIO2CREATE);

struct win32_sound_output {
    int32 samples_per_second;
    int32 bytes_per_sample;
    int32 buffer_size;
    int32 latency_sample_count;
    uint32 running_sample_index;
};

global bool8 global_quit;

global WINDOWPLACEMENT global_window_placement;
global LPDIRECTSOUNDBUFFER global_sound_buffer;
global IXAudio2 *global_audio;
global IXAudio2MasteringVoice *global_audio_mastering_voice;
global IXAudio2SourceVoice *global_audio_source_voice;

X_INPUT_GET_STATE(_xinput_get_state) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
X_INPUT_SET_STATE(_xinput_set_state) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}

internal XINPUTGETSTATE *xinput_get_state = _xinput_get_state;
internal XINPUTSETSTATE *xinput_set_state = _xinput_set_state;

internal void
win32_process_digital_button(ex_digital_button *button, bool8 down) {
    bool8 was_down = button->down;
    button->pressed = !was_down && down;
    button->released = was_down && !down;
    button->down = down;
}

internal void
win32_process_analog_button(ex_analog_button *button, float32 value) {
    button->value = value;
    bool8 was_down = button->down;
    button->down = (value >= button->threshold);
    button->pressed = !was_down && button->down;
    button->released = was_down && !button->down;
}

internal void
win32_process_stick(ex_stick *stick, float32 x, float32 y) {
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

internal void
win32_window_toggle_fullscreen(HWND window, WINDOWPLACEMENT *placement) {
    DWORD window_style = GetWindowLong(window, GWL_STYLE);
    if (window_style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO monitor_info = {};
        monitor_info.cbSize = sizeof(monitor_info);
        if (GetWindowPlacement(window, placement) &&
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) {
            SetWindowLong(window, GWL_STYLE, (window_style & ~(WS_OVERLAPPEDWINDOW)) | WS_POPUP);
            SetWindowPos(window, HWND_TOP,
                         monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLongPtr(window, GWL_STYLE, (window_style & ~(WS_POPUP)) | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, placement);
        SetWindowPos(window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal void
win32_direct_sound_initialize(HWND window, int32 samples_per_second, int32 buffer_size) {
    HMODULE direct_sound_library = LoadLibraryA("dsound.dll");
    if (!direct_sound_library) {
        EXERROR("< Failed to load 'dsound.dll'.");
        return;
    }
    EXDEBUG("\t> Successfully loaded 'dsound.dll'.");
    
    DIRECTSOUNDCREATE *direct_sound_create = (DIRECTSOUNDCREATE *)
        GetProcAddress(direct_sound_library, "DirectSoundCreate");
    if (!direct_sound_create) {
        EXERROR("< Failed to load internal \"DirectSoundCreate\"");
        return;
    }
        
    IDirectSound *direct_sound;
    HRESULT result = direct_sound_create(0, &direct_sound, 0);
    if (FAILED(result)) {
        EXERROR("< Failed to create direct sound object.");
        return;
    }
    EXDEBUG("\t> Direct sound object created.");
    direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY);
    
    // NOTE(xkazu0x): create a primary buffer
    DSBUFFERDESC buffer_description = {};
    buffer_description.dwSize = sizeof(buffer_description);
    buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;

    IDirectSoundBuffer *primary_buffer;
    result = direct_sound->CreateSoundBuffer(&buffer_description, &primary_buffer, 0);
    if (FAILED(result)) {
        EXERROR("< Failed to create primary buffer.");
        return;
    }
    EXDEBUG("\t> Primary buffer created.");

    WAVEFORMATEX wave_format = {};
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 2;
    wave_format.wBitsPerSample = 16;
    wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
    wave_format.nSamplesPerSec = samples_per_second;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
    wave_format.cbSize = 0;
    result = primary_buffer->SetFormat(&wave_format);
    if (FAILED(result)) {
        EXERROR("< Failed to set primary buffer format.");
        return;
    }
    EXDEBUG("\t> Primary buffer format was set.");
            
    // NOTE(xkazu0x): create a secondary buffer
    DSBUFFERDESC buffer_description1 = {};
    buffer_description1.dwSize = sizeof(buffer_description1);
    buffer_description1.dwFlags = 0;
    buffer_description1.dwBufferBytes = buffer_size;
    buffer_description1.lpwfxFormat = &wave_format;

    result = direct_sound->CreateSoundBuffer(&buffer_description1, &global_sound_buffer, 0);
    if (FAILED(result)) {
        EXERROR("< Failed to create secondary buffer.");
        return;
    }
    EXDEBUG("\t> Secondary buffer created.");
}

internal void
win32_clear_direct_sound_buffer(win32_sound_output *sound_output) {
    VOID *region1;
    DWORD region1_size;
    VOID *region2;
    DWORD region2_size;
    if (SUCCEEDED(global_sound_buffer->Lock(0, sound_output->buffer_size,
                                            &region1, &region1_size,
                                            &region2, &region2_size,
                                            0))) {
        uint8 *dest_sample = (uint8 *)region1;
        for (DWORD byte_index = 0;
             byte_index < region1_size;
             ++byte_index) {
            *dest_sample++ = 0;
        }
        
        dest_sample = (uint8 *)region2;
        for (DWORD byte_index = 0;
             byte_index < region2_size;
             ++byte_index) {
            *dest_sample++ = 0;
        }
        
        global_sound_buffer->Unlock(region1, region1_size, region2, region2_size);        
    }
}

internal void
win32_fill_direct_sound_buffer(win32_sound_output *sound_output,
                               DWORD byte_to_lock,
                               DWORD bytes_to_write,
                               ex_sound_buffer *source_buffer) {
    VOID *region1;
    DWORD region1_size;
    VOID *region2;
    DWORD region2_size;
    if (SUCCEEDED(global_sound_buffer->Lock(byte_to_lock, bytes_to_write,
                                            &region1, &region1_size,
                                            &region2, &region2_size,
                                            0))) {
        DWORD region1_sample_count = region1_size / sound_output->bytes_per_sample;
        int16 *dest_sample = (int16 *)region1;
        int16 *source_sample = source_buffer->samples;

        for (DWORD sample_index = 0;
             sample_index < region1_sample_count;
             ++sample_index) {
            *dest_sample++ = *source_sample++;
            *dest_sample++ = *source_sample++;
            ++sound_output->running_sample_index;
        }

        DWORD region2_sample_count = region2_size / sound_output->bytes_per_sample;
        dest_sample = (int16 *)region2;

        for (DWORD sample_index = 0;
             sample_index < region2_sample_count;
             ++sample_index) {
            *dest_sample++ = *source_sample++;
            *dest_sample++ = *source_sample++;
            ++sound_output->running_sample_index;
        }

        global_sound_buffer->Unlock(region1, region1_size, region2, region2_size);
    }
}

internal bool8
win32_xaudio2_initialize(void) {
    HMODULE xaudio2_library = LoadLibraryA("xaudio2_9.dll");
    if (!xaudio2_library) {
        EXERROR("\t< Failed to load 'xaudio2_9.dll'.");
        return(EX_FALSE);
    } else {
        EXINFO("\t> Successfully loaded 'xaudio2_9.dll'.");
    }

    XAUDIO2CREATE *xaudio2_create = (XAUDIO2CREATE *)
        GetProcAddress(xaudio2_library, "XAudio2Create");
    if (!xaudio2_create) {
        EXERROR("\t< Failed to load function \"XAudio2Create\"");
        return(EX_FALSE);
    }

    HRESULT result = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(result)) {
        EXERROR("\t< Failed to initialize COM");
        return(EX_FALSE);
    }
    
    result = xaudio2_create(&global_audio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(result)) {
        EXERROR("\t< Failed to create XAudio2.");
        return(EX_FALSE);
    }
    EXINFO("\t> XAudio2 created.");

    result = global_audio->CreateMasteringVoice(&global_audio_mastering_voice);
    if (FAILED(result)) {
        EXERROR("\t< Failed to create XAudio2 mastering voice.");
        return(EX_FALSE);
    }
    EXINFO("\t> XAudio2 Mastering Voice created.");

    WAVEFORMATEX wave_format = {};
    wave_format.wFormatTag = WAVE_FORMAT_PCM;
    wave_format.nChannels = 2;
    wave_format.nSamplesPerSec = 44100;
    wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nChannels * 2;
    wave_format.nBlockAlign = wave_format.nChannels * 2;
    wave_format.wBitsPerSample = 16;
    wave_format.cbSize = 0;

    result = global_audio->CreateSourceVoice(&global_audio_source_voice, &wave_format);
    if (FAILED(result)) {
        EXERROR("\t< Failed to create XAudio2 source voice.");
        return(EX_FALSE);
    }
    EXINFO("\t> XAudio2 source voice created.");
    global_audio_source_voice->SetVolume(0.5f, XAUDIO2_COMMIT_NOW);

    return(EX_TRUE);
}

internal bool32
excalibur_initialize(excalibur_desc *description) {
    EXINFO("-+=+EXCALIBUR : CONTEXT+=+-");
    EXINFO("\t> Operating system: %s", string_from_operating_system(operating_system_from_context()));
    EXINFO("\t> Architecture: %s", string_from_architecture(architecture_from_context()));

    // NOTE(xkazu0x): window configuration
    const char *window_title = "EXCALIBUR";
    int32 window_width = 960;
    int32 window_height = 540;
    int32 window_x = (GetSystemMetrics(SM_CXSCREEN) - window_width) / 2;
    int32 window_y = (GetSystemMetrics(SM_CYSCREEN) - window_height) / 2;

    // NOTE(xkazu0x): window initialize
    EXINFO("-+=+EXCALIBUR : INITIALIZE+=+-");
    EXINFO("> Window initialize...");
    RECT window_rectangle = {};
    window_rectangle.left = 0;
    window_rectangle.right = window_width;
    window_rectangle.top = 0;
    window_rectangle.bottom = window_height;
    if (AdjustWindowRect(&window_rectangle, WS_OVERLAPPEDWINDOW, 0)) {
        window_width = window_rectangle.right - window_rectangle.left;
        window_height = window_rectangle.bottom - window_rectangle.top;
    }

    HINSTANCE window_instance = GetModuleHandleA(nullptr);
    
    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = window_instance;
    window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = 0;
    window_class.lpszMenuName = 0;
    window_class.lpszClassName = "EXCALIBUR_WINDOW_CLASS";
    ATOM window_atom = RegisterClassA(&window_class);
    if (!window_atom) {
        EXFATAL("\t< Failed to register win32 window.");
        return(EX_FALSE);
    }
    EXDEBUG("\t> Win32 window registered.");
    
    HWND window_handle = CreateWindow(MAKEINTATOM(window_atom),
                                      window_title,
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      window_x, window_y,
                                      window_width, window_height,
                                      0, 0, window_instance, 0);
    if (!window_handle) {
        EXFATAL("\t< Failed to create win32 window.");
        return(EX_FALSE);
    }
    EXDEBUG("\t> Win32 window created.");
    
    // NOTE(xkazu0x): mouse initialize
    EXINFO("> Mouse initialize...");
    RAWINPUTDEVICE raw_input_device = {};
    raw_input_device.usUsagePage = 0x01;
    raw_input_device.usUsage = 0x02;
    raw_input_device.hwndTarget = window_handle;
    if (!RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device))) {
        EXERROR("\t< Failed to register mouse as raw input device.");
        return(EX_FALSE);
    }
    EXDEBUG("\t> Mouse registered as raw input device.");
    
    // NOTE(xkazu0x): gamepad initialize
    EXINFO("> Gamepad initialize...");
    HMODULE xinput_library = LoadLibraryA("xinput1_4.dll");
    if (!xinput_library) {
        EXWARN("\t< Failed to load 'xinput1_4.dll'.");
        xinput_library = LoadLibraryA("xinput1_3.dll");
        if (!xinput_library) {
            EXWARN("\t< Failed to load 'xinput1_3.dll'.");
            xinput_library = LoadLibraryA("xinput9_1_0.dll");
            if (!xinput_library) {
                EXWARN("\t< Failed to load 'xinput9_1_0.dll'.");
            } else {
                EXDEBUG("\t> Successfully loaded 'xinput9_1_0.dll'.");
            }
        } else {
            EXDEBUG("\t> Successfully loaded 'xinput1_3.dll'.");
        }
    } else {
        EXDEBUG("\t> Successfully loaded 'xinput1_4.dll'.");
    }
    
    if (xinput_library) {
        xinput_get_state = (XINPUTGETSTATE *)GetProcAddress(xinput_library, "XInputGetState");
        if (!xinput_get_state) xinput_get_state = _xinput_get_state;
        xinput_set_state = (XINPUTSETSTATE *)GetProcAddress(xinput_library, "XInputSetState");
        if (!xinput_set_state) xinput_set_state = _xinput_set_state;
    }

    ex_input input = {};
    float32 trigger_threshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;
    for (uint32 i = 0; i < EX_ARRAY_COUNT(input.gamepads); i++) {
        input.gamepads[i].left_trigger.threshold = trigger_threshold;
        input.gamepads[i].right_trigger.threshold = trigger_threshold;
        input.gamepads[i].left_stick.threshold = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
        input.gamepads[i].right_stick.threshold = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / 32767.0f;
    }
    
    // NOTE(xkazu0x): sound initialize
    EXINFO("> Sound initialize...");
    win32_sound_output sound_output = {};
    sound_output.samples_per_second = 48000;
    sound_output.bytes_per_sample = sizeof(int16) * 2;
    sound_output.buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;    
    sound_output.latency_sample_count = sound_output.samples_per_second / 15;
    
    win32_direct_sound_initialize(window_handle, sound_output.samples_per_second, sound_output.buffer_size);
    win32_clear_direct_sound_buffer(&sound_output);
    global_sound_buffer->Play(0, 0, DSBPLAY_LOOPING);

    int16 *samples = (int16 *)VirtualAlloc(0, sound_output.buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
    // NOTE(xkazu0x): time initialize
    LARGE_INTEGER large_integer;
    QueryPerformanceFrequency(&large_integer);
    int64 counts_per_second = large_integer.QuadPart;
    
    LARGE_INTEGER last_counter;
    QueryPerformanceCounter(&last_counter);

    int64 last_cycle_count = __rdtsc();
    
    while (!global_quit) {
        // NOTE(xkazu0x): keyboard reset
        for (uint32 i = 0; i < EX_KEY_MAX; i++) {
            input.keyboard[i].pressed = EX_FALSE;
            input.keyboard[i].released = EX_FALSE;
        }
        
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
        MSG msg;
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
                            win32_process_digital_button(&input.mouse.left, left_button_down);

                            bool8 right_button_down = input.mouse.right.down;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_DOWN) right_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_RIGHT_BUTTON_UP) right_button_down = EX_FALSE;
                            win32_process_digital_button(&input.mouse.right, right_button_down);
                            
                            bool8 middle_button_down = input.mouse.middle.down;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) middle_button_down = EX_TRUE;
                            if (button_flags & RI_MOUSE_MIDDLE_BUTTON_UP) middle_button_down = EX_FALSE;
                            win32_process_digital_button(&input.mouse.middle, middle_button_down);

                            if (raw_input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                                input.mouse.delta_wheel += ((SHORT)raw_input->data.mouse.usButtonData) / WHEEL_DELTA;
                            }
                        }
                    }
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                } break;
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP: {
                    bool8 down = EX_FALSE;
                    if (msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYDOWN) down = EX_TRUE;
                    if (msg.message == WM_SYSKEYUP || msg.message == WM_KEYUP) down = EX_FALSE;
                    win32_process_digital_button(&input.keyboard[msg.wParam], down);
                } break;
                case WM_QUIT: {
                    UnregisterClassA(MAKEINTATOM(window_atom), window_instance);
                    global_quit = EX_TRUE;
                } break;
                default: {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        RECT client_rectangle;
        GetClientRect(window_handle, &client_rectangle);
        window_width = client_rectangle.right - client_rectangle.left;
        window_height = client_rectangle.bottom - client_rectangle.top;

        POINT window_position = { client_rectangle.left, client_rectangle.top };
        ClientToScreen(window_handle, &window_position);
        window_x = window_position.x;
        window_y = window_position.y;
        
        // NOTE(xkazu0x): mouse update
        input.mouse.position.x += input.mouse.delta_position.x;
        input.mouse.position.y += input.mouse.delta_position.y;
        input.mouse.wheel += input.mouse.delta_wheel;

        POINT mouse_position;
        GetCursorPos(&mouse_position);
        ScreenToClient(window_handle, &mouse_position);
        if (mouse_position.x < 0) mouse_position.x = 0;
        if (mouse_position.y < 0) mouse_position.y = 0;
        if (mouse_position.x > window_width) mouse_position.x = window_width;
        if (mouse_position.y > window_height) mouse_position.y = window_height;
        input.mouse.position.x = mouse_position.x;
        input.mouse.position.y = mouse_position.y;
                        
        // NOTE(xkazu0x): gamepad update
        uint32_t max_gamepad_count = XUSER_MAX_COUNT;
        if (max_gamepad_count > EX_ARRAY_COUNT(input.gamepads)) {
            max_gamepad_count = EX_ARRAY_COUNT(input.gamepads);
        }
        for (uint32 i = 0; i < max_gamepad_count; i++) {
            XINPUT_STATE xinput_state = {};
            if (xinput_get_state(i, &xinput_state) == ERROR_SUCCESS) {
                XINPUT_GAMEPAD *xgamepad = &xinput_state.Gamepad;
                ex_gamepad_input *gamepad = &input.gamepads[i];
                win32_process_digital_button(&gamepad->up, (xgamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
                win32_process_digital_button(&gamepad->down, (xgamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
                win32_process_digital_button(&gamepad->left, (xgamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
                win32_process_digital_button(&gamepad->right, (xgamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
                win32_process_digital_button(&gamepad->start, (xgamepad->wButtons & XINPUT_GAMEPAD_START) != 0);
                win32_process_digital_button(&gamepad->back, (xgamepad->wButtons & XINPUT_GAMEPAD_BACK) != 0);
                win32_process_digital_button(&gamepad->left_thumb,(xgamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
                win32_process_digital_button(&gamepad->right_thumb, (xgamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
                win32_process_digital_button(&gamepad->left_shoulder, (xgamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
                win32_process_digital_button(&gamepad->right_shoulder, (xgamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
                win32_process_digital_button(&gamepad->a, (xgamepad->wButtons & XINPUT_GAMEPAD_A) != 0);
                win32_process_digital_button(&gamepad->b, (xgamepad->wButtons & XINPUT_GAMEPAD_B) != 0);
                win32_process_digital_button(&gamepad->x, (xgamepad->wButtons & XINPUT_GAMEPAD_X) != 0);
                win32_process_digital_button(&gamepad->y, (xgamepad->wButtons & XINPUT_GAMEPAD_Y) != 0);
                win32_process_analog_button(&gamepad->left_trigger, xgamepad->bLeftTrigger / 255.0f);
                win32_process_analog_button(&gamepad->right_trigger, xgamepad->bRightTrigger / 255.0f);
#define CONVERT(x) (2.0f * (((x + 32768) / 65535.0f) - 0.5f))
                win32_process_stick(&gamepad->left_stick, CONVERT(xgamepad->sThumbLX), CONVERT(xgamepad->sThumbLY));
                win32_process_stick(&gamepad->right_stick, CONVERT(xgamepad->sThumbRX), CONVERT(xgamepad->sThumbRY));
#undef CONVERT
            }
        }
                
        // NOTE(xkazu0x): input
        if (input.keyboard[EX_KEY_ESCAPE].pressed) DestroyWindow(window_handle);
        if (input.keyboard[EX_KEY_F11].pressed) win32_window_toggle_fullscreen(window_handle, &global_window_placement);

        // NOTE(xkazu0x): sound check
        DWORD byte_to_lock = 0;
        DWORD target_cursor = 0;
        DWORD bytes_to_write = 0;
        
        DWORD play_cursor = 0;
        DWORD write_cursor = 0;
        bool8 sound_is_valid = EX_FALSE;
        if (SUCCEEDED(global_sound_buffer->GetCurrentPosition(&play_cursor, &write_cursor))) {
            byte_to_lock = ((sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.buffer_size);
            target_cursor = ((play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.buffer_size);
            if (byte_to_lock > target_cursor) {
                bytes_to_write = (sound_output.buffer_size - byte_to_lock);
                bytes_to_write += target_cursor;
            } else {
                bytes_to_write = target_cursor - byte_to_lock;
            }
            sound_is_valid = EX_TRUE;
        }

        // NOTE(xkazu0x): game update and render
        ex_sound_buffer sound_buffer = {};
        sound_buffer.samples_per_second = sound_output.samples_per_second;
        sound_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
        sound_buffer.samples = samples;

        description->update_and_render(&input, &sound_buffer);
        
        // NOTE(xkazu0x): sound update
        if (sound_is_valid) {
            win32_fill_direct_sound_buffer(&sound_output,
                                           byte_to_lock,
                                           bytes_to_write,
                                           &sound_buffer);
        }

        // NOTE(xkazu0x): time update
        int64 end_cycle_count = __rdtsc();
        
        LARGE_INTEGER end_counter;
        QueryPerformanceCounter(&end_counter);

        int64 counts_per_frame = end_counter.QuadPart - last_counter.QuadPart;
        int64 cycles_per_frame = end_cycle_count - last_cycle_count;
        
        float32 ms_per_frame = (1000.0f * ((float32)counts_per_frame / (float32)counts_per_second));
        float32 frames_per_second = (float32)counts_per_second / (float32)counts_per_frame;
        float32 mega_cycles_per_frame = ((float32)cycles_per_frame / (1000.0f * 1000.0f));
        
#if 1
        char new_window_title[512];
        sprintf(new_window_title, "%s - %.02fms, %.02ffps, %.02fmc", window_title, ms_per_frame, frames_per_second, mega_cycles_per_frame);
        SetWindowTextA(window_handle, new_window_title);
#endif
        
        last_counter = end_counter;
        last_cycle_count = end_cycle_count;
    }
    EXINFO("-+=+EXCALIBUR : SHUTDOWN+=+-");
    return(EX_TRUE);
}
