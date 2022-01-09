// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/memory_manager.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_memory_manager.h"

namespace REV::GPU
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

void *GetTexturePixelData(const TextureData *texture_data, u64 plane, u64 subtexture, u64 mip_level, TEXTURE_FORMAT format, Math::v2u uv)
{
    REV_CHECK(plane      < texture_data->planes_count);
    REV_CHECK(subtexture < texture_data->subtextures_count);
    REV_CHECK(mip_level  < texture_data->mip_levels_count);

    const TextureSurface *surface = texture_data->surfaces
                                  + plane      * texture_data->mip_levels_count * texture_data->subtextures_count
                                  + subtexture * texture_data->mip_levels_count
                                  + mip_level;

    u32 bytes_per_pixel = GetBytesPerPixel(format);

    u32 width_in_pixels  = cast(u32, surface->row_bytes     / bytes_per_pixel);
    u32 height_in_pixels = cast(u32, surface->size_in_bytes / surface->row_bytes);

    if (width_in_pixels)  --width_in_pixels;
    if (height_in_pixels) --height_in_pixels;

    Math::v2u clamped_uv = Math::v2u::clamp(uv, Math::v2u(), Math::v2u(width_in_pixels, height_in_pixels));

    return surface->data
         + clamped_uv.y * surface->row_bytes
         + clamped_uv.x * bytes_per_pixel;
}

