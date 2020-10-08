//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

interface ENGINE_IMPEXP IRenderer
{
    virtual void Destroy() = 0;

    virtual void ResizeBuffers() = 0;
    virtual void ResizeTarget()  = 0;

    virtual void StartFrame() = 0;
    virtual void EndFrame()   = 0;

    virtual bool VSyncEnabled()        = 0;
    virtual void SetVSync(bool enable) = 0;
};
