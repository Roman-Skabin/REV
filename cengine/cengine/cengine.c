//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "cengine.h"
#include "core/memory.h"
#include "core/id.h"

//
// Input
//

typedef DWORD (WINAPI *XINPUT_GET_STATE)(DWORD dwUserIndex, XINPUT_STATE *pState);
internal XINPUT_GET_STATE XInputGetState_;
global HMODULE gXinput;

internal INLINE void DigitalButtonPull(DigitalButton *button, b32 down)
{
    b32 was_down = button->down;

    button->down     =  down;
    button->pressed  = !was_down &&  down;
    button->released =  was_down && !down;
}

internal INLINE void AnalogButtonPull(AnalogButton *button, f32 value)
{
    b32 was_down = button->down;

    button->value    =  value;
    button->down     =  button->value >= button->threshold;
    button->pressed  = !was_down &&  button->down;
    button->released =  was_down && !button->down;
}

internal INLINE void StickPull(Stick *stick, f32 x, f32 y)
{
    #define ABSF(val) ((val) < 0.0f ? (-(val)) : (val))
    stick->offset.x = ABSF(x) <= stick->deadzone ? x : 0;
    stick->offset.y = ABSF(y) <= stick->deadzone ? y : 0;
    #undef ABSF
}

internal void GamepadPull(EngineState *state)
{
    XINPUT_STATE xinput_state = {0};

    b32 was_connected = state->input.gamepad.connected;
    state->input.gamepad.connected = XInputGetState_(0, &xinput_state) == ERROR_SUCCESS;

    /**/ if (!was_connected &&  state->input.gamepad.connected) Log(&state->logger, "gamepad was connected");
    else if ( was_connected && !state->input.gamepad.connected) Log(&state->logger, "gamepad was disconnected");

    if (state->input.gamepad.connected)
    {
        #define IS_DIGITAL_DOWN(button) ((xinput_state.Gamepad.wButtons & (button)) != 0)

        DigitalButtonPull(&state->input.gamepad.button_a, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_A));
        DigitalButtonPull(&state->input.gamepad.button_b, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_B));
        DigitalButtonPull(&state->input.gamepad.button_x, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_X));
        DigitalButtonPull(&state->input.gamepad.button_y, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_Y));

        DigitalButtonPull(&state->input.gamepad.left_shoulder, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_SHOULDER));
        DigitalButtonPull(&state->input.gamepad.right_shoulder, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_SHOULDER));

        DigitalButtonPull(&state->input.gamepad.left_stick.button, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_THUMB));
        DigitalButtonPull(&state->input.gamepad.right_stick.button, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_THUMB));
        
        DigitalButtonPull(&state->input.gamepad.button_start, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_START));
        DigitalButtonPull(&state->input.gamepad.button_back, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_BACK));

        #undef IS_DIGITAL_DOWN

        AnalogButtonPull(&state->input.gamepad.left_trigger, xinput_state.Gamepad.bLeftTrigger / cast(f32, MAXBYTE));
        AnalogButtonPull(&state->input.gamepad.right_trigger, xinput_state.Gamepad.bRightTrigger / cast(f32, MAXBYTE));

        #define STICK_OFFSET(stick_dir) (2.0f * (((stick_dir) + 0x8000) / cast(f32, MAXWORD) - 0.5f))

        StickPull(&state->input.gamepad.left_stick, STICK_OFFSET(xinput_state.Gamepad.sThumbLX), STICK_OFFSET(xinput_state.Gamepad.sThumbLY));
        StickPull(&state->input.gamepad.right_stick, STICK_OFFSET(xinput_state.Gamepad.sThumbRX), STICK_OFFSET(xinput_state.Gamepad.sThumbRY));

        #undef STICK_OFFSET
    }
}

