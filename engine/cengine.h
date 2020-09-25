//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"
#include "input.h"
#include "tools/work_queue.h"
#include "core/memory.h"
#include "core/allocator.h"
#include "core/window.h"
#include "tools/logger.h"
#include "sound/sound.h"
#include "gpu/gpu_manager.h"
#include "gpu/gpu_memory_manager.h"
#include "graphics/gpu_program_manager.h"
#include "tools/timer.h"

#define USER_CALLBACK(name) void name(Engine *engine)
typedef USER_CALLBACK(UserCallback);

#define USER_MAIN(_OnInit, _OnDestroy, _OnUpdateAndRender, _OnSound)                             \
int __cdecl main(int args_count, char *args[])                                                   \
{                                                                                                \
    return EngineRun(GetModuleHandleA(null), _OnInit, _OnDestroy, _OnUpdateAndRender, _OnSound); \
}

struct Engine
{
    Window window;

    struct
    {
        Key     keys[KEY_MAX];
        Mouse   mouse;
        Gamepad gamepad;
    } input;

    Timer timer;

    struct
    {
        UserCallback  *OnInit;
        UserCallback  *OnDestroy;
        UserCallback  *OnUpdateAndRender;
        SoundCallback *OnSound;
    } user_callbacks;

    WorkQueue   *work_queue;
    Memory       memory;
    Allocator    allocator;
    Logger       logger;
    SoundStream  sound;

    GPUManager        gpu_manager;
    GPUMemoryManager  gpu_memory_manager;
    GPUProgramManager gpu_program_manager;

    void *user_pointer;
};

ENGINE_FUN int EngineRun(
    opt HINSTANCE      instance,
    in  UserCallback  *OnInit,
    in  UserCallback  *OnDestroy,
    in  UserCallback  *OnUpdateAndRender,
    in  SoundCallback *OnSound
);

ENGINE_FUN void SetFullscreen(
    in Engine *engine,
    in b32     set);
