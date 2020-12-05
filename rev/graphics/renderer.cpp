//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/renderer.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_renderer.h"

namespace REV
{

void Renderer::StartFrame()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast<D3D12::Renderer *>(platform)->StartFrame();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void Renderer::EndFrame()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast<D3D12::Renderer *>(platform)->EndFrame();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

bool Renderer::VSyncEnabled()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return cast<D3D12::Renderer *>(platform)->VSyncEnabled();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return false;
}

void Renderer::SetVSync(bool enable)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast<D3D12::Renderer *>(platform)->SetVSync(enable);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void Renderer::WaitForGPU()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast<D3D12::Renderer *>(platform)->WaitForGPU();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void Renderer::SetFullscreenMode(bool set)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast<D3D12::Renderer *>(platform)->SetFullscreenMode(set);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

}
