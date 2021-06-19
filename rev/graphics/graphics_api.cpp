//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/graphics_api.h"

#include "platform/d3d12/d3d12_renderer.h"
#include "platform/d3d12/d3d12_memory_manager.h"
#include "platform/d3d12/d3d12_program_manager.h"

namespace REV
{

GraphicsAPI::API     GraphicsAPI::s_API            = GraphicsAPI::API::NONE;
GPU::Renderer       *GraphicsAPI::s_Renderer       = null;
GPU::MemoryManager  *GraphicsAPI::s_MemoryManager  = null;
GPU::ProgramManager *GraphicsAPI::s_ProgramManager = null;

void GraphicsAPI::SetGraphicsAPI(API api)
{
    //@TODO(Roman): Ability to change API with recreating all the GPU stuff.
    REV_CHECK_M(s_API == API::NONE, "Currently, switching Graphics API to another at run-time is not supported. So you can set it only once for now.");
    s_API = api;
}

GPU::Renderer *GraphicsAPI::GetRenderer()
{
    REV_CHECK_M(s_Renderer, "Renderer is not created yet");
    return s_Renderer;
}

GPU::MemoryManager *GraphicsAPI::GetMemoryManager()
{
    REV_CHECK_M(s_MemoryManager, "GPU Memory Manager is not created yet");
    return s_MemoryManager;
}

GPU::ProgramManager *GraphicsAPI::GetProgramManager()
{
    REV_CHECK_M(s_ProgramManager, "GPU Program Manager is not created yet");
    return s_ProgramManager;
}

void GraphicsAPI::Init(Window *window, Allocator *allocator, const Logger& logger)
{
    switch (s_API)
    {
        case API::D3D12:
        {
            REV_LOCAL bool d3d12_initialized = false;
            REV_CHECK_M(!d3d12_initialized, "Graphics API for D3D12 is already created.");

            byte *d3d12_area = Memory::Get()->PushToPA<byte>(sizeof(D3D12::Renderer)
                                                           + sizeof(D3D12::MemoryManager)
                                                           + sizeof(D3D12::ProgramManager));

            byte *renderer_area        = d3d12_area;
            byte *memory_manager_area  = renderer_area       + sizeof(D3D12::Renderer);
            byte *program_manager_area = memory_manager_area + sizeof(D3D12::MemoryManager);

            s_Renderer       = cast<GPU::Renderer       *>(new (renderer_area)        D3D12::Renderer(window, logger));
            s_MemoryManager  = cast<GPU::MemoryManager  *>(new (memory_manager_area)  D3D12::MemoryManager(allocator));
            s_ProgramManager = cast<GPU::ProgramManager *>(new (program_manager_area) D3D12::ProgramManager(allocator, logger));

            d3d12_initialized = true;
        } break;

        case API::VULKAN:
        {
            REV_FAILED_M("Vulkan API is not supported yet.");
        } break;

        default:
        {
            REV_FAILED_M("Unknown graphics API.");
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
            REV_FAILED_M("Vulkan API is not supported yet.");
        } break;

        default:
        {
            REV_FAILED_M("Unknown graphics API.");
        } break;
    }
}

}
