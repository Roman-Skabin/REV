//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/allocator.h"
#include "core/window.h"

#include "renderer/renderer.h"
#include "renderer/gpu_memory_manager.h"
#include "renderer/gpu_program_manager.h"

#include "tools/logger.h"

class ENGINE_API GraphicsAPI
{
public:
    enum class API
    {
        NONE,
        D3D12,
        VULKAN,
    };

    static void SetGraphicsAPI(API api);
    
private:
    static IRenderer          *CreateRenderer(Window *window, const Logger& logger, v2s rt_size);
    static IGPUMemoryManager  *CreateGPUMemoryManager(Allocator *allocator, const Logger& logger, u64 gpu_memory_capacity);
    static IGPUProgramManager *CreateGPUProgramManager(Allocator *allocator);

public:
    static IRenderer          *GetRenderer();
    static IGPUMemoryManager  *GetGPUMemoryManager();
    static IGPUProgramManager *GetGPUProgramManager();

private:
    static API                 s_API;
    static IRenderer          *s_Renderer;
    static IGPUMemoryManager  *s_GPUMemoryManager;
    static IGPUProgramManager *s_GPUProgramManager;

    friend class Application;
};
