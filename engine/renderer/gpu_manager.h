//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/window.h"
#include "tools/logger.h"

enum class GRAPHICS_API
{
    NONE,
    D3D12,
    VULKAN,
};

interface ENGINE_IMPEXP IGPUManager
{
    virtual void Destroy() = 0;

    virtual void WaitForGPU() = 0;
    virtual void FlushGPU()   = 0;

    virtual void ResizeBuffers() = 0;
    virtual void ResizeTarget()  = 0;

    virtual void StartFrame() = 0;
    virtual void EndFrame()   = 0;
};

ENGINE_FUN IGPUManager *CreateGPUManager(
    in GRAPHICS_API  api,
    in Window       *window,
    in const Logger& logger
);

ENGINE_FUN IGPUManager *GetGPUManager();
