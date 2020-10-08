//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/allocator.h"
#include "core/window.h"

#include "renderer/renderer.h"
#include "renderer/gpu_memory_manager.h"

#include "tools/logger.h"

class ENGINE_IMPEXP GraphicsAPI
{
public:
    enum class API
    {
        NONE,
        D3D12,
        VULKAN,
    };

    static void SetGraphicsAPI(in API api);
    
private:
    static IRenderer         *CreateRenderer(in Window *window, in const Logger& logger);
    static IGPUMemoryManager *CreateGPUMemoryManager(in Allocator *allocator, in const Logger& logger, in u64 gpu_memory_capacity);

public:
    static IRenderer         *GetRenderer();
    static IGPUMemoryManager *GetGPUMemoryManager();

private:
    static API                s_API;
    static IRenderer         *s_Renderer;
    static IGPUMemoryManager *s_GPUMemoryManager;

    friend class Application;
};
