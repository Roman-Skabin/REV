// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/const_string.h"
#include "math/vec.h"
#include "tools/array.hpp"
#include "memory/arena.h"
#include "tools/function.hpp"

namespace REV { class GraphicsAPI; }

namespace REV::GPU
{
    enum RESOURCE_KIND : u32
    {
        RESOURCE_KIND_UNKNOWN = 0,

        RESOURCE_KIND_VERTEX_BUFFER,
        RESOURCE_KIND_INDEX_BUFFER,
        RESOURCE_KIND_CONSTANT_BUFFER, // @NOTE(Roman): D3D12 = CBV, Vulkan = uniform buffer
        RESOURCE_KIND_BUFFER,

        RESOURCE_KIND_TEXTURE,

        RESOURCE_KIND_SAMPLER,
    };

    enum RESOURCE_FLAG : u32
    {
        RESOURCE_FLAG_NONE                  = 0,
        RESOURCE_FLAG_STATIC                = 1 << 0,
        RESOURCE_FLAG_CPU_READ              = 1 << 1,
        RESOURCE_FLAG_CPU_WRITE             = 1 << 2,
        RESOURCE_FLAG_NONCOHERENT_GPU_WRITE = 1 << 3, // @Important(Roman): Do not combine with RESOURCE_FLAG_DEPTH_STENCIL
                                                      // @NOTE(Roman): D3D12 = UAV, Vulkan = storage buffer
        RESOURCE_FLAG_RENDER_TARGET         = 1 << 4, // @Important(Roman): Do not combine with RESOURCE_FLAG_DEPTH_STENCIL
        RESOURCE_FLAG_DEPTH_STENCIL         = 1 << 5, // @Important(Roman): Do not combine with RESOURCE_FLAG_NONCOHERENT_GPU_WRITE nor with RESOURCE_FLAG_RENDER_TARGET
    };
    REV_ENUM_OPERATORS(RESOURCE_FLAG);

    struct ResourceHandle final
    {
        u64           index = REV_INVALID_U64_INDEX;
        RESOURCE_KIND kind  = RESOURCE_KIND_UNKNOWN;
        RESOURCE_FLAG flags = RESOURCE_FLAG_NONE;

        REV_INLINE operator bool() const { return index != REV_INVALID_U64_INDEX; }
    };

    REV_INLINE bool operator==(const ResourceHandle& left, const ResourceHandle& right) { return left.index == right.index && left.kind == right.kind && left.flags == right.flags; }
    REV_INLINE bool operator!=(const ResourceHandle& left, const ResourceHandle& right) { return left.index != right.index || left.kind != right.kind || left.flags != right.flags; }

    enum BUFFER_FORMAT : u32
    {
        BUFFER_FORMAT_UNKNOWN = 0,

        BUFFER_FORMAT_BOOL,
        BUFFER_FORMAT_BOOLx1 = BUFFER_FORMAT_BOOL,
        BUFFER_FORMAT_BOOLx2,
    //  BUFFER_FORMAT_BOOLx3,
        BUFFER_FORMAT_BOOLx4,

        BUFFER_FORMAT_F16,
        BUFFER_FORMAT_F16x1 = BUFFER_FORMAT_F16,
        BUFFER_FORMAT_F16x2,
    //  BUFFER_FORMAT_F16x3,
        BUFFER_FORMAT_F16x4,

        BUFFER_FORMAT_F32,
        BUFFER_FORMAT_F32x1 = BUFFER_FORMAT_F32,
        BUFFER_FORMAT_F32x2,
        BUFFER_FORMAT_F32x3,
        BUFFER_FORMAT_F32x4,

        BUFFER_FORMAT_U32,
        BUFFER_FORMAT_U32x1 = BUFFER_FORMAT_U32,
        BUFFER_FORMAT_U32x2,
        BUFFER_FORMAT_U32x3,
        BUFFER_FORMAT_U32x4,

        BUFFER_FORMAT_S32,
        BUFFER_FORMAT_S32x1 = BUFFER_FORMAT_S32,
        BUFFER_FORMAT_S32x2,
        BUFFER_FORMAT_S32x3,
        BUFFER_FORMAT_S32x4,
    };

    enum TEXTURE_FORMAT : u32
    {
        TEXTURE_FORMAT_UNKNOWN = 0,

