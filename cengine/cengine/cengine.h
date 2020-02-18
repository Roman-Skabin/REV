//
// Copyright 2020 Roman Skabin
//

#pragma once

// exchange with your own lib file (if you have it obviously)
#pragma comment(lib, "sandbox.lib")

#include "core/core.h"
#include "core/logger.h"
#include "math/mat.h"
#include "input.h"
#include "graphics/renderer.h"
#include "core/work_queue.h"
#include "core/memory.h"

#ifndef ENGINE_STATE_DEFINED
#define ENGINE_STATE_DEFINED
    typedef struct EngineState EngineState;
#endif

#define USER_CALLBACK(name) void name(EngineState *engine_state)
typedef USER_CALLBACK(UserCallback);

#define AUDIO_CALLBACK(name) void name(EngineState *state, f32 *samples, u32 num_samples)
typedef AUDIO_CALLBACK(AudioCallback);

struct EngineState
{
    _Out_ struct
    {
        HWND handle;
        HDC  context;
        v2s  size;
        b32  closed;
        b32  resized;
        b32  fullscreened;
    } window;

    _Out_ struct
    {
        HMONITOR handle;
        v2s      pos;
        v2s      size;
    } monitor;

    _Out_ struct
    {
        Key     keys[KEY_MAX];
        Mouse   mouse;
        Gamepad gamepad;
    } input;

    _Out_ struct
    {
        s64 initial_ticks;
        s64 ticks_per_second;

        s64 ticks;
        s64 delta_ticks;

        s64 stop_begin;
        s64 stop_duration;
        s64 stop_last_duration;

        f32 seconds;
        f32 delta_seconds;
        f32 total_seconds;

        b32 stopped;
    } timer;

    struct
    {
        union
        {
            WAVEFORMATEXTENSIBLE extensible;
            WAVEFORMATEX         ex;
        } wave_format;
        IMMDevice            *device;
        IAudioClient         *client;
        IAudioRenderClient   *renderer;
        IAudioStreamVolume   *volume;
        HRESULT               error;
        _In_ b32              pause;
    } audio;

    _Out_ Renderer renderer;

    Logger logger;

    _Out_ WorkQueue *queue;

    _Out_ Memory memory;

    _In_ f32   cpu_frame_rate_limit;
    _In_ void *user_ponter;
};

CEXTERN USER_CALLBACK(User_OnInit);
CEXTERN USER_CALLBACK(User_OnDestroy);
CEXTERN USER_CALLBACK(User_OnUpdate);
CEXTERN USER_CALLBACK(User_OnRender);
CEXTERN AUDIO_CALLBACK(User_AudioCallback);

CEXTERN void TimerStart(EngineState *state);
CEXTERN void TimerStop(EngineState *state);

CEXTERN void SetFullscreen(EngineState *state, b32 set);
