// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "math/vec.h"

namespace REV
{
    class Window;
    class GraphicsAPI;
}

namespace REV::GPU
{
    class REV_API DeviceContext final
    {
    public:
        void StartFrame();
        void EndFrame();

        bool VSyncEnabled();
        void SetVSync(bool enable);

        void WaitForGPU();

        bool FrameStarted();

    private:
        void SetFullscreenMode(bool set);

        DeviceContext()                     = delete;
        DeviceContext(const DeviceContext&) = delete;
        DeviceContext(DeviceContext&&)      = delete;

        ~DeviceContext() = delete;

        DeviceContext& operator=(const DeviceContext&) = delete;
        DeviceContext& operator=(DeviceContext&&)      = delete;

    private:
        byte platform[0];

        friend class ::REV::Window;
        friend class ::REV::GraphicsAPI;
    };
}
