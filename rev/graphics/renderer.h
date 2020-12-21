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
    class REV_API Renderer final
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

        Renderer()                = delete;
        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&)      = delete;

        ~Renderer() = delete;

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&)      = delete;

    private:
        #pragma warning(suppress: 4200)
        byte platform[0];

        friend class ::REV::Window;
        friend class ::REV::GraphicsAPI;
    };
}