        // @NOTE(Roman): Uncompressed formats
        TEXTURE_FORMAT_S32,
        TEXTURE_FORMAT_U32,
        TEXTURE_FORMAT_RGBA8,
        TEXTURE_FORMAT_BGRA8,

        // @NOTE(Roman): Block compressed formats
        TEXTURE_FORMAT_BC1,
        TEXTURE_FORMAT_BC2,
        TEXTURE_FORMAT_BC3,

        // @NOTE(Roman): Packed formats
        TEXTURE_FORMAT_YUV422_8,
        TEXTURE_FORMAT_YUV422_10,
        TEXTURE_FORMAT_YUV422_16,

        TEXTURE_FORMAT_YUY2 = TEXTURE_FORMAT_YUV422_8,
        TEXTURE_FORMAT_Y210 = TEXTURE_FORMAT_YUV422_10,
        TEXTURE_FORMAT_Y216 = TEXTURE_FORMAT_YUV422_16,

        // @NOTE(Roman): Planar formats
        TEXTURE_FORMAT_YUV411_8,
        TEXTURE_FORMAT_YUV420_8,
        TEXTURE_FORMAT_YUV420_10,
        TEXTURE_FORMAT_YUV420_16,

        TEXTURE_FORMAT_NV11 = TEXTURE_FORMAT_YUV411_8,
        TEXTURE_FORMAT_NV12 = TEXTURE_FORMAT_YUV420_8,
        TEXTURE_FORMAT_P010 = TEXTURE_FORMAT_YUV420_10,
        TEXTURE_FORMAT_P016 = TEXTURE_FORMAT_YUV420_16,
    };
    REV_ENUM_CLASS_OPERATORS(TEXTURE_FORMAT);

    struct TextureSurface final
    {
        byte *data;
        u64   row_bytes;
        u64   size_in_bytes;
    };

    struct TextureData final
    {
        u64            planes_count;
        u64            subtextures_count;
        u64            mip_levels_count;
        u64            surfaces_count;
        TextureSurface surfaces[0];
    };

    REV_API void *GetTexturePixelData(const TextureData *texture_data, u64 plane, u64 subtexture, u64 mip_level, TEXTURE_FORMAT format, Math::v2u uv);

    REV_API void InterateTextureSurfaces(TextureData *texture_data, const Function<bool(TextureSurface *surface, u64 mip_level, u64 subtexture, u64 plane)>& SurfaceCallback);
    REV_API void InterateVolumeTextureSurfaces(TextureData *texture_data, const Function<bool(TextureSurface *surface, u64 mip_level)>& SurfaceCallback);

    REV_API TextureData *CreateTextureData(
        const byte     *surfaces,
        u64             mip_levels_count,
        u64             subtextures_count,
        u64             first_mip_level_width,
        u64             first_mip_level_height,
        TEXTURE_FORMAT  format
    );

    REV_API TextureData *CreateVolumeTextureData(
        const byte     *surfaces,
        u64             mip_levels_count,
        u64             first_mip_level_width,
        u64             first_mip_level_height,
        u64             first_mip_level_depth,
        TEXTURE_FORMAT  format
    );

    enum TEXTURE_ADDRESS_MODE : u32
    {
        TEXTURE_ADDRESS_MODE_WRAP = 1,
        TEXTURE_ADDRESS_MODE_MIRROR,
        TEXTURE_ADDRESS_MODE_CLAMP,
        TEXTURE_ADDRESS_MODE_BORDER,
        TEXTURE_ADDRESS_MODE_MIRROR_ONCE,
    };

