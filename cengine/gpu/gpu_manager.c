//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "gpu/gpu_manager.h"
#include "cengine.h"

#define SafeRelease(directx_interface)                           \
{                                                                \
    if (directx_interface)                                       \
    {                                                            \
        (directx_interface)->lpVtbl->Release(directx_interface); \
        (directx_interface) = 0;                                 \
    }                                                            \
}

void SetVSync(Engine *engine, b32 enable)
{
    if (engine->gpu_manager.vsync != enable)
        engine->gpu_manager.vsync = enable;
}
