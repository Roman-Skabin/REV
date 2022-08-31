// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/graphics_api.h"
#include "memory/memory.h"

#include "platform/d3d12/d3d12_device_context.h"
#include "platform/d3d12/d3d12_memory_manager.h"
#include "platform/d3d12/d3d12_shader_manager.h"

namespace REV
{

GraphicsAPI::API  GraphicsAPI::s_API           = GraphicsAPI::API::NONE;
DeviceContext    *GraphicsAPI::s_DeviceContext = null;
MemoryManager    *GraphicsAPI::s_MemoryManager = null;
ShaderManager    *GraphicsAPI::s_ShaderManager = null;

void GraphicsAPI::SetGraphicsAPI(API api)
{
    // @TODO(Roman): Ability to change API with recreating all the GPU stuff.
    REV_CHECK_M(s_API == NONE, "Currently, switching Graphics API to another at run-time is not supported. So you can set it only once for now.");
    s_API = api;
}

DeviceContext *GraphicsAPI::GetDeviceContext()
{
    REV_CHECK_M(s_DeviceContext, "DeviceContext is not created yet");
    return s_DeviceContext;
}

MemoryManager *GraphicsAPI::GetMemoryManager()
{
    REV_CHECK_M(s_MemoryManager, "GPU Memory Manager is not created yet");
    return s_MemoryManager;
}

ShaderManager *GraphicsAPI::GetShaderManager()
{
    REV_CHECK_M(s_ShaderManager, "GPU Shader Manager is not created yet");
    return s_ShaderManager;
}

void GraphicsAPI::Init(Window *window, Allocator *allocator, const Logger& logger)
{
    switch (s_API)
    {
        case D3D12:
        {
            REV_LOCAL bool d3d12_initialized = false;
            REV_CHECK_M(!d3d12_initialized, "Graphics API for D3D12 is already created.");

            byte *d3d12_memory = Memory::Get()->PushToPA<byte>(sizeof(D3D12::DeviceContext)
                                                             + sizeof(D3D12::MemoryManager)
                                                             + sizeof(D3D12::ShaderManager));

            byte *device_context_area = d3d12_memory;
            byte *memory_manager_area = d3d12_memory + sizeof(D3D12::DeviceContext);
            byte *shader_manager_area = d3d12_memory + sizeof(D3D12::DeviceContext) + sizeof(D3D12::MemoryManager);

            s_DeviceContext = cast(DeviceContext *, new (device_context_area) D3D12::DeviceContext(window, logger));
            s_MemoryManager = cast(MemoryManager *, new (memory_manager_area) D3D12::MemoryManager(allocator));
            s_ShaderManager = cast(ShaderManager *, new (shader_manager_area) D3D12::ShaderManager(allocator, logger));

            d3d12_initialized = true;
        } break;

        case VULKAN:
        {
            REV_ERROR_M("Vulkan API is not supported yet.");
        } break;

        default:
        {
            REV_ERROR_M("Unknown graphics API.");
        } break;
    }
}

void GraphicsAPI::Destroy()
{
    switch (s_API)
    {
        case D3D12:
        {
            // @Important(Roman): DeviceContext's destruction must be the last one
            cast(D3D12::ShaderManager *, s_ShaderManager)->~ShaderManager();
            cast(D3D12::MemoryManager *, s_MemoryManager)->~MemoryManager();
            cast(D3D12::DeviceContext *, s_DeviceContext)->~DeviceContext();
        } break;

        case VULKAN:
        {
            REV_ERROR_M("Vulkan API is not supported yet.");
        } break;

        default:
        {
            REV_ERROR_M("Unknown graphics API.");
        } break;
    }
}

}
