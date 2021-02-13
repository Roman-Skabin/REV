//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "tools/static_string.hpp"

// @Cleanup(Roman): We do not need GPU memory manager on user side.
//                  Asset manager allocates and sets all the data we need for us.
//                  So we are fine having only platform version of it.

namespace REV { class GraphicsAPI; }

namespace REV::GPU
{
    enum class RESOURCE_KIND : u64
    {
        UNKNOWN,
        VB,
        IB,
        CB,
        SR,
    };

    struct ResourceHandle final
    {
        u64           index = REV_U64_MAX;
        RESOURCE_KIND kind  = RESOURCE_KIND::UNKNOWN;
    };

    enum class TEXTURE_FORMAT : u32
    {
        UNKNOWN,
        RGBA8,
        BGRA8,
        DXT1,
        DXT2,
        DXT3,
        DXT4,
        DXT5,

        _DDS_DX10 = 0x80000000, // @NOTE(Roman): Internal
    };
    REV_ENUM_CLASS_OPERATORS(TEXTURE_FORMAT);

    struct SubTextureDesc final
    {
        byte *data;
        u64   bytes_per_row;
        u64   bytes_per_tex2d;
    };

    struct TextureDesc final
    {
        SubTextureDesc *subtexture_desc;
        u64             subtextures_count;
    };

    class REV_API MemoryManager final
    {
    public:
        // @Optimize(Roman): Use (upload pointer + offset) as a CPU data storage for buffers?
        ResourceHandle AllocateVertexBuffer(u32 vertex_count, const StaticString<64>& name = null); // stride = sizeof(REV::Vertex)
        ResourceHandle AllocateIndexBuffer(u32 index_count, const StaticString<64>& name = null); // stride = sizeof(REV::Index)
        ResourceHandle AllocateConstantBuffer(u32 bytes, const StaticString<64>& name = null);

        ResourceHandle AllocateTexture1D(  u16 width,                                        TEXTURE_FORMAT texture_format, const StaticString<64>& name = null);
        ResourceHandle AllocateTexture2D(  u16 width, u16 height,            u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name = null);
        ResourceHandle AllocateTexture3D(  u16 width, u16 height, u16 depth, u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name = null);
        ResourceHandle AllocateTextureCube(u16 width, u16 height,            u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name = null);

        ResourceHandle AllocateTexture1DArray(  u16 width,             u16 count,                 TEXTURE_FORMAT texture_format, const StaticString<64>& name = null);
        ResourceHandle AllocateTexture2DArray(  u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name = null);
        ResourceHandle AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT texture_format, const StaticString<64>& name = null);

        void SetBufferData(ResourceHandle resource, const void *data);
        void SetTextureData(ResourceHandle resource, TextureDesc *texture_desc);

        void SetBufferDataImmediately(ResourceHandle resource, const void *data);
        void SetTextureDataImmediately(ResourceHandle resource, TextureDesc *texture_desc);

        void StartImmediateExecution();
        void EndImmediateExecution();

        void FreeMemory();

    private:
        MemoryManager()                     = delete;
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&)      = delete;

        ~MemoryManager() = delete;

        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager& operator=(MemoryManager&&)      = delete;

    private:
        #pragma warning(suppress: 4200)
        byte platform[0];

        friend class ::REV::GraphicsAPI;
    };
}
