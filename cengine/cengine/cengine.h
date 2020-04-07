//
// Copyright 2020 Roman Skabin
//

#pragma once

// @NOTE(Roman): Exchange with your own .lib file[s]
#pragma comment(lib, "sandbox.lib")

#include "core/core.h"
#include "tools/logger.h"
#include "math/mat.h"
#include "input.h"
#include "graphics/renderer.h"
#include "tools/work_queue.h"
#include "core/memory.h"
#include "core/allocator.h"
#include "sound/sound.h"

#ifndef ENGINE_DEFINED
#define ENGINE_DEFINED
    typedef struct Engine Engine;
#endif

#define USER_CALLBACK(name) void name(Engine *engine)
typedef USER_CALLBACK(UserCallback);

struct Engine
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

    _Out_ SoundStream  sound;
    _Out_ Renderer     renderer;
    _Out_ Logger       logger;
    _Out_ WorkQueue   *queue;
    _Out_ Memory      *memory;
    _Out_ Allocator    allocator;

    _In_ f32   cpu_frame_rate_limit;
    _In_ void *user_ponter;
};

CEXTERN USER_CALLBACK(User_OnInit);
CEXTERN USER_CALLBACK(User_OnDestroy);
CEXTERN USER_CALLBACK(User_OnUpdate);
CEXTERN USER_CALLBACK(User_OnRender);
CEXTERN SOUND_CALLBACK(User_SoundCallback);

CEXTERN void TimerStart(Engine *engine);
CEXTERN void TimerStop(Engine *engine);

CEXTERN void SetFullscreen(Engine *engine, b32 set);