void InterateTextureSurfaces(TextureData *texture_data, const Function<bool(TextureSurface *surface, u64 mip_level, u64 subtexture, u64 plane)>& SurfaceCallback)
{
    for (u64 i = 0; i < texture_data->surfaces_count; i++)
    {
        TextureSurface *surface          = texture_data->surfaces + i;
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

struct SurfaceInfo
{
    u64 bpp_or_bpe   = 0;
    u64 planes_count = 0;

    u64 (*GetRowBytes)(u64 width, u64 bpp_bpe)       = null;
    u64 (*GetSizeInBytes)(u64 height, u64 row_bytes) = null;
};

REV_INTERNAL REV_INLINE u64 GetRowBytesBC(u64 width, u64 block_size)               { return Math::max(1ui64, (width + 3) / 4) * block_size; }
REV_INTERNAL REV_INLINE u64 GetRowBytesPacked(u64 width, u64 bytes_per_element)    { return ((width + 1) >> 1) * bytes_per_element; }
REV_INTERNAL REV_INLINE u64 GetRowBytesPlanar(u64 width, u64 bytes_per_element)    { return ((width + 1) >> 1) * bytes_per_element; }
REV_INTERNAL REV_INLINE u64 GetRowBytesNV11(u64 width, u64)                        { return ((width + 3) >> 2) * 4; }
REV_INTERNAL REV_INLINE u64 GetRowBytesUncompressed(u64 width, u64 bits_per_pixel) { return (width * bits_per_pixel + 7) / 8; }

REV_INTERNAL REV_INLINE u64 GetSizeInBytesBC(u64 height, u64 row_bytes)           { return Math::max(1ui64, (height + 3) / 4) * row_bytes;  }
REV_INTERNAL REV_INLINE u64 GetSizeInBytesPacked(u64 height, u64 row_bytes)       { return height * row_bytes; }
REV_INTERNAL REV_INLINE u64 GetSizeInBytesPlanar(u64 height, u64 row_bytes)       { return height * row_bytes + ((height * row_bytes + 1) >> 1); }
REV_INTERNAL REV_INLINE u64 GetSizeInBytesNV11(u64 height, u64 row_bytes)         { return 2*height * row_bytes; }
REV_INTERNAL REV_INLINE u64 GetSizeInBytesUncompressed(u64 height, u64 row_bytes) { return height * row_bytes; }

REV_INTERNAL void GetSurfaceInfo(TEXTURE_FORMAT format, SurfaceInfo *surface_info)
{
    surface_info->planes_count = GraphicsAPI::GetDeviceContext()->GetFormatPlanesCount(format);

    switch (format)
    {
        case TEXTURE_FORMAT_S32:
        case TEXTURE_FORMAT_U32:
        case TEXTURE_FORMAT_RGBA8:
        case TEXTURE_FORMAT_BGRA8:
        {
            REV_CHECK_M(surface_info->planes_count == 1, "This is not a planar data type");
            surface_info->bpp_or_bpe     = 32;
            surface_info->GetRowBytes    = GetRowBytesUncompressed;
            surface_info->GetSizeInBytes = GetSizeInBytesUncompressed;
        } break;

        case TEXTURE_FORMAT_BC1:
        {
            REV_CHECK_M(surface_info->planes_count == 1, "This is not a planar data type");
            surface_info->bpp_or_bpe     = 8;
            surface_info->GetRowBytes    = GetRowBytesBC;
            surface_info->GetSizeInBytes = GetSizeInBytesBC;
        } break;

        case TEXTURE_FORMAT_BC2:
        case TEXTURE_FORMAT_BC3:
        {
            REV_CHECK_M(surface_info->planes_count == 1, "This is not a planar data type");
            surface_info->bpp_or_bpe     = 16;
            surface_info->GetRowBytes    = GetRowBytesBC;
            surface_info->GetSizeInBytes = GetSizeInBytesBC;
        } break;

        case TEXTURE_FORMAT_YUV422_8:
        {
            REV_CHECK_M(surface_info->planes_count == 1, "This is not a planar data type");
            surface_info->bpp_or_bpe     = 4;
            surface_info->GetRowBytes    = GetRowBytesPacked;
            surface_info->GetSizeInBytes = GetSizeInBytesPacked;
        } break;

        case TEXTURE_FORMAT_YUV422_10:
        case TEXTURE_FORMAT_YUV422_16:
        {
            REV_CHECK_M(surface_info->planes_count == 1, "This is not a planar data type");
            surface_info->bpp_or_bpe     = 8;
            surface_info->GetRowBytes    = GetRowBytesPacked;
            surface_info->GetSizeInBytes = GetSizeInBytesPacked;
        } break;

        case TEXTURE_FORMAT_YUV411_8:
        {
            REV_CHECK_M(surface_info->planes_count > 1, "This is a planar data type");
            surface_info->bpp_or_bpe     = 4;
            surface_info->GetRowBytes    = GetRowBytesNV11;
            surface_info->GetSizeInBytes = GetSizeInBytesNV11;
        } break;

        case TEXTURE_FORMAT_YUV420_8:
        {
            REV_CHECK_M(surface_info->planes_count > 1, "This is a planar data type");
            surface_info->bpp_or_bpe     = 2;
            surface_info->GetRowBytes    = GetRowBytesPlanar;
            surface_info->GetSizeInBytes = GetSizeInBytesPlanar;
        } break;

        case TEXTURE_FORMAT_YUV420_10:
        case TEXTURE_FORMAT_YUV420_16:
        {
            REV_CHECK_M(surface_info->planes_count > 1, "This is a planar data type");
            surface_info->bpp_or_bpe     = 4;
            surface_info->GetRowBytes    = GetRowBytesPlanar;
            surface_info->GetSizeInBytes = GetSizeInBytesPlanar;
        } break;

        default:
        {
            REV_ERROR_M("Invalid or unsupported TEXTURE_FORMAT: 0x%X", format);
        } break;
    }
}

TextureData *CreateTextureData(
    const byte     *surfaces,
    u64             mip_levels_count,
    u64             subtextures_count,
    u64             first_mip_level_width,
    u64             first_mip_level_height,
    TEXTURE_FORMAT  format)
{
    SurfaceInfo surface_info;
    GetSurfaceInfo(format, &surface_info);

    u64 surfaces_count    = mip_levels_count * subtextures_count * surface_info.planes_count;
    u64 bytes_to_allocate = REV_StructFieldOffset(TextureData, surfaces) + sizeof(TextureSurface) * surfaces_count;

    TextureData *texture_data       = cast(TextureData *, Memory::Get()->PushToFrameArena(bytes_to_allocate));
    texture_data->planes_count      = surface_info.planes_count;
    texture_data->subtextures_count = subtextures_count;
    texture_data->mip_levels_count  = mip_levels_count;
    texture_data->surfaces_count    = surfaces_count;

    const byte *surface_it = surfaces;

    for (u64 plane = 0; plane < texture_data->planes_count; ++plane)
    {
        for (u64 subtexture = 0; subtexture < texture_data->subtextures_count; ++subtexture)
        {
            u64 width  = first_mip_level_width;
            u64 height = first_mip_level_height;

            for (u64 mip_level = 0; mip_level < texture_data->mip_levels_count; ++mip_level)
            {
                TextureSurface *surface = texture_data->surfaces
                                        + plane      * texture_data->mip_levels_count * texture_data->subtextures_count
                                        + subtexture * texture_data->mip_levels_count
                                        + mip_level;

                surface->data          = cast(byte *, surface_it);
                surface->row_bytes     = surface_info.GetRowBytes(width, surface_info.bpp_or_bpe);
                surface->size_in_bytes = surface_info.GetSizeInBytes(height, surface->row_bytes);

                width  >>= 1;
                height >>= 1;

                if (width  == 0) width  = 1;
                if (height == 0) height = 1;

                surface_it += surface->size_in_bytes;
            }
        }
    }

    return texture_data;
}

TextureData *CreateVolumeTextureData(
    const byte     *surfaces,
    u64             mip_levels_count,
    u64             first_mip_level_width,
    u64             first_mip_level_height,
    u64             first_mip_level_depth,
    TEXTURE_FORMAT  format)
{
    SurfaceInfo surface_info;
    GetSurfaceInfo(format, &surface_info);

    u64 surfaces_count    = mip_levels_count * surface_info.planes_count;
    u64 bytes_to_allocate = REV_StructFieldOffset(TextureData, surfaces) + sizeof(TextureSurface) * surfaces_count;

    TextureData *texture_data       = cast(TextureData *, Memory::Get()->PushToFrameArena(bytes_to_allocate));
    texture_data->planes_count      = surface_info.planes_count;
    texture_data->subtextures_count = 1;
    texture_data->mip_levels_count  = mip_levels_count;
    texture_data->surfaces_count    = surfaces_count;

    const byte *surface_it = surfaces;

    for (u64 plane = 0; plane < texture_data->planes_count; ++plane)
    {
        u64 width  = first_mip_level_width;
        u64 height = first_mip_level_height;
        u64 depth  = first_mip_level_depth;

        for (u64 mip_level = 0; mip_level < texture_data->mip_levels_count; ++mip_level)
        {
            TextureSurface *surface = texture_data->surfaces
                                    + plane * texture_data->mip_levels_count
                                    + mip_level;

            surface->data          = cast(byte *, surface_it);
            surface->row_bytes     = surface_info.GetRowBytes(width, surface_info.bpp_or_bpe);
            surface->size_in_bytes = surface_info.GetSizeInBytes(height, surface->row_bytes);

            width  >>= 1;
            height >>= 1;
            depth  >>= 1;

            if (width  == 0) width  = 1;
            if (height == 0) height = 1;
            if (depth  == 0) depth  = 1;

            surface_it += surface->size_in_bytes;
        }
    }

    return texture_data;
}

//
// MemoryManager
//

ResourceHandle MemoryManager::AllocateVertexBuffer(u32 count, bool _static, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_VERTEX_BUFFER;
    handle.flags = RESOURCE_FLAG_CPU_WRITE;
    if (_static) handle.flags |= RESOURCE_FLAG_STATIC;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateVertexBuffer(count, handle.flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

    return handle;
}

ResourceHandle MemoryManager::AllocateIndexBuffer(u32 count, bool _static, const ConstString& name)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_INDEX_BUFFER;
    handle.flags = RESOURCE_FLAG_CPU_WRITE;
    if (_static) handle.flags |= RESOURCE_FLAG_STATIC;

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            handle.index = cast(D3D12::MemoryManager *, platform)->AllocateIndexBuffer(count, handle.flags, name);
        } break;

        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }

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

    return handle;
}

ResourceHandle MemoryManager::AllocateSampler(TEXTURE_ADDRESS_MODE address_mode, Math::v4 border_color, Math::v2 min_max_lod, bool _static)
{
    ResourceHandle handle;
    handle.kind  = RESOURCE_KIND_SAMPLER;
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

    return handle;
}

void MemoryManager::LoadResources(const ConstArray<GPU::ResourceHandle>& resources)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->LoadResources(resources);
        } break;
        
        case GraphicsAPI::API::VULKAN:
        {
        } break;
    }
}

void MemoryManager::StoreResources(const ConstArray<GPU::ResourceHandle>& resources)
{
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:
        {
            cast(D3D12::MemoryManager *, platform)->StoreResources(resources);
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
