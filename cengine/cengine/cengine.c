//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "cengine.h"
#include "core/id.h"
#include "math/color.h"

b32 gFrameStart = false;

//
// Input
//

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(XInputGetStateProc);
internal XInputGetStateProc *XInputGetState_;

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

internal void GamepadPull(Engine *engine)
{
    XINPUT_STATE xinput_state = {0};

    b32 was_connected = engine->input.gamepad.connected;
    engine->input.gamepad.connected = XInputGetState_(0, &xinput_state) == ERROR_SUCCESS;

    /**/ if (!was_connected &&  engine->input.gamepad.connected) Log(&engine->logger, "gamepad was connected");
    else if ( was_connected && !engine->input.gamepad.connected) Log(&engine->logger, "gamepad was disconnected");

    if (engine->input.gamepad.connected)
    {
        #define IS_DIGITAL_DOWN(button) ((xinput_state.Gamepad.wButtons & (button)) != 0)

        DigitalButtonPull(&engine->input.gamepad.button_a, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_A));
        DigitalButtonPull(&engine->input.gamepad.button_b, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_B));
        DigitalButtonPull(&engine->input.gamepad.button_x, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_X));
        DigitalButtonPull(&engine->input.gamepad.button_y, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_Y));

        DigitalButtonPull(&engine->input.gamepad.left_shoulder, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_SHOULDER));
        DigitalButtonPull(&engine->input.gamepad.right_shoulder, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_SHOULDER));

        DigitalButtonPull(&engine->input.gamepad.left_stick.button, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_THUMB));
        DigitalButtonPull(&engine->input.gamepad.right_stick.button, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_THUMB));
        
        DigitalButtonPull(&engine->input.gamepad.button_start, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_START));
        DigitalButtonPull(&engine->input.gamepad.button_back, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_BACK));

        #undef IS_DIGITAL_DOWN

        AnalogButtonPull(&engine->input.gamepad.left_trigger, xinput_state.Gamepad.bLeftTrigger / cast(f32, MAXBYTE));
        AnalogButtonPull(&engine->input.gamepad.right_trigger, xinput_state.Gamepad.bRightTrigger / cast(f32, MAXBYTE));

        #define STICK_OFFSET(stick_dir) (2.0f * (((stick_dir) + 0x8000) / cast(f32, MAXWORD) - 0.5f))

        StickPull(&engine->input.gamepad.left_stick, STICK_OFFSET(xinput_state.Gamepad.sThumbLX), STICK_OFFSET(xinput_state.Gamepad.sThumbLY));
        StickPull(&engine->input.gamepad.right_stick, STICK_OFFSET(xinput_state.Gamepad.sThumbRX), STICK_OFFSET(xinput_state.Gamepad.sThumbRY));

        #undef STICK_OFFSET
    }
}

internal void InputCreate(Engine *engine)
{
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.dwFlags     = 0;
    rid.hwndTarget  = engine->window.handle;

    if (RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        Success(&engine->logger, "Mouse was created");

        POINT pos = {0};
        if (GetCursorPos(&pos) && ScreenToClient(engine->window.handle, &pos))
        {
            engine->input.mouse.pos.x = pos.x;
            engine->input.mouse.pos.y = pos.y;
        }
    }
    else
    {
        Error(&engine->logger, "Mouse was not created");
        FailedM("Mouse creation failure");
    }

    if (!gXinput)
    {
        gXinput = LoadLibraryA(XINPUT_DLL_A);
        Check(gXinput);

        XInputGetState_ = cast(XInputGetStateProc *, GetProcAddress(gXinput, "XInputGetState"));
        Check(XInputGetState_);
    }

    engine->input.gamepad.left_trigger.threshold  = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / cast(f32, MAXBYTE);
    engine->input.gamepad.right_trigger.threshold = engine->input.gamepad.left_trigger.threshold;

    engine->input.gamepad.left_stick.deadzone  = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  / cast(f32, MAXSHORT);
    engine->input.gamepad.right_stick.deadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / cast(f32, MAXSHORT);

    Success(&engine->logger, "Gamepad created");

    XINPUT_STATE xinput_state = {0};
    if (engine->input.gamepad.connected = (XInputGetState_(0, &xinput_state) == ERROR_SUCCESS))
    {
        Log(&engine->logger, "Gamepad is connected");
    }
    else
    {
        Log(&engine->logger, "Gamepad is not connected");
    }
}

