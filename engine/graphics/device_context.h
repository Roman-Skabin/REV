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

    enum TEXTURE_FORMAT : u32;

    class REV_API DeviceContext final
    {
    public:
        void StartFrame();
        void EndFrame();

        bool VSyncEnabled();
        void SetVSync(bool enable);

        void WaitForGPU();

        u8 GetFormatPlanesCount(TEXTURE_FORMAT format);

        bool FrameStarted();

        Math::v2s REV_VECTORCALL RTSize();

    private:
        void SetFullscreenMode(bool set);

        REV_REMOVE_OOP_STUFF(DeviceContext);

    private:
        friend class Window;
        friend class GraphicsAPI;
    };
}