    // @NOTE(Roman): +-----------------------------+------------------------------+
    //               |            D3D12            |             Vulkan           |
    //               +-----------------------------+------------------------------+
    //               | ConstantBuffer       (CBV)  | Uniform Buffer, std140       |
    //               | TextureBuffer        (SRV?) | Storage Buffer, std430       | // No 256 bytes alignment => std430 => storage blocks
    //               | Buffer               (SRV?) | Storage Texel Buffer, std430 | // No 256 bytes alignment => std430 => storage blocks
    //               | StructuredBuffer     (SRV?) | Storage Buffer, std430       | // No 256 bytes alignment => std430 => storage blocks
    //               | ByteAddressBuffer    (SRV?) | Storage Buffer, std430       | // No 256 bytes alignment => std430 => storage blocks
    //               | RWBuffer             (UAV)  | Storage Texel Buffer, std430 |
    //               | RWStructuredBuffer   (UAV)  | Storage Buffer, std430       |
    //               | RWByteAddressBuffer  (UAV)  | Storage Buffer, std430       |
    //               | Texture[*D*|Cube*]   (SRV)  | Sampled Image                |
    //               | RWTexture[*D*|Cube*] (UAV)  | Storage Image                |
    //               +-----------------------------+------------------------------+
    class REV_API REV_NOVTABLE MemoryManager final
    {
    public:
        ResourceHandle AllocateVertexBuffer(u32 count, u32 stride, bool _static, const ConstString& name = null);
        ResourceHandle AllocateIndexBuffer(u32 count, u32 stride, bool _static, const ConstString& name = null);
        // @NOTE(Roman): D3D12 (HLSL):
        //                   ConstantBuffer or cbuffer;
        //               Vulkan (SPIR-V):
        //                   uniform buffer, std140;
        ResourceHandle AllocateConstantBuffer(u32 bytes, bool _static, const ConstString& name = null);
        // @NOTE(Roman): D3D12 (HLSL):
        //                   if (format == BUFFER_FORMAT_UNKNOWN)
        //                       if (stride) [RW]StructuredBuffer or tbuffer;
        //                       else        [RW]ByteAddressBuffer;
        //                   else
        //                       [RW]Buffer, <= 32 bytes;
        //               Vulkan (SPIR-V):
        //                   storage buffer, std430;
        ResourceHandle AllocateBuffer(u32 bytes, u32 stride, BUFFER_FORMAT format, RESOURCE_FLAG flags, const ConstString& name = null);

        ResourceHandle AllocateTexture1D(u16   width,                        u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name);
        ResourceHandle AllocateTexture2D(u16   width, u16 height,            u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name);
        ResourceHandle AllocateTexture3D(u16   width, u16 height, u16 depth, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name);
        ResourceHandle AllocateTextureCube(u16 width, u16 height,            u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name);

        ResourceHandle AllocateTexture1DArray(u16   width,             u16 count, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name);
        ResourceHandle AllocateTexture2DArray(u16   width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name);
        ResourceHandle AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT format, RESOURCE_FLAG flags, const ConstString& name);

        ResourceHandle AllocateSampler(TEXTURE_ADDRESS_MODE address_mode, Math::v4 border_color, Math::v2 min_max_lod, bool _static);

        void LoadResources(const ConstArray<GPU::ResourceHandle>& resources);
        void StoreResources(const ConstArray<GPU::ResourceHandle>& resources);

        void ResizeRenderTarget(ResourceHandle resource, u16 width, u16 height = 0, u16 depth = 0);

        // @NOTE(Roman): Zero copy return. Just a readback pointer.
        const void *GetBufferData(const ResourceHandle& resource) const;
        void SetBufferData(const ResourceHandle& resource, const void *data);

        // @NOTE(Roman): Is pushed onto frame arena.
        TextureData *GetTextureData(const ResourceHandle& resource) const;
        void SetTextureData(const ResourceHandle& resource, const TextureData *data);

        void FreeSceneMemory();
        void FreeStaticMemory();

        ConstString GetResourceName(const ResourceHandle& resource);
        ConstString GetBufferName(const ResourceHandle& resource);
        ConstString GetTextureName(const ResourceHandle& resource);

        static REV_INLINE bool IsBuffer(const ResourceHandle& resource)
        {
            return resource.kind == RESOURCE_KIND_VERTEX_BUFFER
                || resource.kind == RESOURCE_KIND_INDEX_BUFFER
                || resource.kind == RESOURCE_KIND_CONSTANT_BUFFER
                || resource.kind == RESOURCE_KIND_BUFFER;
        }

        static REV_INLINE bool IsTexture(const ResourceHandle& resource)
        {
            return resource.kind == RESOURCE_KIND_TEXTURE;
        }

        static REV_INLINE bool IsSampler(const ResourceHandle& resource)
        {
            return resource.kind == RESOURCE_KIND_SAMPLER;
        }

        static REV_INLINE bool IsStatic(const ResourceHandle& resource)
        {
            return resource.flags & RESOURCE_FLAG_STATIC;
        }

    private:
        REV_REMOVE_OOP_STUFF(MemoryManager);

    private:
        byte platform[0];

        friend class ::REV::GraphicsAPI;
    };
}   