internal void InputCreate(EngineState *state)
{
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.dwFlags     = 0;
    rid.hwndTarget  = state->window.handle;

    if (RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        Success(&state->logger, "Mouse was created");

        POINT pos = {0};
        if (GetCursorPos(&pos) && ScreenToClient(state->window.handle, &pos))
        {
            state->input.mouse.pos.x = pos.x;
            state->input.mouse.pos.y = pos.y;
        }
    }
    else
    {
        Error(&state->logger, "Mouse was not created");
        FailedM("Mouse creation failure");
    }

    if (!gXinput)
    {
        Check(gXinput         = LoadLibraryA(XINPUT_DLL_A));
        Check(XInputGetState_ = cast(XINPUT_GET_STATE, GetProcAddress(gXinput, "XInputGetState")));
    }

    state->input.gamepad.left_trigger.threshold  = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / cast(f32, MAXBYTE);
    state->input.gamepad.right_trigger.threshold = state->input.gamepad.left_trigger.threshold;

    state->input.gamepad.left_stick.deadzone  = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  / cast(f32, MAXSHORT);
    state->input.gamepad.right_stick.deadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / cast(f32, MAXSHORT);

    Success(&state->logger, "Gamepad created");

    XINPUT_STATE xinput_state = {0};
    if (state->input.gamepad.connected = (XInputGetState_(0, &xinput_state) == ERROR_SUCCESS))
    {
        Log(&state->logger, "Gamepad is connected");
    }
    else
    {
        Log(&state->logger, "Gamepad is not connected");
    }
}

internal void InputReset(EngineState *state)
{
    state->input.mouse.dpos.x = 0;
    state->input.mouse.dpos.y = 0;
    state->input.mouse.dwheel = 0;

    state->input.mouse.left_button.pressed    = false;
    state->input.mouse.left_button.released   = false;

    state->input.mouse.middle_button.pressed  = false;
    state->input.mouse.middle_button.released = false;

    state->input.mouse.right_button.pressed   = false;
    state->input.mouse.right_button.released  = false;

    state->input.mouse.x1_button.pressed      = false;
    state->input.mouse.x1_button.released     = false;

    state->input.mouse.x2_button.pressed      = false;
    state->input.mouse.x2_button.released     = false;
}

internal INLINE void InputPull(EngineState *state)
{
    BYTE keyboard_state[256];
    GetKeyboardState(keyboard_state);

    for (s16 key = KEY_FIRST; key < KEY_MAX; ++key)
    {
        DigitalButtonPull(state->input.keys + key, keyboard_state[key] >> 7);
    }

    GamepadPull(state);
}

//
// Timer
//

#define QPF(s64_val) QueryPerformanceFrequency(cast(LARGE_INTEGER *, &(s64_val)))
#define QPC(s64_val) QueryPerformanceCounter(cast(LARGE_INTEGER *, &(s64_val)))

internal void TimerCreate(EngineState *state)
{
    if (QPF(state->timer.ticks_per_second) && QPC(state->timer.initial_ticks))
    {
        Success(&state->logger, "Timer was created");
    }
    else
    {
        Error(&state->logger, "Timer was not created");
        FailedM("Timer creation failure");
    }

    state->timer.stopped = true;
    state->timer.stop_begin = state->timer.initial_ticks;
}

internal void TimerPull(EngineState *state)
{
    s64 cur_ticks;
    QPC(cur_ticks);
    cur_ticks -= state->timer.initial_ticks + state->timer.stop_duration;

    if (!state->timer.stopped)
    {
        state->timer.delta_ticks   = cur_ticks - state->timer.ticks;
        state->timer.delta_seconds = state->timer.delta_ticks / cast(f32, state->timer.ticks_per_second);

        if (state->cpu_frame_rate_limit > 0 && state->timer.delta_seconds < 1.0f / state->cpu_frame_rate_limit)
        {
            u32 sleep_ms = cast(u32, 1000.0f * (1.0f / (state->cpu_frame_rate_limit - state->timer.delta_seconds)));
            if (sleep_ms > 0)
            {
                Sleep(sleep_ms);

                QPC(cur_ticks);
                cur_ticks -= state->timer.initial_ticks + state->timer.stop_duration;

                state->timer.delta_ticks   = cur_ticks - state->timer.ticks;
                state->timer.delta_seconds = state->timer.delta_ticks / cast(f32, state->timer.ticks_per_second);
            }
        }

        state->timer.ticks   = cur_ticks;
        state->timer.seconds = state->timer.ticks / cast(f32, state->timer.ticks_per_second);
    }

    state->timer.total_seconds = (cur_ticks + state->timer.stop_duration) / cast(f32, state->timer.ticks_per_second);
}

