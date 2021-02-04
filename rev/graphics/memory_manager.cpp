//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/memory_manager.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_memory_manager.h"

namespace REV::GPU
{

REV_INTERNAL DXGI_FORMAT REVToDXGITextureFormat(TEXTURE_FORMAT format)
{
    switch (format)
    {
        case TEXTURE_FORMAT::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TEXTURE_FORMAT::BGRA8: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case TEXTURE_FORMAT::DXT1:  return DXGI_FORMAT_BC1_UNORM;
        case TEXTURE_FORMAT::DXT2:
        case TEXTURE_FORMAT::DXT3:  return DXGI_FORMAT_BC2_UNORM;
        case TEXTURE_FORMAT::DXT4:
        case TEXTURE_FORMAT::DXT5:  return DXGI_FORMAT_BC3_UNORM;
    }

    if (cast<bool>(format & TEXTURE_FORMAT::_DDS_DX10))
    {
        return cast<DXGI_FORMAT>(format & ~TEXTURE_FORMAT::_DDS_DX10);
    }
    else
    {
        REV_FAILED_M("Unknown TEXTURE_FORMAT");
        return DXGI_FORMAT_UNKNOWN;
    }
}

ResourceHandle MemoryManager::AllocateVertexBuffer(u32 vertex_count, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateVertexBuffer(vertex_count, name), RESOURCE_KIND::VB };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::VB };
}

ResourceHandle MemoryManager::AllocateIndexBuffer(u32 index_count, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateIndexBuffer(index_count, name), RESOURCE_KIND::IB };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::IB };
}

ResourceHandle MemoryManager::AllocateConstantBuffer(u32 bytes, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateConstantBuffer(bytes, name), RESOURCE_KIND::CB };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::CB };
}

ResourceHandle MemoryManager::AllocateTexture1D(u16 width, TEXTURE_FORMAT texture_format, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateTexture1D(width, REVToDXGITextureFormat(texture_format), name), RESOURCE_KIND::SR };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::SR };
}

ResourceHandle MemoryManager::AllocateTexture2D(u16 width, u16 height, u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateTexture2D(width, height, mip_levels, REVToDXGITextureFormat(texture_format), name), RESOURCE_KIND::SR };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::SR };
}

ResourceHandle MemoryManager::AllocateTexture3D(u16 width, u16 height, u16 depth, u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateTexture3D(width, height, depth, mip_levels, REVToDXGITextureFormat(texture_format), name), RESOURCE_KIND::SR };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::SR };
}

ResourceHandle MemoryManager::AllocateTextureCube(u16 width, u16 height, u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateTextureCube(width, height, mip_levels, REVToDXGITextureFormat(texture_format), name), RESOURCE_KIND::SR };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::SR };
}

ResourceHandle MemoryManager::AllocateTexture1DArray(u16 width, u16 count, TEXTURE_FORMAT texture_format, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateTexture1DArray(width, count, REVToDXGITextureFormat(texture_format), name), RESOURCE_KIND::SR };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::SR };
}

ResourceHandle MemoryManager::AllocateTexture2DArray(u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateTexture2DArray(width, height, count, mip_levels, REVToDXGITextureFormat(texture_format), name), RESOURCE_KIND::SR };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::SR };
}

ResourceHandle MemoryManager::AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            return { cast<D3D12::MemoryManager *>(platform)->AllocateTextureCubeArray(width, height, count, mip_levels, REVToDXGITextureFormat(texture_format), name), RESOURCE_KIND::SR };
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    return { REV_U64_MAX, RESOURCE_KIND::SR };
}

void MemoryManager::SetBufferData(ResourceHandle resource, const void *data)
{
    REV_CHECK_M(   resource.kind == RESOURCE_KIND::VB
                || resource.kind == RESOURCE_KIND::IB
                || resource.kind == RESOURCE_KIND::CB,
                "Resource is not a buffer");

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::MemoryManager *memory_manager = cast<D3D12::MemoryManager *>(platform);
            memory_manager->SetBufferData(memory_manager->GetBuffer(resource.index), data);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::SetTextureData(ResourceHandle resource, TextureDesc *texture_desc)
{
    REV_CHECK_M(resource.kind == RESOURCE_KIND::SR, "Resource is not a texture");

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::MemoryManager *memory_manager = cast<D3D12::MemoryManager *>(platform);
            memory_manager->SetTextureData(&memory_manager->GetTexture(resource.index), texture_desc);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::SetBufferDataImmediately(ResourceHandle resource, const void *data)
{
    REV_CHECK_M(   resource.kind == RESOURCE_KIND::VB
                || resource.kind == RESOURCE_KIND::IB
                || resource.kind == RESOURCE_KIND::CB,
                "Resource is not a buffer");

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::MemoryManager *memory_manager = cast<D3D12::MemoryManager *>(platform);
            memory_manager->SetBufferDataImmediately(memory_manager->GetBuffer(resource.index), data);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::SetTextureDataImmediately(ResourceHandle resource, TextureDesc *texture_desc)
{
    REV_CHECK_M(resource.kind == RESOURCE_KIND::SR, "Resource is not a texture");

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            D3D12::MemoryManager *memory_manager = cast<D3D12::MemoryManager *>(platform);
            memory_manager->SetTextureDataImmediately(&memory_manager->GetTexture(resource.index), texture_desc);
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
