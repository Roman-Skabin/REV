// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/allocator.h"
#include "core/window.h"

#include "graphics/device_context.h"
#include "graphics/memory_manager.h"
#include "graphics/shader_manager.h"

#include "tools/logger.h"

namespace REV
{
    class REV_API GraphicsAPI final
    {
    public:
        enum API
        {
            NONE,
            D3D12,
            VULKAN,
        };

        static REV_INLINE API GetAPI() { return s_API; }

        static void SetGraphicsAPI(API api);

        static DeviceContext *GetDeviceContext();
        static MemoryManager *GetMemoryManager();
        static ShaderManager *GetShaderManager();

    private:
        static void Init(Window *window, Allocator *allocator, const Logger& logger);
        static void Destroy();

    private:
        static API            s_API;
        static DeviceContext *s_DeviceContext;
        static MemoryManager *s_MemoryManager;
        static ShaderManager *s_ShaderManager;

        friend class Application;
    };
}