void TimerStart(EngineState *state)
{
    if (state->timer.stopped)
    {
        s64 stop_end;
        QPC(stop_end);
        state->timer.stop_last_duration  = stop_end - state->timer.stop_begin;
        state->timer.stop_duration      += state->timer.stop_last_duration;
        state->timer.stopped             = false;

        Log(&state->logger, "Timer was started");
    }
}

void TimerStop(EngineState *state)
{
    if (!state->timer.stopped)
    {
        QPC(state->timer.stop_begin);
        state->timer.stopped = true;

        Log(&state->logger, "Timer was stopped");
    }
}

//
// Renderer
//

internal void RendererCreate(EngineState *state)
{
    state->renderer.rt.pixels = PushToTA(u32, &state->memory, state->window.size.w * state->window.size.h);

    state->renderer.rt.info.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    state->renderer.rt.info.bmiHeader.biWidth         = state->window.size.w;
    state->renderer.rt.info.bmiHeader.biHeight        = state->window.size.h;
    state->renderer.rt.info.bmiHeader.biPlanes        = 1;
    state->renderer.rt.info.bmiHeader.biBitCount      = 32;
    state->renderer.rt.info.bmiHeader.biCompression   = BI_RGB;

    state->renderer.zb.z    = PushToTAA(f32, &state->memory, state->window.size.w * state->window.size.h, sizeof(__m256));
    state->renderer.zb.size = ALIGN_UP(state->window.size.w * state->window.size.h * sizeof(f32), sizeof(__m256));

    Success(&state->logger, "Renderer was created");
}

internal INLINE void RendererResize(EngineState *state)
{
    state->renderer.rt.info.bmiHeader.biWidth  = state->window.size.w;
    state->renderer.rt.info.bmiHeader.biHeight = state->window.size.h;

    if (state->window.size.w * state->window.size.h)
    {
        state->renderer.rt.pixels = PushToTA(u32, &state->memory, state->window.size.w * state->window.size.h);
        state->renderer.zb.z      = PushToTAA(f32, &state->memory, state->window.size.w * state->window.size.h, sizeof(__m256));
    }

    state->renderer.zb.size = ALIGN_UP(state->window.size.w * state->window.size.h * sizeof(f32), sizeof(__m256));
}

internal INLINE void RendererPresent(EngineState *state)
{
    SetDIBitsToDevice(state->window.context,
        0, 0, state->window.size.w, state->window.size.h,
        0, 0, 0, state->window.size.h,
        state->renderer.rt.pixels, &state->renderer.rt.info,
        DIB_RGB_COLORS);
}

//
// Window
//

