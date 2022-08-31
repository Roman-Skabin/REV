// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/const_string.h"
#include "math/vec.h"
#include "tools/const_array.hpp"

namespace REV
{
    class GraphicsAPI;
    class TextureData;

    enum RESOURCE_KIND : u8
    {
        RESOURCE_KIND_NONE = 0,
        RESOURCE_KIND_BUFFER,
        RESOURCE_KIND_TEXTURE,
        RESOURCE_KIND_SAMPLER,
    };

    enum RESOURCE_FLAG : u16
    {
        RESOURCE_FLAG_NONE                         = 0,

        // @NOTE(Roman): Textures & buffers
        RESOURCE_FLAG_ALLOW_NONCOHERENT_GPU_WRITES = 1 << 0,
        RESOURCE_FLAG_ALLOW_SHADER_READ            = 1 << 1,

        // @NOTE(Roman): Textures
        RESOURCE_FLAG_ALLOW_COLOR_TARGET           = 1 << 2,
        RESOURCE_FLAG_ALLOW_DEPTH_STENCIL_TARGET   = 1 << 3,

        // @NOTE(Roman): Buffers
        RESOURCE_FLAG_VERTEX_BUFFER                = 1 << 4,
        RESOURCE_FLAG_INDEX_BUFFER                 = 1 << 5,
        RESOURCE_FLAG_CONSTANT_BUFFER              = 1 << 6,

        // @NOTE(Roman): Access
        RESOURCE_FLAG_CPU_READ                     = 1 << 7,
        RESOURCE_FLAG_CPU_READ_ONCE                = 1 << 8,
        RESOURCE_FLAG_CPU_WRITE                    = 1 << 9,
        RESOURCE_FLAG_CPU_WRITE_ONCE               = 1 << 10,

        // @NOTE(Roman): Memory
        RESOURCE_FLAG_PER_FRAME                    = 1 << 11,
        RESOURCE_FLAG_PER_SCENE                    = 1 << 12,
        RESOURCE_FLAG_PERMANENT                    = 1 << 13,
     // RESOURCE_FLAG_CUSTOM_LIFETIME              = 1 << 14, // @TODO(Roman): Custom lifetime heap
    };
    REV_ENUM_OPERATORS(RESOURCE_FLAG);

    enum TEXTURE_FORMAT : u8
    {
        TEXTURE_FORMAT_UNKNOWN = 0,

        TEXTURE_FORMAT_FIRST_UNCOMPRESSED,

        // @NOTE(Roman): Uncompressed formats (order: xyzw)
        TEXTURE_FORMAT_S32 = TEXTURE_FORMAT_FIRST_UNCOMPRESSED,
        TEXTURE_FORMAT_U32,

        TEXTURE_FORMAT_R32,
        TEXTURE_FORMAT_RG32,
        TEXTURE_FORMAT_RGB32,
        TEXTURE_FORMAT_RGBA32,

        TEXTURE_FORMAT_BGRA8,
        TEXTURE_FORMAT_RGBA8,
        TEXTURE_FORMAT_A8,
        TEXTURE_FORMAT_B5G6R5,
        TEXTURE_FORMAT_BGR5A1,
        TEXTURE_FORMAT_BGRA4,
        TEXTURE_FORMAT_R10G10B10A2,
        TEXTURE_FORMAT_RGBA16,

        // @NOTE(Roman): Depth stencil formats (order: xyzw)
        TEXTURE_FORMAT_D16,
        TEXTURE_FORMAT_D32,
        TEXTURE_FORMAT_D24S8,

        TEXTURE_FORMAT_LAST_UNCOMPRESSED = TEXTURE_FORMAT_D24S8,
        TEXTURE_FORMAT_FIRST_COMPRESSED,

        // @NOTE(Roman): Block compressed formats
        TEXTURE_FORMAT_BC1 = TEXTURE_FORMAT_FIRST_COMPRESSED,
        TEXTURE_FORMAT_BC2,
        TEXTURE_FORMAT_BC3,
        TEXTURE_FORMAT_BC4,
        TEXTURE_FORMAT_BC5,
    //  TEXTURE_FORMAT_BC6H, // @TODO(Roman): HDR support
        TEXTURE_FORMAT_BC7,

        TEXTURE_FORMAT_LAST_COMPRESSED = TEXTURE_FORMAT_BC7
    };

