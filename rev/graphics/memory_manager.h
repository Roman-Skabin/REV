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
    // @TODO(Roman): Add RTVs and DSVs?
    enum RESOURCE_KIND : u32
    {
        RESOURCE_KIND_UNKNOWN = 0,

        RESOURCE_KIND_VERTEX_BUFFER     = 1,
        RESOURCE_KIND_INDEX_BUFFER      = 2,
        RESOURCE_KIND_CONSTANT_BUFFER   = 3, // @NOTE(Roman): CBV
        RESOURCE_KIND_READ_ONLY_BUFFER  = 4, // @NOTE(Roman): SRV
        RESOURCE_KIND_READ_WRITE_BUFFER = 5, // @NOTE(Roman): UAV

        RESOURCE_KIND_READ_ONLY_TEXTURE  = 6, // @NOTE(Roman): SRV
        RESOURCE_KIND_READ_WRITE_TEXTURE = 7, // @NOTE(Roman): UAV

        RESOURCE_KIND_SAMPLER = 8,

        RESOURCE_KIND_STATIC = 0x80000000,
    };
    REV_ENUM_OPERATORS(RESOURCE_KIND);

    struct ResourceHandle final
    {
        u64           index = REV_INVALID_U64_INDEX;
        RESOURCE_KIND kind  = RESOURCE_KIND_UNKNOWN;

        REV_INLINE operator bool() const { return index != REV_INVALID_U64_INDEX; }
    };

    REV_INLINE bool operator==(const ResourceHandle& left, const ResourceHandle& right) { return left.index == right.index && left.kind == right.kind; }
    REV_INLINE bool operator!=(const ResourceHandle& left, const ResourceHandle& right) { return left.index != right.index || left.kind != right.kind; }

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

    class REV_API REV_NOVTABLE MemoryManager final
    {
    public:
        ResourceHandle AllocateVertexBuffer(u32 count, bool _static, const ConstString& name = null);
        ResourceHandle AllocateIndexBuffer(u32 count, bool _static, const ConstString& name = null);
        // @NOTE(Roman): D3D12 (HLSL):    cbuffer or ConstantBuffer;
        //               Vulkan (SPIR-V): uniform buffer, std140;
        ResourceHandle AllocateConstantBuffer(u32 bytes, bool _static, const ConstString& name = null);
        // @NOTE(Roman): D3D12 (HLSL):
        //                   if (BUFFER_FORMAT == BUFFER_FORMAT_UNKNOWN)
        //                       if (stride) StructuredBuffer;
        //                       else        ByteAddressBuffer;
        //                   else
        //                       Buffer;
        //               Vulkan (SPIR-V):
        //                   if (cpu_read_access) uniform buffer, std430;
        //                   else                 storage buffer, std430;
        ResourceHandle AllocateBuffer(u32 bytes, u32 stride, BUFFER_FORMAT format, bool cpu_read_access, bool _static, const ConstString& name = null);

        ResourceHandle AllocateTexture(u16 width, u16 height, u16 depth, u16 mip_levels, RESOURCE_KIND kind, TEXTURE_FORMAT texture_format, const ConstString& name);
        ResourceHandle AllocateTextureArray(u16 width, u16 height, u16 count, u16 mip_levels, RESOURCE_KIND kind, TEXTURE_FORMAT texture_format, const ConstString& name);
        ResourceHandle AllocateTextureCube(u16 width, u16 height, u16 mip_levels, RESOURCE_KIND kind, TEXTURE_FORMAT texture_format, const ConstString& name);
        ResourceHandle AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, RESOURCE_KIND kind, TEXTURE_FORMAT texture_format, const ConstString& name);

        ResourceHandle AllocateSampler(TEXTURE_ADDRESS_MODE address_mode, Math::v4 border_color, Math::v2 min_max_lod, bool _static);

        void SetBufferData(const ResourceHandle& resource, const void *data);
        void SetTextureData(const ResourceHandle& resource, const TextureData *data);

        void SetBufferDataImmediately(const ResourceHandle& resource, const void *data);
        void SetTextureDataImmediately(const ResourceHandle& resource, const TextureData *data);

        void CopyDefaultResourcesToReadBackResources(const ConstArray<ResourceHandle>& read_write_resources);

        // @NOTE(Roman): Zero copy return. Just a readback pointer.
        const void *GetBufferData(const ResourceHandle& resource) const;
        // @NOTE(Roman): Is pushed onto frame arena.
        TextureData *GetTextureData(const ResourceHandle& resource) const;

        void StartImmediateExecution();
        void EndImmediateExecution();

        void FreeSceneMemory();
        void FreeStaticMemory();

        // @NOTE(Roman): Are pushed onto frame arena.
        ConstArray<GPU::ResourceHandle> GetReadWriteResources() const;

        static REV_INLINE bool IsBuffer(const ResourceHandle& resource)
        {
            RESOURCE_KIND kind = resource.kind & ~RESOURCE_KIND_STATIC;
            return kind == RESOURCE_KIND_VERTEX_BUFFER
                || kind == RESOURCE_KIND_INDEX_BUFFER
                || kind == RESOURCE_KIND_CONSTANT_BUFFER
                || kind == RESOURCE_KIND_READ_ONLY_BUFFER
                || kind == RESOURCE_KIND_READ_WRITE_BUFFER;
        }

        static REV_INLINE bool IsTexture(const ResourceHandle& resource)
        {
            RESOURCE_KIND kind = resource.kind & ~RESOURCE_KIND_STATIC;
            return kind == RESOURCE_KIND_READ_ONLY_TEXTURE
                || kind == RESOURCE_KIND_READ_WRITE_TEXTURE;
        }

        static REV_INLINE bool IsSampler(const ResourceHandle& resource)
        {
            return (resource.kind & ~RESOURCE_KIND_STATIC) == RESOURCE_KIND_SAMPLER;
        }

        static REV_INLINE bool IsStatic(const ResourceHandle& resource)
        {
            return resource.kind & RESOURCE_KIND_STATIC;
        }

        static REV_INLINE bool IsReadWriteResource(const ResourceHandle& resource)
        {
            RESOURCE_KIND kind = resource.kind & ~RESOURCE_KIND_STATIC;
            return kind == RESOURCE_KIND_READ_WRITE_BUFFER
                || kind == RESOURCE_KIND_READ_WRITE_TEXTURE;
        }

    private:
        REV_REMOVE_OOP_STUFF(MemoryManager);

    private:
        byte platform[0];

        friend class ::REV::GraphicsAPI;
    };
}   
