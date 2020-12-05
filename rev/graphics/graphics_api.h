//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/allocator.h"
#include "core/window.h"

#include "graphics/renderer.h"
#include "graphics/memory_manager.h"
#include "graphics/program_manager.h"

#include "tools/logger.h"

namespace REV
{
    class REV_API GraphicsAPI final
    {
    public:
        enum class API
        {
            NONE,
            D3D12,
            VULKAN,
        };

        static constexpr API GetAPI() { return s_API; }

        static void SetGraphicsAPI(API api);

        static Renderer            *GetRenderer();
        static GPU::MemoryManager  *GetMemoryManager();
        static GPU::ProgramManager *GetProgramManager();

    private:
        static void Init(Window *window, Allocator *allocator, const Logger& logger, Math::v2s rt_size);
        static void Destroy();

    private:
        static API                  s_API;
        static Renderer            *s_Renderer;
        static GPU::MemoryManager  *s_MemoryManager;
        static GPU::ProgramManager *s_ProgramManager;

        friend class Application;
    };
}
