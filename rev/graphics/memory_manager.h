//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "tools/const_string.h"
#include "math/vec.h"

// @Cleanup(Roman): We do not need GPU memory manager on user side.
//                  Asset manager allocates and sets all the data we need for us.
//                  So we are fine having only platform version of it?

namespace REV { class GraphicsAPI; }

namespace REV::GPU
{
    enum RESOURCE_KIND : u64
    {
        RESOURCE_KIND_UNKNOWN = 0,

        RESOURCE_KIND_VB      = 1 << 0,
        RESOURCE_KIND_IB      = 1 << 1,
        RESOURCE_KIND_CB      = 1 << 2,
        RESOURCE_KIND_SR      = 1 << 3,
        RESOURCE_KIND_SAMPLER = 1 << 4,

        RESOURCE_KIND_STATIC = 1ui64 << 63,

        RESOURCE_KIND_BUFFER = RESOURCE_KIND_VB | RESOURCE_KIND_IB | RESOURCE_KIND_CB,

        RESOURCE_KIND_STATIC_VB      = RESOURCE_KIND_STATIC | RESOURCE_KIND_VB,
        RESOURCE_KIND_STATIC_IB      = RESOURCE_KIND_STATIC | RESOURCE_KIND_IB,
        RESOURCE_KIND_STATIC_CB      = RESOURCE_KIND_STATIC | RESOURCE_KIND_CB,
        RESOURCE_KIND_STATIC_SR      = RESOURCE_KIND_STATIC | RESOURCE_KIND_SR,
        RESOURCE_KIND_STATIC_SAMPLER = RESOURCE_KIND_STATIC | RESOURCE_KIND_SAMPLER,
    };
    REV_ENUM_OPERATORS(RESOURCE_KIND);

    struct ResourceHandle final
    {
        u64           index = REV_U64_MAX;
        RESOURCE_KIND kind  = RESOURCE_KIND_UNKNOWN;
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

        _DDS_DX10 = 1ui32 << 31, // @NOTE(Roman): Internal
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
        u64            subtextures_count;
        SubTextureDesc subtexture_desc[0];
    };

    enum TEXTURE_ADDRESS_MODE : u32
    {
        TEXTURE_ADDRESS_MODE_WRAP = 1,
        TEXTURE_ADDRESS_MODE_MIRROR,
        TEXTURE_ADDRESS_MODE_CLAMP,
        TEXTURE_ADDRESS_MODE_BORDER,
        TEXTURE_ADDRESS_MODE_MIRROR_ONCE,
    };

    class REV_API MemoryManager final
    {
    public:
        // @Optimize(Roman): Use (upload pointer + offset) as a CPU data storage for buffers?
        ResourceHandle AllocateVertexBuffer(u32 vertex_count, bool _static, const ConstString& name = null); // stride = sizeof(REV::Vertex)
        ResourceHandle AllocateIndexBuffer(u32 index_count, bool _static, const ConstString& name = null); // stride = sizeof(REV::Index)
        ResourceHandle AllocateConstantBuffer(u32 bytes, bool _static, const ConstString& name = null);

        ResourceHandle AllocateTexture1D(  u16 width,                                        TEXTURE_FORMAT texture_format, const ConstString& name, bool _static);
        ResourceHandle AllocateTexture2D(  u16 width, u16 height,            u16 mip_levels, TEXTURE_FORMAT texture_format, const ConstString& name, bool _static);
        ResourceHandle AllocateTexture3D(  u16 width, u16 height, u16 depth, u16 mip_levels, TEXTURE_FORMAT texture_format, const ConstString& name, bool _static);
        ResourceHandle AllocateTextureCube(u16 width, u16 height,            u16 mip_levels, TEXTURE_FORMAT texture_format, const ConstString& name, bool _static);

        ResourceHandle AllocateTexture1DArray(  u16 width,             u16 count,                 TEXTURE_FORMAT texture_format, const ConstString& name, bool _static);
        ResourceHandle AllocateTexture2DArray(  u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT texture_format, const ConstString& name, bool _static);
        ResourceHandle AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, TEXTURE_FORMAT texture_format, const ConstString& name, bool _static);

        ResourceHandle AllocateSampler(TEXTURE_ADDRESS_MODE address_mode, Math::v4 border_color, Math::v2 min_max_lod, bool _static);

        void SetBufferData(const ResourceHandle& resource, const void *data);
        void SetTextureData(const ResourceHandle& resource, TextureDesc *texture_desc);

        void SetBufferDataImmediately(const ResourceHandle& resource, const void *data);
        void SetTextureDataImmediately(const ResourceHandle& resource, TextureDesc *texture_desc);

        void StartImmediateExecution();
        void EndImmediateExecution();

        void FreeSceneMemory();
        void FreeStaticMemory();

    private:
        REV_REMOVE_OOP_STUFF(MemoryManager);

    private:
        byte platform[0];

        friend class ::REV::GraphicsAPI;
    };
}