internal void InputDestroy(Engine *engine)
{
    if (gXinput) FreeLibrary(gXinput);
    Log(&engine->logger, "Input was destroyed");
}

internal void InputReset(Engine *engine)
{
    engine->input.mouse.dpos.x = 0;
    engine->input.mouse.dpos.y = 0;
    engine->input.mouse.dwheel = 0;

    engine->input.mouse.left_button.pressed    = false;
    engine->input.mouse.left_button.released   = false;

    engine->input.mouse.middle_button.pressed  = false;
    engine->input.mouse.middle_button.released = false;

    engine->input.mouse.right_button.pressed   = false;
    engine->input.mouse.right_button.released  = false;

    engine->input.mouse.x1_button.pressed      = false;
    engine->input.mouse.x1_button.released     = false;

    engine->input.mouse.x2_button.pressed      = false;
    engine->input.mouse.x2_button.released     = false;
}

internal INLINE void InputPull(Engine *engine)
{
    BYTE keyboard_state[256];
    GetKeyboardState(keyboard_state);

    for (s16 key = KEY_FIRST; key < KEY_MAX; ++key)
    {
        DigitalButtonPull(engine->input.keys + key, keyboard_state[key] >> 7);
    }

    GamepadPull(engine);
}

//
// Timer
//

#define QPF(s64_val) QueryPerformanceFrequency(cast(LARGE_INTEGER *, &(s64_val)))
#define QPC(s64_val) QueryPerformanceCounter(cast(LARGE_INTEGER *, &(s64_val)))

internal void TimerCreate(Engine *engine)
{
    if (QPF(engine->timer.ticks_per_second) && QPC(engine->timer.initial_ticks))
    {
        Success(&engine->logger, "Timer was created");
    }
    else
    {
        Error(&engine->logger, "Timer was not created");
        FailedM("Timer creation failure");
    }

    engine->timer.stopped    = true;
    engine->timer.stop_begin = engine->timer.initial_ticks;
}

internal void TimerPull(Engine *engine)
{
    s64 cur_ticks;
    QPC(cur_ticks);
    cur_ticks -= engine->timer.initial_ticks + engine->timer.stop_duration;

    if (!engine->timer.stopped)
    {
        engine->timer.delta_ticks   = cur_ticks - engine->timer.ticks;
        engine->timer.delta_seconds = engine->timer.delta_ticks / cast(f32, engine->timer.ticks_per_second);

        if (engine->cpu_frame_rate_limit > 0 && engine->timer.delta_seconds < 1.0f / engine->cpu_frame_rate_limit)
        {
            u32 sleep_ms = cast(u32, 1000.0f * (1.0f / (engine->cpu_frame_rate_limit - engine->timer.delta_seconds)));
            if (sleep_ms > 0)
            {
                Sleep(sleep_ms);

                QPC(cur_ticks);
                cur_ticks -= engine->timer.initial_ticks + engine->timer.stop_duration;

                engine->timer.delta_ticks   = cur_ticks - engine->timer.ticks;
                engine->timer.delta_seconds = engine->timer.delta_ticks / cast(f32, engine->timer.ticks_per_second);
            }
        }

        engine->timer.ticks   = cur_ticks;
        engine->timer.seconds = engine->timer.ticks / cast(f32, engine->timer.ticks_per_second);
    }

    engine->timer.total_seconds = (cur_ticks + engine->timer.stop_duration) / cast(f32, engine->timer.ticks_per_second);
}

