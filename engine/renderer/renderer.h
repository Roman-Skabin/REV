//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

ENGINE_INTERFACE ENGINE_API IRenderer
{
    virtual void Destroy() = 0;

    virtual void StartFrame() = 0;
    virtual void EndFrame()   = 0;

    virtual bool VSyncEnabled()        = 0;
    virtual void SetVSync(bool enable) = 0;

    virtual void WaitForGPU() = 0;
    virtual void FlushGPU()   = 0;

private:
    virtual void SetFullscreenMode(bool set) = 0;

    friend class Window;
};
