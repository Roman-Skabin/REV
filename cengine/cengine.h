//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"
#include "input.h"
#include "tools/work_queue.h"
#include "core/memory.h"
#include "core/allocator.h"
#include "tools/logger.h"
#include "sound/sound.h"
#include "gpu/gpu_manager.h"
#include "gpu/gpu_memory_manager.h"

#define USER_CALLBACK(name) void name(Engine *engine)
typedef USER_CALLBACK(UserCallback);

#ifdef __cplusplus
    #define USER_MAIN() int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
#else
    #define USER_MAIN() int WINAPI WinMain(HINSTANCE instance, HINSTANCE phi, LPSTR cl, int cs)
#endif

struct Engine
{
    struct
    {
        HWND handle;
        HDC  context;
        v2s  size;
        b32  closed;
        b32  resized;
        b32  fullscreened;
    } window;

    struct
    {
        HMONITOR handle;
        v2s      pos;
        v2s      size;
    } monitor;

    struct
    {
        Key     keys[KEY_MAX];
        Mouse   mouse;
        Gamepad gamepad;
    } input;

    struct
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
        UserCallback  *OnInit;
        UserCallback  *OnDestroy;
        UserCallback  *OnUpdateAndRender;
        UserCallback  *OnRender;
        SoundCallback *OnSound;
    } user_callbacks;

    WorkQueue   *work_queue;
    Memory      *memory;
    Allocator    allocator;
    Logger       logger;
    SoundStream  sound;

    GPUManager       gpu_manager;
    GPUMemoryManager gpu_memory_manager;

    void *user_ponter;
};

CENGINE_FUN int EngineRun(
    OPTIONAL HINSTANCE      instance,
    IN       UserCallback  *OnInit,
    IN       UserCallback  *OnDestroy,
    IN       UserCallback  *OnUpdateAndRender,
    IN       SoundCallback *OnSound
);

CENGINE_FUN void TimerStart(Engine *engine);
CENGINE_FUN void TimerStop(Engine *engine);

CENGINE_FUN void SetFullscreen(Engine *engine, b32 set);