void TimerStart(Engine *engine)
{
    if (engine->timer.stopped)
    {
        s64 stop_end;
        QPC(stop_end);
        engine->timer.stop_last_duration  = stop_end - engine->timer.stop_begin;
        engine->timer.stop_duration      += engine->timer.stop_last_duration;
        engine->timer.stopped             = false;

        Log(&engine->logger, "Timer was started");
    }
}

void TimerStop(Engine *engine)
{
    if (!engine->timer.stopped)
    {
        QPC(engine->timer.stop_begin);
        engine->timer.stopped = true;

        Log(&engine->logger, "Timer was stopped");
    }
}

//
// Renderer
//

internal void RendererCreate(Engine *engine)
{
    engine->renderer.rt.info.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    engine->renderer.rt.info.bmiHeader.biWidth         = engine->window.size.w;
    engine->renderer.rt.info.bmiHeader.biHeight        = engine->window.size.h;
    engine->renderer.rt.info.bmiHeader.biPlanes        = 1;
    engine->renderer.rt.info.bmiHeader.biBitCount      = 32;
    engine->renderer.rt.info.bmiHeader.biCompression   = BI_RGB;

    engine->renderer.common.count = engine->window.size.w * engine->window.size.h;

    engine->renderer.rt.pixels    = PushToTA(u32, engine->memory, engine->renderer.common.count);
    engine->renderer.zb.z         = PushToTAA(f32, engine->memory, engine->renderer.common.count, sizeof(__m256));
    engine->renderer.blending.sum = PushToTA(v4, engine->memory, engine->renderer.common.count);
    engine->renderer.blending.mul = PushToTAA(f32, engine->memory, engine->renderer.common.count, sizeof(__m256));

    Success(&engine->logger, "Renderer was created");
}

internal void RendererResize(Engine *engine)
{
    engine->renderer.rt.info.bmiHeader.biWidth  = engine->window.size.w;
    engine->renderer.rt.info.bmiHeader.biHeight = engine->window.size.h;

    engine->renderer.common.count = engine->window.size.w * engine->window.size.h;

    if (engine->renderer.common.count)
    {
        engine->renderer.rt.pixels    = PushToTA(u32, engine->memory, engine->renderer.common.count);
        engine->renderer.zb.z         = PushToTAA(f32, engine->memory, engine->renderer.common.count, sizeof(__m256));
        engine->renderer.blending.sum = PushToTA(v4, engine->memory, engine->renderer.common.count);
        engine->renderer.blending.mul = PushToTAA(f32, engine->memory, engine->renderer.common.count, sizeof(__m256));
    }
}

typedef struct ChunkInfo
{
    Engine *engine;
    u32     start;
    u32     end;
} ChunkInfo;

internal WORK_QUEUE_ENTRY_PROC(MergeOutput)
{
    ChunkInfo *info = arg;

    for (u32 i = info->start; i < info->end; ++i)
    {
        u32 *pixel = info->engine->renderer.rt.pixels    + i;
        v4  *sum   = info->engine->renderer.blending.sum + i;

        if (sum->r || sum->g || sum->b)
        {
            __m128 mm_mul = _mm_set_ps1(info->engine->renderer.blending.mul[i]);
            __m128 mm_one = _mm_set_ps1(1.0f);
            __m128 mm_255 = _mm_set_ps1(255.0f);

            // hex_to_norm with zero alpha
            __m128 dest_color = _mm_div_ps(
                                    _mm_cvtepi32_ps(
                                        _mm_and_si128(
                                            _mm_setr_epi32(*pixel >> 16, *pixel >> 8, *pixel, 0),
                                            _mm_set1_epi32(0xFF))),
                                    mm_255);
        
            __m128 fraction = _mm_div_ps(sum->mm, _mm_set_ps(sum->a, sum->a, sum->a, 1.0f));
            __m128 bracket  = _mm_sub_ps(mm_one, mm_mul);
            __m128 left     = _mm_mul_ps(fraction, bracket);
            __m128 right    = _mm_mul_ps(dest_color, mm_mul);
            __m128 norm     = _mm_add_ps(left, right);

            // norm_to_hex
            __m128i hex = _mm_cvtps_epi32(
                              _mm_mul_ps(
                                  mm_255,
                                  _mm_max_ps(
                                      _mm_setzero_ps(),
                                      _mm_min_ps(
                                          mm_one,
                                          norm))));

            *pixel = (hex.m128i_u32[3] << 24)
                   | (hex.m128i_u32[0] << 16)
                   | (hex.m128i_u32[1] << 8 )
                   | (hex.m128i_u32[2]      );
        }
    }
}

