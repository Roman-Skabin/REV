//
// Copyright 2020 Roman Skabin
//

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
        #pragma warning(suppress: 4200)
        byte platform[0];

        friend class ::REV::Window;
        friend class ::REV::GraphicsAPI;
    };
}
