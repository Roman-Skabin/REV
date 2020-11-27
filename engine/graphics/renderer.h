//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/vec.h"

class ENGINE_API Renderer final
{
public:
    void StartFrame();
    void EndFrame();

    bool VSyncEnabled();
    void SetVSync(bool enable);

    void WaitForGPU();

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

    friend class Window;
    friend class GraphicsAPI;
};