internal void RendererPresent(Engine *engine)
{
    // Waiting for renderer
    WaitForWorkQueue(engine->queue);

    u32 chunks_count = engine->renderer.common.count / PAGE_SIZE;

    u32 additional_last_chunk_bytes = engine->renderer.common.count % PAGE_SIZE;
    if (additional_last_chunk_bytes)
    {
        ChunkInfo info;
        info.engine = engine;
        info.start  = engine->renderer.common.count + additional_last_chunk_bytes - PAGE_SIZE;
        info.end    = engine->renderer.common.count;

        AddWorkQueueEntry(engine->queue, MergeOutput, &info);
    }

    ChunkInfo *infos = PushToTA(ChunkInfo, engine->memory, chunks_count);
    for (u32 i = 0; i < chunks_count; ++i)
    {
        ChunkInfo *info = infos + i;
        info->engine    = engine;
        info->start     = i * PAGE_SIZE;
        info->end       = info->start + PAGE_SIZE;

        AddWorkQueueEntry(engine->queue, MergeOutput, info);
    }

    // Waiting for merging
    WaitForWorkQueue(engine->queue);

    SetDIBitsToDevice(engine->window.context,
        0, 0, engine->window.size.w, engine->window.size.h,
        0, 0, 0, engine->window.size.h,
        engine->renderer.rt.pixels, &engine->renderer.rt.info,
        DIB_RGB_COLORS);
}

//
// Window
//

internal LRESULT WINAPI WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

internal void WindowCreate(Engine *engine, const char *title, v2s size, HINSTANCE instance)
{
    WNDCLASSA wca     = {0};
    wca.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wca.lpfnWndProc   = WindowProc;
    wca.hInstance     = instance;
    wca.hCursor       = LoadCursorA(0, IDC_ARROW);
    wca.lpszClassName = "cengine_window_class";
    DebugResult(RegisterClassA(&wca));

    engine->window.size = size;

    s32 width  = engine->window.size.w;
    s32 height = engine->window.size.h;

    RECT wr = { 0, 0, width, height };
    if (AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false))
    {
        width  = wr.right  - wr.left;
        height = wr.bottom - wr.top;
    }

    engine->window.handle = CreateWindowA(wca.lpszClassName, title, WS_OVERLAPPEDWINDOW, 10, 10, width, height, 0, 0, wca.hInstance, 0);
    Check(engine->window.handle);

    engine->window.context = GetDC(engine->window.handle);
    Check(engine->window.context);

    Success(&engine->logger, "Window was created");

    engine->monitor.handle = MonitorFromWindow(engine->window.handle, MONITOR_DEFAULTTONEAREST);
    Check(engine->monitor.handle);

    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    DebugResult(GetMonitorInfoA(engine->monitor.handle, &info));

    engine->monitor.pos  = v2s_1(info.rcMonitor.left, info.rcMonitor.top);
    engine->monitor.size = v2s_1(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);

    Success(&engine->logger, "Monitor was initialized");

    SetWindowLongPtrA(engine->window.handle, GWLP_USERDATA, cast(LONG_PTR, engine));
}

internal void WindowDestroy(Engine *engine, HINSTANCE instance)
{
    DebugResult(UnregisterClassA("cengine_window_class", instance));
    Log(&engine->logger, "Window was destroyed");
}

