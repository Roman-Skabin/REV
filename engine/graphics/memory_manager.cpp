//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/memory_manager.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_memory_manager.h"

namespace GPU
{

ResourceHandle MemoryManager::AllocateVertexBuffer(u32 vertex_count, u32 vertex_stride, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return ResourceHandle(cast<D3D12::MemoryManager *>(platform)->AllocateVertexBuffer(vertex_count, vertex_stride, name), RESOURCE_KIND::VB);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return ResourceHandle(U64_MAX, RESOURCE_KIND::VB);
}

ResourceHandle MemoryManager::AllocateIndexBuffer(u32 index_count, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return ResourceHandle(cast<D3D12::MemoryManager *>(platform)->AllocateIndexBuffer(index_count, name), RESOURCE_KIND::IB);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return ResourceHandle(U64_MAX, RESOURCE_KIND::IB);
}

ResourceHandle MemoryManager::AllocateConstantBuffer(u32 bytes, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return ResourceHandle(cast<D3D12::MemoryManager *>(platform)->AllocateConstantBuffer(bytes, name), RESOURCE_KIND::CB);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return ResourceHandle(U64_MAX, RESOURCE_KIND::CB);
}

void MemoryManager::SetResoucreData(ResourceHandle resource, const void *data)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::MemoryManager *memory_manager = cast<D3D12::MemoryManager *>(platform);

            switch (resource.kind)
            {
                case RESOURCE_KIND::VB:
                case RESOURCE_KIND::IB:
                case RESOURCE_KIND::CB:
                {
                    memory_manager->SetBufferData(memory_manager->GetBuffer(resource.index), data);
                } break;

                case RESOURCE_KIND::SR:
                {
                } break;

                case RESOURCE_KIND::UA:
                {
                } break;
            }
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::SetResourceDataImmediate(ResourceHandle resource, const void *data)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::MemoryManager *memory_manager = cast<D3D12::MemoryManager *>(platform);

            switch (resource.kind)
            {
                case RESOURCE_KIND::VB:
                case RESOURCE_KIND::IB:
                case RESOURCE_KIND::CB:
                {
                    memory_manager->SetBufferDataImmediate(memory_manager->GetBuffer(resource.index), data);
                } break;

                case RESOURCE_KIND::SR:
                {
                } break;

                case RESOURCE_KIND::UA:
                {
                } break;
            }
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::StartImmediateExecution()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast<D3D12::MemoryManager *>(platform)->StartImmediateExecution();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::EndImmediateExecution()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast<D3D12::MemoryManager *>(platform)->EndImmediateExecution();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::FreeMemory()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast<D3D12::MemoryManager *>(platform)->FreeMemory();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

}