    enum TEXTURE_DIMENSION : u8
    {
        TEXTURE_DIMENSION_UNKNOWN = 0,
        TEXTURE_DIMENSION_1D,
        TEXTURE_DIMENSION_2D,
        TEXTURE_DIMENSION_3D,
        TEXTURE_DIMENSION_CUBE,
        TEXTURE_DIMENSION_1D_ARRAY,
        TEXTURE_DIMENSION_2D_ARRAY,
        TEXTURE_DIMENSION_CUBE_ARRAY,
    };

    enum SAMPLER_ADRESS_MODE : u8
    {
        SAMPLER_ADRESS_MODE_CLAMP,
        SAMPLER_ADRESS_MODE_WRAP,
        SAMPLER_ADRESS_MODE_MIRROR,
    };

    enum SAMPLER_FILTER : u8
    {
        SAMPLER_FILTER_POINT,
        SAMPLER_FILTER_BILINEAR,
        SAMPLER_FILTER_TRILINEAR,
        SAMPLER_FILTER_ANISOTROPIC_1X,
        SAMPLER_FILTER_ANISOTROPIC_2X,
        SAMPLER_FILTER_ANISOTROPIC_4X,
        SAMPLER_FILTER_ANISOTROPIC_8X,
        SAMPLER_FILTER_ANISOTROPIC_16X,
    };

    struct BufferDesc
    {
        union
        {
            // @NOTE(Roman): For constant and non-coherent buffers only.
            u32 size        = 0;

            // @NOTE(Roman): For vertex and index buffers only.
            struct
            {
                u32 count   = 0;
                u32 stride  = 0;
            };
        };
        RESOURCE_FLAG flags = RESOURCE_FLAG_NONE;
    };

    struct TextureDesc
    {
        u16               width      = 0;
        u16               height     = 0;
        u16               depth      = 0;
        u16               array_size = 0;
        u16               mip_levels = 1;
        RESOURCE_FLAG     flags      = RESOURCE_FLAG_NONE;
        TEXTURE_FORMAT    format     = TEXTURE_FORMAT_UNKNOWN;
        TEXTURE_DIMENSION dimension  = TEXTURE_DIMENSION_UNKNOWN;
    };

    struct SamplerDesc
    {
        Math::v2            min_max_lod = Math::v2(0.0f, 100.0f);
        SAMPLER_ADRESS_MODE adress_mode = SAMPLER_ADRESS_MODE_CLAMP;
        SAMPLER_FILTER      filter      = SAMPLER_FILTER_POINT;
        RESOURCE_FLAG       flags       = RESOURCE_FLAG_NONE;
    };

    struct ResourceHandle
    {
        void          *ptr  = null;
        RESOURCE_KIND  kind = RESOURCE_KIND_NONE;

        operator bool() const;

    private:
        // @TODO(Roman): #MultipleHandlesToTheSameResource.
        //               What if we want to have several handles to the same resoucre?
        REV_DELETE_CONSTRS_AND_OPS(ResourceHandle);
    };

    bool operator==(const ResourceHandle& left, const ResourceHandle& right);
    bool operator!=(const ResourceHandle& left, const ResourceHandle& right);

    class REV_API MemoryManager final
    {
    public:
        ResourceHandle AllocateBuffer(const BufferDesc& desc, const ConstString& name = null);
        ResourceHandle AllocateTexture(const TextureDesc& desc, const ConstString& name = null);
        ResourceHandle AllocateSampler(const SamplerDesc& desc, const ConstString& name = null);

        void UpdateBuffer(const ResourceHandle& resource, const void *data);
        void UpdateTexture(const ResourceHandle& resource, const TextureData& data);

        void *ReadBuffer(const ResourceHandle& resource);
        TextureData ReadTexture(const ResourceHandle& resource);

        void UploadResources(const ConstArray<ResourceHandle>& resources);
        void ReadbackResources(const ConstArray<ResourceHandle>& resources);

        void ResizeBuffer(const ResourceHandle& resource, u32 bytes);
        void ResizeTexture(const ResourceHandle& resource, u16 width, u16 height = 0, u16 depth = 0);

        void FreePerFrameMemory();
        void FreePerSceneMemory();

        ConstString GetResourceName(const ResourceHandle& resource);
        ConstString GetBufferName(const ResourceHandle& resource);
        ConstString GetTextureName(const ResourceHandle& resource);

    private:
        REV_REMOVE_OOP_STUFF(MemoryManager);

    private:
        friend class GraphicsAPI;
    };
}

#include "graphics/memory_manager.inl"