void SetFullscreen(Engine *engine, b32 set)
{
    if (set != engine->window.fullscreened)
    {
        local RECT wr;

        if (set)
        {
            DebugResult(GetWindowRect(engine->window.handle, &wr));
            SetWindowLongA(engine->window.handle, GWL_STYLE,
                           GetWindowLongA(engine->window.handle, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
            DebugResult(SetWindowPos(engine->window.handle, HWND_TOP,
                                     engine->monitor.pos.x, engine->monitor.pos.y,
                                     engine->monitor.size.w, engine->monitor.size.h,
                                     SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS));
        }
        else
        {
            SetWindowLongA(engine->window.handle, GWL_STYLE,
                           GetWindowLongA(engine->window.handle, GWL_STYLE) | WS_OVERLAPPEDWINDOW);
            DebugResult(SetWindowPos(engine->window.handle, HWND_TOP,
                                     wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top,
                                     SWP_FRAMECHANGED | SWP_ASYNCWINDOWPOS));
        }

        engine->window.fullscreened = set;
    }
}

//
// Sound
//

internal DWORD WINAPI SoundThreadProc(LPVOID arg)
{
    Engine *engine = cast(Engine *, arg);

    HANDLE event       = CreateEventExA(0, "EngineSoundEvent", 0, EVENT_ALL_ACCESS);
    engine->sound.error = engine->sound.client->lpVtbl->SetEventHandle(engine->sound.client, event);
    Check(SUCCEEDED(engine->sound.error));

    u32 buffer_size = 0;
    engine->sound.error = engine->sound.client->lpVtbl->GetBufferSize(engine->sound.client, &buffer_size);
    Check(SUCCEEDED(engine->sound.error));

    SoundBuffer buffer        = {0};
    buffer.samples_per_second = engine->sound.wave_format.Format.nSamplesPerSec;
    buffer.channels_count     = engine->sound.wave_format.Format.nChannels;

    engine->sound.error = engine->sound.client->lpVtbl->Start(engine->sound.client);
    Check(SUCCEEDED(engine->sound.error));

    while (true)
    {
        while (WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0)
        {
        }

        if (!engine->sound.pause)
        {
            u32 padding = 0;
            engine->sound.error = engine->sound.client->lpVtbl->GetCurrentPadding(engine->sound.client, &padding);
            Check(SUCCEEDED(engine->sound.error));

            buffer.samples_count = buffer_size - padding;
            buffer.samples       = 0;

            engine->sound.error = engine->sound.renderer->lpVtbl->GetBuffer(engine->sound.renderer, buffer.samples_count, cast(u8 **, &buffer.samples));
            Check(SUCCEEDED(engine->sound.error));

            User_SoundCallback(engine, &buffer);

            engine->sound.error = engine->sound.renderer->lpVtbl->ReleaseBuffer(engine->sound.renderer, buffer.samples_count, 0);
            Check(SUCCEEDED(engine->sound.error));
        }
    }

    return 0;
}

internal void CreateSound(Engine *engine)
{
    engine->sound.error = CoInitializeEx(0, COINIT_MULTITHREADED);
    Check(SUCCEEDED(engine->sound.error));

    IMMDeviceEnumerator *mmdevice_enumerator = 0;
    engine->sound.error = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, &mmdevice_enumerator);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = mmdevice_enumerator->lpVtbl->GetDefaultAudioEndpoint(mmdevice_enumerator, eRender, eConsole, &engine->sound.device);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = engine->sound.device->lpVtbl->Activate(engine->sound.device, &IID_IAudioClient, CLSCTX_INPROC_SERVER | CLSCTX_ACTIVATE_64_BIT_SERVER, 0, &engine->sound.client);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.wave_format.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    engine->sound.wave_format.Format.nChannels            = 2;
    engine->sound.wave_format.Format.nSamplesPerSec       = 48000;
    engine->sound.wave_format.Format.wBitsPerSample       = 32;
    engine->sound.wave_format.Format.nBlockAlign          = engine->sound.wave_format.Format.nChannels * engine->sound.wave_format.Format.wBitsPerSample / 8;
    engine->sound.wave_format.Format.nAvgBytesPerSec      = engine->sound.wave_format.Format.nSamplesPerSec * engine->sound.wave_format.Format.nBlockAlign;
    engine->sound.wave_format.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    engine->sound.wave_format.Samples.wValidBitsPerSample = engine->sound.wave_format.Format.wBitsPerSample;
    engine->sound.wave_format.dwChannelMask               = KSAUDIO_SPEAKER_STEREO;
    engine->sound.wave_format.SubFormat                   = (GUID){ STATIC_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT };

    DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK
                       | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
                       | AUDCLNT_STREAMFLAGS_RATEADJUST;

    engine->sound.error = engine->sound.client->lpVtbl->Initialize(engine->sound.client, AUDCLNT_SHAREMODE_SHARED, stream_flags, 0, 0, cast(WAVEFORMATEX *, &engine->sound.wave_format), 0);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = engine->sound.client->lpVtbl->GetService(engine->sound.client, &IID_IAudioRenderClient, &engine->sound.renderer);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = engine->sound.client->lpVtbl->GetService(engine->sound.client, &IID_ISimpleAudioVolume, &engine->sound.volume);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.pause = true;

    DebugResult(CloseHandle(CreateThread(0, 0, SoundThreadProc, engine, 0, 0)));

    mmdevice_enumerator->lpVtbl->Release(mmdevice_enumerator);

    engine->sound.error = MFStartup(MF_VERSION, MFSTARTUP_LITE);
    Check(SUCCEEDED(engine->sound.error));
    
    Success(&engine->logger, "Sound was initialized");
}

internal void DestroySound(Engine *engine)
{
    engine->sound.pause = true;

    engine->sound.error = engine->sound.client->lpVtbl->Stop(engine->sound.client);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.volume->lpVtbl->Release(engine->sound.volume);
    engine->sound.renderer->lpVtbl->Release(engine->sound.renderer);
    engine->sound.client->lpVtbl->Release(engine->sound.client);
    engine->sound.device->lpVtbl->Release(engine->sound.device);
    
    Success(&engine->logger, "Sound was destroyed");
}

//
// Main
//

internal LRESULT WINAPI WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT      result = 0;
    Engine *engine  = cast(Engine *, GetWindowLongPtrA(window, GWLP_USERDATA));

    switch (message)
    {
        case WM_DESTROY: // 0x0002
        {
            engine->window.closed = true;
        } break;

        case WM_SIZE: // 0x0005
        {
            engine->window.size = v2s_1(cast(s32, lparam & 0xFFFF), cast(s32, lparam >> 16));
            // RendererResize(engine);
            engine->window.resized = true;
            Log(&engine->logger, "Window was resized");
        } break;

        case WM_DISPLAYCHANGE: // 0x007E
        {
            // CloseHandle(engine->monitor.handle);
            engine->monitor.handle = MonitorFromWindow(engine->window.handle, MONITOR_DEFAULTTONEAREST);
            Check(engine->monitor.handle);

            MONITORINFO info;
            info.cbSize = sizeof(MONITORINFO);
            DebugResult(GetMonitorInfoA(engine->monitor.handle, &info));

            engine->monitor.pos  = v2s_1(info.rcMonitor.left, info.rcMonitor.top);
            engine->monitor.size = v2s_1(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);

            Log(&engine->logger, "Monitor was changed");
        } break;

        case WM_INPUT: // 0x00FF
        {
            HRAWINPUT raw_input_handle = cast(HRAWINPUT, lparam);
            UINT size = 0;
            GetRawInputData(raw_input_handle, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER));

            RAWINPUT *ri = cast(RAWINPUT *, PushToTransientArea(engine->memory, size));
            if (GetRawInputData(raw_input_handle, RID_INPUT, ri, &size, sizeof(RAWINPUTHEADER)) == size
            &&  ri->header.dwType == RIM_TYPEMOUSE)
            {
                engine->input.mouse.dpos.x = ri->data.mouse.lLastX;
                engine->input.mouse.dpos.y = ri->data.mouse.lLastY;

                engine->input.mouse.pos.x += engine->input.mouse.dpos.x;
                engine->input.mouse.pos.y += engine->input.mouse.dpos.y;

                if (ri->data.mouse.ulButtons & RI_MOUSE_WHEEL)
                {
                    engine->input.mouse.dwheel  = cast(s32, ri->data.mouse.usButtonData / WHEEL_DELTA);
                    engine->input.mouse.wheel  += engine->input.mouse.dwheel;
                }

                b32 down = engine->input.mouse.left_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP  ) down = false;
                DigitalButtonPull(&engine->input.mouse.left_button, down);

                down = engine->input.mouse.right_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP  ) down = false;
	            DigitalButtonPull(&engine->input.mouse.right_button, down);

                down = engine->input.mouse.middle_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP  ) down = false;
	            DigitalButtonPull(&engine->input.mouse.middle_button, down);

                down = engine->input.mouse.x1_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP  ) down = false;
	            DigitalButtonPull(&engine->input.mouse.x1_button, down);

                down = engine->input.mouse.x2_button.down;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) down = true;
	            if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP  ) down = false;
	            DigitalButtonPull(&engine->input.mouse.x2_button, down);
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
    DebugResult(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST));

    Memory engine_memory;
    CreateMemory(&engine_memory, GB(1ui64), GB(3ui64));

    Engine *engine = PushToPA(Engine, &engine_memory, 1);
    engine->memory = &engine_memory;

    void *base_address = PushToPermanentArea(engine->memory, GB(1ui64));
    CreateAllocator(&engine->allocator, base_address, cast(u64, GB(1.5)), false);

    CreateLogger(&engine->logger, "Engine logger", "cengine.log", LOG_TO_FILE | LOG_TO_DEBUG | LOG_TO_CONSOLE);
    WindowCreate(engine, "CEngine", v2s_1(960, 540), instance);
    InputCreate(engine);
    TimerCreate(engine);
    RendererCreate(engine);
    CreateSound(engine);
    engine->queue = CreateWorkQueue(engine);

    User_OnInit(engine);

    TimerStart(engine);

    MSG msg = {0};
    ShowWindow(engine->window.handle, SW_SHOW);
    while (!engine->window.closed)
    {
        // Reset
    #if DEBUG
        gAllocationsPerFrame   = 0;
        gReAllocationsPerFrame = 0;
        gDeAllocationsPerFrame = 0;
    #endif
        ResetTransientArea(engine->memory);
        engine->window.resized = false;
        InputReset(engine);
        gFrameStart = true;

        while (PeekMessageA(&msg, engine->window.handle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (!engine->window.closed)
        {
            // Reinitialize all stuff, was allocated on transient memory
            RendererResize(engine);

            // Pull
            InputPull(engine);
            TimerPull(engine);

            // Update
            User_OnUpdate(engine);

            // Clear
            u32 bytes = ALIGN_UP(engine->renderer.common.count * sizeof(f32), sizeof(__m256)) / sizeof(f32);
            memset_f32(engine->renderer.zb.z, engine->renderer.zb.far, bytes);
            memset_f32(engine->renderer.blending.mul, 1.0f, bytes);

            // Render
            User_OnRender(engine);

            // Present
            RendererPresent(engine);
        }
    }

    TimerStop(engine);
    DestroySound(engine);

    User_OnDestroy(engine);

    InputDestroy(engine);
    WindowDestroy(engine, instance);
    DestroyLogger(&engine->logger);
    DestroyAllocator(&engine->allocator);
    DestroyMemory(engine->memory);

    return 0;
}
