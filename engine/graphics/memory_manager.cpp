// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/memory_manager.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_memory_manager.h"

namespace REV
{

REV_INTERNAL REV_INLINE u32 GetBytesPerPixel(TEXTURE_FORMAT format)
{
    if (format == TEXTURE_FORMAT_S32
    ||  format == TEXTURE_FORMAT_U32
    ||  format == TEXTURE_FORMAT_RGBA8
    ||  format == TEXTURE_FORMAT_BGRA8)
    {
        return 4;
    }

    REV_ERROR_M("This is compressed format so we can not get bytes per pixel");
    return 0;
}

void InterateTextureSurfaces(TextureData *texture_data, const Function<bool(TextureSurface *surface, u64 mip_level, u64 subtexture, u64 plane)>& SurfaceCallback)
{
    for (u64 i = 0; i < texture_data->surfaces_count; i++)
    {
        TextureSurface *surface    = texture_data->surfaces + i;
        u64             plane      = i / (texture_data->mip_levels_count * texture_data->subtextures_count);
        u64             subtexture = i % (texture_data->mip_levels_count * texture_data->subtextures_count) / texture_data->mip_levels_count;
        u64             mip_level  = i % (texture_data->mip_levels_count * texture_data->subtextures_count) % texture_data->mip_levels_count;
        if (!SurfaceCallback(surface, mip_level, subtexture, plane))
        {
            break;
        }
    }
}

void InterateVolumeTextureSurfaces(TextureData *texture_data, const Function<bool(TextureSurface *surface, u64 mip_level)>& SurfaceCallback)
{
    for (u64 i = 0; i < texture_data->surfaces_count; i++)
    {
        TextureSurface *surface = texture_data->surfaces + i;
        if (!SurfaceCallback(surface, i))
        {
            break;
        }
    }
}

//
// MemoryManager
//

ResourceHandle MemoryManager::AllocateVertexBuffer(u32 count, u32 stride, bool _static, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_VERTEX_BUFFER;
    handle.flags = RESOURCE_FLAG_CPU_WRITE;
    if (_static) handle.flags |= RESOURCE_FLAG_STATIC;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateVertexBuffer(count, stride, handle.flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateIndexBuffer(u32 count, u32 stride, bool _static, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_INDEX_BUFFER;
    handle.flags = RESOURCE_FLAG_CPU_WRITE;
    if (_static) handle.flags |= RESOURCE_FLAG_STATIC;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateIndexBuffer(count, stride, handle.flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateConstantBuffer(u32 bytes, bool _static, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_CONSTANT_BUFFER;
    handle.flags = RESOURCE_FLAG_CPU_WRITE;
    if (_static) handle.flags |= RESOURCE_FLAG_STATIC;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateConstantBuffer(bytes, handle.flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateBuffer(u32 bytes, u32 stride, BUFFER_FORMAT format, RESOURCE_FLAG flags, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_BUFFER;
    handle.flags = flags;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            if (format == BUFFER_FORMAT_UNKNOWN)
            {
                if (stride) handle.index = cast(D3D12::MemoryManager *, platform)->AllocateStructuredBuffer(bytes / stride, stride, handle.flags, name);
                else        handle.index = cast(D3D12::MemoryManager *, platform)->AllocateByteAddressBuffer(bytes, handle.flags, name);
            }
            else
            {
                handle.index = cast(D3D12::MemoryManager *, platform)->AllocateBuffer(bytes, format, handle.flags, name);
            }
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateTexture1D(u16 width, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_TEXTURE;
    handle.flags = flags;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateTexture1D(width, mip_levels, format, flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateTexture2D(u16 width, u16 height, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_TEXTURE;
    handle.flags = flags;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateTexture2D(width, height, mip_levels, format, flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateTexture3D(u16 width, u16 height, u16 depth, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_TEXTURE;
    handle.flags = flags;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateTexture3D(width, height, depth, mip_levels, format, flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateTextureCube(u16 width, u16 height, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_TEXTURE;
    handle.flags = flags;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateTextureCube(width, height, mip_levels, format, flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateTexture1DArray(u16 width, u16 count, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_TEXTURE;
    handle.flags = flags;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateTexture1DArray(width, count, mip_levels, format, flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateTexture2DArray(u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_TEXTURE;
    handle.flags = flags;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateTexture2DArray(width, height, count, mip_levels, format, flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_TEXTURE;
    handle.flags = flags;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateTextureCubeArray(width, height, count, mip_levels, format, flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

ResourceHandle MemoryManager::AllocateSampler(TEXTURE_ADDRESS_MODE address_mode, Math::v4 border_color, Math::v2 min_max_lod, bool _static)
{
    ResourceHandle handle;
    handle.kind = RESOURCE_KIND_SAMPLER;
    if (_static) handle.flags = RESOURCE_FLAG_STATIC;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateSampler(address_mode, border_color, min_max_lod, _static);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    REV_CHECK(handle);
    return handle;
}

void MemoryManager::UploadResources(const ConstArray<ResourceHandle>& resources)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->UploadResources(resources);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::ReadbackResources(const ConstArray<ResourceHandle>& resources)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->ReadbackResources(resources);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::ResizeRenderTarget(ResourceHandle resource, u16 width, u16 height, u16 depth)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->ResizeRenderTarget(resource, width, height, depth);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

const void *MemoryManager::GetBufferData(const ResourceHandle& resource) const
{
    const void *data = null;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            data = cast(D3D12::MemoryManager *, platform)->GetBufferData(resource);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    return data;
}

void MemoryManager::SetBufferData(const ResourceHandle& resource, const void *data)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->SetBufferData(resource, data);
        } break;
        
        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

TextureData *MemoryManager::GetTextureData(const ResourceHandle& resource) const
{
    TextureData *data = null;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            data = cast(D3D12::MemoryManager *, platform)->GetTextureData(resource);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    return data;
}

void MemoryManager::SetTextureData(const ResourceHandle& resource, const TextureData *data)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->SetTextureData(resource, data);
        } break;
        
        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::FreeSceneMemory()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->FreeSceneMemory();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::FreeStaticMemory()
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->FreeStaticMemory();
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

ConstString MemoryManager::GetResourceName(const ResourceHandle& resource)
{
    ConstString name = null;
    
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            name = cast(D3D12::MemoryManager *, platform)->GetResourceName();
        } break;
        
        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    
    return name;
}

ConstString MemoryManager::GetBufferName(const ResourceHandle& resource)
{
    ConstString name = null;
    
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            name = cast(D3D12::MemoryManager *, platform)->GetBufferName();
        } break;
        
        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    
    return name;
}

ConstString MemoryManager::GetTextureName(const ResourceHandle& resource)
{
    ConstString name = null;
    
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            name = cast(D3D12::MemoryManager *, platform)->GetTextureName();
        } break;
        
        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
    
    return name;
}

}
