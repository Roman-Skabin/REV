//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/graphics_api.h"

#include "platform/d3d12/d3d12_renderer.h"
#include "platform/d3d12/d3d12_memory_manager.h"
#include "platform/d3d12/d3d12_program_manager.h"

GraphicsAPI::API     GraphicsAPI::s_API            = GraphicsAPI::API::NONE;
Renderer            *GraphicsAPI::s_Renderer       = null;
GPU::MemoryManager  *GraphicsAPI::s_MemoryManager  = null;
GPU::ProgramManager *GraphicsAPI::s_ProgramManager = null;

void GraphicsAPI::SetGraphicsAPI(API api)
{
    //@TODO(Roman): Ability to change API with recreating all the GPU stuff.
    CheckM(s_API == API::NONE, "Currently, switching Graphics API to another at run-time is not supported. So you can set it only once for now.");
    s_API = api;
}

Renderer *GraphicsAPI::GetRenderer()
{
    CheckM(s_Renderer, "Renderer is not created yet");
    return s_Renderer;
}

GPU::MemoryManager *GraphicsAPI::GetMemoryManager()
{
    CheckM(s_MemoryManager, "GPU Memory Manager is not created yet");
    return s_MemoryManager;
}

GPU::ProgramManager *GraphicsAPI::GetProgramManager()
{
    CheckM(s_ProgramManager, "GPU Program Manager is not created yet");
    return s_ProgramManager;
}

void GraphicsAPI::Init(Window *window, Allocator *allocator, const Logger& logger, Math::v2s rt_size)
{
    switch (s_API)
    {
        case API::D3D12:
        {
            local bool d3d12_initialized = false;
            CheckM(!d3d12_initialized, "Graphics API for D3D12 is already created.");

            s_Renderer = cast<Renderer *>(Memory::Get()->PushToPermanentArea(sizeof(D3D12::Renderer)
                                                                           + sizeof(D3D12::MemoryManager)
                                                                           + sizeof(D3D12::ProgramManager)));
            s_MemoryManager  = cast<GPU::MemoryManager *>(cast<byte *>(s_Renderer) + sizeof(D3D12::Renderer));
            s_ProgramManager = cast<GPU::ProgramManager *>(cast<byte *>(s_MemoryManager) + sizeof(D3D12::MemoryManager));

            *cast<D3D12::Renderer *>(s_Renderer->platform)             = D3D12::Renderer(window, logger, rt_size);
            *cast<D3D12::MemoryManager *>(s_MemoryManager->platform)   = D3D12::MemoryManager(allocator);
            *cast<D3D12::ProgramManager *>(s_ProgramManager->platform) = D3D12::ProgramManager(allocator, logger);

            d3d12_initialized = true;
        } break;

        case API::VULKAN:
        {
            FailedM("Vulkan API is not supported yet.");
        } break;

        default:
        {
            FailedM("Unknown graphics API.");
        } break;
    }
}

void GraphicsAPI::Destroy()
{
    switch (s_API)
    {
        case API::D3D12:
        {
            // @Important(Roman): Renderer's destruction must be the last one
            cast<D3D12::ProgramManager *>(s_ProgramManager->platform)->~ProgramManager();
            cast<D3D12::MemoryManager *>(s_MemoryManager->platform)->~MemoryManager();
            cast<D3D12::Renderer *>(s_Renderer->platform)->~Renderer();
        } break;

        case API::VULKAN:
        {
            FailedM("Vulkan API is not supported yet.");
        } break;

        default:
        {
            FailedM("Unknown graphics API.");
        } break;
    }
}
