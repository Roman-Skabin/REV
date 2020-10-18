//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

interface ENGINE_IMPEXP IRenderer
{
    virtual void Destroy() = 0;

    virtual void ResizeBuffers(v2 render_target_size) = 0;
    virtual void SetFullscreen(bool set)              = 0;

    virtual void StartFrame() = 0;
    virtual void EndFrame()   = 0;

    virtual bool VSyncEnabled()        = 0;
    virtual void SetVSync(bool enable) = 0;

    virtual void WaitForGPU() = 0;
    virtual void FlushGPU()   = 0;
};
