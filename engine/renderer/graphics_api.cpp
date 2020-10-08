//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "renderer/graphics_api.h"

#include "platform/d3d12_renderer.h"
#include "platform/d3d12_gpu_memory_manager.h"

GraphicsAPI::API   GraphicsAPI::s_API              = GraphicsAPI::API::NONE;
IRenderer         *GraphicsAPI::s_Renderer         = null;
IGPUMemoryManager *GraphicsAPI::s_GPUMemoryManager = null;

void GraphicsAPI::SetGraphicsAPI(in API api)
{
    //@TODO(Roman): Ability to change API with recreating all the GPU stuff.
    CheckM(s_API == API::NONE, "Currently, switching Graphics API to another in run-time is not supported. So you can set it only once for now.");
    s_API = api;
}

IRenderer *GraphicsAPI::CreateRenderer(in Window *window, in const Logger& logger)
{
    switch (s_API)
    {
        case API::D3D12:
        {
            local D3D12::Renderer *renderer = null;
            CheckM(!renderer, "Renderer is already created. Use GraphicsAPI::GetRenderer() function instead");
            renderer = new D3D12::Renderer(window, logger);
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
}

IGPUMemoryManager *GraphicsAPI::CreateGPUMemoryManager(in Allocator *allocator, in const Logger& logger, in u64 gpu_memory_capacity)
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