void SetFullscreen(EngineState *state, b32 set)
{
    if (set != state->window.fullscreened)
    {
        local RECT wr;

        if (set)
        {
            Check(GetWindowRect(state->window.handle, &wr));
            SetWindowLongA(state->window.handle, GWL_STYLE,
                GetWindowLongA(state->window.handle, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
            Check(SetWindowPos(state->window.handle, HWND_TOP,
                state->monitor.pos.x, state->monitor.pos.y,
                state->monitor.size.w, state->monitor.size.h,
                SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS));
        }
        else
        {
            SetWindowLongA(state->window.handle, GWL_STYLE,
                GetWindowLongA(state->window.handle, GWL_STYLE) | WS_OVERLAPPEDWINDOW);
            Check(SetWindowPos(state->window.handle, HWND_TOP,
                wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top,
                SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS));
        }

        state->window.fullscreened = set;
    }
}

//
// Sound
//

internal DWORD WINAPI AudioThreadProc(LPVOID arg)
{
    EngineState *state = cast(EngineState *, arg);

    Check(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST));

    HANDLE buffer_ready_event = CreateEventA(0, false, false, "BufferReadyEvent");
    state->audio.error = state->audio.client->lpVtbl->SetEventHandle(state->audio.client, buffer_ready_event);
    Check(SUCCEEDED(state->audio.error));

    u32 buffer_size = 0;
    state->audio.error = state->audio.client->lpVtbl->GetBufferSize(state->audio.client, &buffer_size);
    Check(SUCCEEDED(state->audio.error));

    state->audio.error = state->audio.client->lpVtbl->Start(state->audio.client);
    Check(SUCCEEDED(state->audio.error));

    Success(&state->logger, "Sound was initialized");

    while (true)
    {
        while (WaitForSingleObject(buffer_ready_event, INFINITE) != WAIT_OBJECT_0)
        {
        }

        if (!state->audio.pause)
        {
            u32 padding = 0;
            state->audio.error = state->audio.client->lpVtbl->GetCurrentPadding(state->audio.client, &padding);
            Check(SUCCEEDED(state->audio.error));

            u32  num_samples = buffer_size - padding;
            f32 *samples     = 0;

            state->audio.error = state->audio.renderer->lpVtbl->GetBuffer(state->audio.renderer, num_samples, cast(u8 **, &samples));
            Check(SUCCEEDED(state->audio.error));

            User_AudioCallback(state, samples, num_samples);

            state->audio.error = state->audio.renderer->lpVtbl->ReleaseBuffer(state->audio.renderer, num_samples, 0);
            Check(SUCCEEDED(state->audio.error));
        }
    }

    return 0;
}

internal void CreateAudio(EngineState *state)
{
    state->audio.error = CoInitializeEx(0, COINIT_MULTITHREADED);
    Check(SUCCEEDED(state->audio.error));

    IMMDeviceEnumerator *mmdevice_enumerator = 0;
    state->audio.error = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, &mmdevice_enumerator);
    Check(SUCCEEDED(state->audio.error));

    state->audio.error = mmdevice_enumerator->lpVtbl->GetDefaultAudioEndpoint(mmdevice_enumerator, eRender, eConsole, &state->audio.device);
    Check(SUCCEEDED(state->audio.error));

    state->audio.error = state->audio.device->lpVtbl->Activate(state->audio.device, &IID_IAudioClient, CLSCTX_INPROC_SERVER | CLSCTX_ACTIVATE_64_BIT_SERVER, 0, &state->audio.client);
    Check(SUCCEEDED(state->audio.error));

    DWORD device_state = 0;
    state->audio.error = state->audio.device->lpVtbl->GetState(state->audio.device, &device_state);
    Check(SUCCEEDED(state->audio.error));

    if (device_state == DEVICE_STATE_ACTIVE)
    {
        state->audio.wave_format.extensible.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
        state->audio.wave_format.extensible.Format.nChannels            = 2;
        state->audio.wave_format.extensible.Format.nSamplesPerSec       = 48000;
        state->audio.wave_format.extensible.Format.wBitsPerSample       = 32;
        state->audio.wave_format.extensible.Format.nBlockAlign          = state->audio.wave_format.extensible.Format.nChannels * state->audio.wave_format.extensible.Format.wBitsPerSample / 8;
        state->audio.wave_format.extensible.Format.nAvgBytesPerSec      = state->audio.wave_format.extensible.Format.nSamplesPerSec * state->audio.wave_format.extensible.Format.nBlockAlign;
        state->audio.wave_format.extensible.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        state->audio.wave_format.extensible.Samples.wValidBitsPerSample = state->audio.wave_format.extensible.Format.wBitsPerSample;
        state->audio.wave_format.extensible.dwChannelMask               = SPEAKER_ALL;
        state->audio.wave_format.extensible.SubFormat                   = GUID_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

        WAVEFORMATEXTENSIBLE *closest = 0; // suppressing state->audio.error = E_POINTER
        state->audio.error = state->audio.client->lpVtbl->IsFormatSupported(state->audio.client, AUDCLNT_SHAREMODE_SHARED, &state->audio.wave_format.ex, cast(WAVEFORMATEX **, &closest));
        Check(SUCCEEDED(state->audio.error));
        
        #define REFTIMES_PER_MICROSEC 10
        #define REFTIMES_PER_MILLISEC 10000
        #define REFTIMES_PER_SECOND   10000000

        REFERENCE_TIME buffer_duration = 4 * REFTIMES_PER_MILLISEC;
        DWORD stream_flags             = AUDCLNT_STREAMFLAGS_EVENTCALLBACK
                                       | AUDCLNT_STREAMFLAGS_RATEADJUST
                                       | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;

        state->audio.error = state->audio.client->lpVtbl->Initialize(state->audio.client, AUDCLNT_SHAREMODE_SHARED, stream_flags, buffer_duration, 0, &state->audio.wave_format.ex, &GUID_NULL);
        Check(SUCCEEDED(state->audio.error));

        state->audio.error = state->audio.client->lpVtbl->GetService(state->audio.client, &IID_IAudioRenderClient, &state->audio.renderer);
        Check(SUCCEEDED(state->audio.error));

        state->audio.error = state->audio.client->lpVtbl->GetService(state->audio.client, &IID_IAudioStreamVolume, &state->audio.volume);
        Check(SUCCEEDED(state->audio.error));

        Check(CloseHandle(CreateThread(0, 0, AudioThreadProc, state, 0, 0)));

        state->audio.pause = false;
    }
    else
    {
        Warning(&state->logger, "Audio device is not active");
        state->audio.pause = true;
    }

    mmdevice_enumerator->lpVtbl->Release(mmdevice_enumerator);
}

