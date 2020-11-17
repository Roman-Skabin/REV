//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "renderer/graphics_api.h"

#include "platform/d3d12_renderer.h"
#include "platform/d3d12_gpu_memory_manager.h"
#include "platform/d3d12_gpu_program_manager.h"

GraphicsAPI::API    GraphicsAPI::s_API               = GraphicsAPI::API::NONE;
IRenderer          *GraphicsAPI::s_Renderer          = null;
IGPUMemoryManager  *GraphicsAPI::s_GPUMemoryManager  = null;
IGPUProgramManager *GraphicsAPI::s_GPUProgramManager = null;

void GraphicsAPI::SetGraphicsAPI(API api)
{
    //@TODO(Roman): Ability to change API with recreating all the GPU stuff.
    CheckM(s_API == API::NONE, "Currently, switching Graphics API to another run-time is not supported. So you can set it only once for now.");
    s_API = api;
}

IRenderer *GraphicsAPI::CreateRenderer(Window *window, const Logger& logger, Math::v2s rt_size)
{
    switch (s_API)
    {
        case API::D3D12:
        {
            local D3D12::Renderer *renderer = null;
            CheckM(!renderer, "Renderer is already created. Use GraphicsAPI::GetRenderer() function instead");
            renderer = new D3D12::Renderer(window, logger, rt_size);
            s_Renderer = renderer;
            return s_Renderer;
        } break;

        case API::VULKAN:
        {
            FailedM("Vulkan is not supported yet");
        } break;

        default:
        {
            FailedM("Unknown Graphics API");
        } break;
    }

    return null;
}

IGPUMemoryManager *GraphicsAPI::CreateGPUMemoryManager(Allocator *allocator, const Logger& logger, u64 gpu_memory_capacity)
{
    switch (s_API)
    {
        case API::D3D12:
        {
            local D3D12::GPUMemoryManager *manager = null;
            CheckM(!manager, "GPU Memory Manager is already created. Use GraphicsAPI::GetGPUMemoryManager() function instead");
            manager = new D3D12::GPUMemoryManager(allocator, logger, gpu_memory_capacity);
            s_GPUMemoryManager = manager;
            return s_GPUMemoryManager;
        } break;

        case API::VULKAN:
        {
            FailedM("Vulkan is not supported yet");
        } break;

        default:
        {
            FailedM("Unknown Graphics API");
        } break;
    }

    return null;
}

IGPUProgramManager *GraphicsAPI::CreateGPUProgramManager(Allocator *allocator)
{
    switch (s_API)
    {
        case API::D3D12:
        {
            local D3D12::GPUProgramManager *manager = null;
            CheckM(!manager, "GPU Program Manager is already created. Use GraphicsAPI::GetGPUProgramManager() function instead");
            manager = new D3D12::GPUProgramManager(allocator);
            s_GPUProgramManager = manager;
            return s_GPUProgramManager;
        } break;

        case API::VULKAN:
        {
            FailedM("Vulkan is not supported yet");
        } break;

        default:
        {
            FailedM("Unknown Graphics API");
        } break;
    }

    return null;
}

IRenderer *GraphicsAPI::GetRenderer()
{
    CheckM(s_Renderer, "Renderer is not created yet");
    return s_Renderer;
}

IGPUMemoryManager *GraphicsAPI::GetGPUMemoryManager()
{
    CheckM(s_GPUMemoryManager, "GPU Memory Manager is not created yet");
    return s_GPUMemoryManager;
}

IGPUProgramManager *GraphicsAPI::GetGPUProgramManager()
{
    CheckM(s_GPUProgramManager, "GPU Program Manager is not created yet");
    return s_GPUProgramManager;
}
