// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/device_context.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_device_context.h"

namespace REV::GPU
{

void DeviceContext::StartFrame()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::DeviceContext *, platform)->StartFrame();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void DeviceContext::EndFrame()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::DeviceContext *, platform)->EndFrame();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

bool DeviceContext::VSyncEnabled()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return cast(D3D12::DeviceContext *, platform)->VSyncEnabled();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return false;
}

void DeviceContext::SetVSync(bool enable)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::DeviceContext *, platform)->SetVSync(enable);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void DeviceContext::WaitForGPU()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::DeviceContext *, platform)->WaitForGPU();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

bool DeviceContext::FrameStarted()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return cast(D3D12::DeviceContext *, platform)->FrameStarted();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return false;
}

void DeviceContext::SetFullscreenMode(bool set)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::DeviceContext *, platform)->SetFullscreenMode(set);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

}