internal void DestroyAudio(EngineState *state)
{
    state->audio.pause = true;

    state->audio.error = state->audio.client->lpVtbl->Stop(state->audio.client);
    Check(SUCCEEDED(state->audio.error));

    state->audio.volume->lpVtbl->Release(state->audio.volume);
    state->audio.renderer->lpVtbl->Release(state->audio.renderer);
    state->audio.client->lpVtbl->Release(state->audio.client);

    Success(&state->logger, "Sound was destroyed");
}

//
// Main
//

internal LRESULT WINAPI WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT      result = 0;
    EngineState *state  = cast(EngineState *, GetWindowLongPtrA(window, GWLP_USERDATA));

    switch (message)
    {
        case WM_DESTROY: // 0x0002
        {
            state->window.closed = true;
        } break;

        case WM_SIZE: // 0x0005
        {
            state->window.size = v2s_1(cast(s32, lparam & 0xFFFF), cast(s32, lparam >> 16));
            RendererResize(state);
            state->window.resized = true;
            Log(&state->logger, "Window was resized");
        } break;

        case WM_DISPLAYCHANGE: // 0x007E
        {
            // CloseHandle(state->monitor.handle);
            Check(state->monitor.handle = MonitorFromWindow(state->window.handle, MONITOR_DEFAULTTONEAREST));

            MONITORINFO info;
            info.cbSize = sizeof(MONITORINFO);
            Check(GetMonitorInfoA(state->monitor.handle, &info));

            state->monitor.pos  = v2s_1(info.rcMonitor.left, info.rcMonitor.top);
            state->monitor.size = v2s_1(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);

            Log(&state->logger, "Monitor was changed");
        } break;

        case WM_INPUT: // 0x00FF
        {
            HRAWINPUT raw_input_handle = cast(HRAWINPUT, lparam);
            UINT size = 0;
            GetRawInputData(raw_input_handle, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER));

            RAWINPUT *ri = cast(RAWINPUT *, PushToTransientArea(&state->memory, size));
            if (GetRawInputData(raw_input_handle, RID_INPUT, ri, &size, sizeof(RAWINPUTHEADER)) == size
            &&  ri->header.dwType == RIM_TYPEMOUSE)
            {
                state->input.mouse.dpos.x = ri->data.mouse.lLastX;
                state->input.mouse.dpos.y = ri->data.mouse.lLastY;

                state->input.mouse.pos.x += state->input.mouse.dpos.x;
                state->input.mouse.pos.y += state->input.mouse.dpos.y;

                if (ri->data.mouse.ulButtons & RI_MOUSE_WHEEL)
                {
                    state->input.mouse.dwheel  = cast(s32, ri->data.mouse.usButtonData / WHEEL_DELTA);
                    state->input.mouse.wheel  += state->input.mouse.dwheel;
                }

                b32 down = state->input.mouse.left_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP  ) down = false;
                DigitalButtonPull(&state->input.mouse.left_button, down);

                down = state->input.mouse.right_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP  ) down = false;
	            DigitalButtonPull(&state->input.mouse.right_button, down);

                down = state->input.mouse.middle_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP  ) down = false;
	            DigitalButtonPull(&state->input.mouse.middle_button, down);

                down = state->input.mouse.x1_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP  ) down = false;
	            DigitalButtonPull(&state->input.mouse.x1_button, down);

                down = state->input.mouse.x2_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP  ) down = false;
	            DigitalButtonPull(&state->input.mouse.x2_button, down);
            }
        } break;

        case WM_MENUCHAR: // 0x0100
        {
            // @NOTE(Roman): suppress bip sound on alt+enter
            result = MNC_CLOSE << 16;
        } break;

        default:
        {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }

    return result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE phi, LPSTR cl, int cs)
{
    local EngineState state;
    // ZeroMemory(&state, sizeof(EngineState));

    CreateMemory(&state.memory, cast(u32, GB(1.75) + PAGE_SIZE));

    state.logger = CreateLogger("Engine logger", "cengine.log", LOG_TO_FILE | LOG_TO_DEBUG | LOG_TO_CONSOLE);

    WNDCLASSA wca     = {0};
    wca.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wca.lpfnWndProc   = WindowProc;
    wca.hInstance     = instance;
    wca.hCursor       = LoadCursorA(0, IDC_ARROW);
    wca.lpszClassName = "cengine_window_class";
    Check(RegisterClassA(&wca));

    state.window.size = v2s_1(960, 540);

    LONG width  = state.window.size.w;
    LONG height = state.window.size.h;

    RECT wr = { 0, 0, width, height };
    if (AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false))
    {
        width  = wr.right  - wr.left;
        height = wr.bottom - wr.top;
    }

    Check(state.window.handle  = CreateWindowA(wca.lpszClassName, "CEngine", WS_OVERLAPPEDWINDOW, 10, 10, width, height, 0, 0, wca.hInstance, 0));
    Check(state.window.context = GetDC(state.window.handle));
    Success(&state.logger, "Window was created");

    Check(state.monitor.handle = MonitorFromWindow(state.window.handle, MONITOR_DEFAULTTONEAREST));
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    Check(GetMonitorInfoA(state.monitor.handle, &info));
    state.monitor.pos  = v2s_1(info.rcMonitor.left, info.rcMonitor.top);
    state.monitor.size = v2s_1(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);
    Success(&state.logger, "Monitor was initialized");

    SetWindowLongPtrA(state.window.handle, GWLP_USERDATA, cast(LONG_PTR, &state));

    InputCreate(&state);
    TimerCreate(&state);
    RendererCreate(&state);
    CreateAudio(&state);
    Check(state.queue = CreateWorkQueue(&state));

    User_OnInit(&state);

    TimerStart(&state);

    MSG msg = {0};
    ShowWindow(state.window.handle, SW_SHOW);
    while (!state.window.closed)
    {
        // Reset
        ResetTransientArea(&state.memory);
        state.window.resized = false;
        InputReset(&state);
        
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (!state.window.closed)
        {
            // Reinitialize all stuff, than was allocated on transient memory
            RendererResize(&state);

            // Pull
            InputPull(&state);
            TimerPull(&state);

            // Update
            User_OnUpdate(&state);

            // Clear
            memset_f32(state.renderer.zb.z, 1.0f, state.renderer.zb.size / sizeof(f32));

            // Render
            User_OnRender(&state);

            // Present
            WaitForWorkQueue(state.queue);
            RendererPresent(&state);
        }
    }

    TimerStop(&state);
    DestroyAudio(&state);

    User_OnDestroy(&state);

    ReleaseDC(state.window.handle, state.window.context);
    UnregisterClassA(wca.lpszClassName, wca.hInstance);
    Log(&state.logger, "Window was destroyed");
    if (gXinput) FreeLibrary(gXinput);
    Log(&state.logger, "Input was destroyed");
    DestroyLogger(&state.logger);
    DestroyMemory(&state.memory);

    return 0;
}
