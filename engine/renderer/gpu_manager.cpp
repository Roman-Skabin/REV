//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/pch.h"
#include "renderer/gpu_manager.h"
#include "platform/d3d12/d3d12_gpu_manager.h"

global GRAPHICS_API     gAPI             = GRAPHICS_API::NONE;
global D3D12GPUManager *gD3D12GPUManager = null;

IGPUManager *CreateGPUManager(
    in GRAPHICS_API  api,
    in Window       *window,
    in const Logger& logger)
{
    gAPI = api;

    switch (gAPI)
    {
        case GRAPHICS_API::D3D12:
        {
            gD3D12GPUManager  = Memory::Get()->PushToPA<D3D12GPUManager>();
            *gD3D12GPUManager = D3D12GPUManager(window, logger);
            return gD3D12GPUManager;
        } break;

        case GRAPHICS_API::VULKAN:
        {
            FailedM("Vulkan is not supported yet");

            if (gD3D12GPUManager)
            {
                gD3D12GPUManager->Destroy();
                gD3D12GPUManager = null;
            }
        } break;

        default:
        {
            FailedM("Unknown Graphics API");
        } break;
    }
}

ENGINE_FUN IGPUManager *GetGPUManager()
{
    switch (gAPI)
    {
        case GRAPHICS_API::D3D12:
        {
            CheckM(gD3D12GPUManager, "GPU Manager is not created yet");
            return gD3D12GPUManager;
        } break;

        case GRAPHICS_API::VULKAN:
        {
            FailedM("Vulkan is not supported yet");
        } break;

        default:
        {
            FailedM("Unknown Graphics API");
        } break;
    }
}
