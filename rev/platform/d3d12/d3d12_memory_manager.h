//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/array.hpp"
#include "platform/d3d12/d3d12_device_context.h"
#include "graphics/memory_manager.h"

// @TODO(Roman): Thread safety

namespace REV
{
    enum
    {
        D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION = (D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / 6),
    };
}

namespace REV::D3D12
{
    enum BUFFER_KIND : u32
    {
        BUFFER_KIND_UNKNOWN,
        BUFFER_KIND_VERTEX_BUFFER,
        BUFFER_KIND_INDEX_BUFFER,
        BUFFER_KIND_CONSTANT_BUFFER,
        BUFFER_KIND_RW_BUFFER,        // @TODO(Roman): ...

        BUFFER_KIND_STATIC = BIT<u32>(31)
    };
    REV_ENUM_OPERATORS(BUFFER_KIND)

    struct Buffer final
    {
        u64              page_index;
        u64              offset;
        BUFFER_KIND      kind;
        union
        {
            struct { u32 vcount, vstride; };
            struct { u32 icount, istride; };
        };
        u64              actual_size;
        u64              aligned_size;
        StaticString<64> name;
    };

    enum
    {
        BUFFER_MEMORY_PAGE_SIZE = MB(512),
    };

    struct BufferMemoryPage final
    {
        ID3D12Resource        *def_mem;
        ID3D12Resource        *upl_mem[SWAP_CHAIN_BUFFERS_COUNT];
        byte                  *upl_ptrs[SWAP_CHAIN_BUFFERS_COUNT];
        u64                    occupied_bytes;
        D3D12_RESOURCE_STATES  initial_state;
    };

    struct BufferMemory final
    {
        Array<Buffer>           buffers;
        Array<BufferMemoryPage> pages;

        REV_INLINE BufferMemory(Allocator *allocator) : buffers(allocator), pages(allocator) {}
        REV_INLINE ~BufferMemory() {}

        REV_INLINE BufferMemory& operator=(BufferMemory&& other) noexcept
        {
            buffers = RTTI::move(other.buffers);
            pages   = RTTI::move(other.pages);
            return *this;
        }
    };

    enum TEXTURE_KIND
    {
        TEXTURE_KIND_UNKNOWN = 0,
        TEXTURE_KIND_1D,
        TEXTURE_KIND_2D,
        TEXTURE_KIND_3D,
        TEXTURE_KIND_CUBE,
        TEXTURE_KIND_1D_ARRAY,
        TEXTURE_KIND_2D_ARRAY,
        TEXTURE_KIND_CUBE_ARRAY,
    };

    struct Texture final
    {
        ID3D12Resource      *def_resource;
        ID3D12Resource      *upl_resources[SWAP_CHAIN_BUFFERS_COUNT];
        byte                *upl_pointers[SWAP_CHAIN_BUFFERS_COUNT];
        TEXTURE_KIND         kind;
        D3D12_RESOURCE_DESC  desc; // @Cleanup(Roman): choose data that we really need to store
        u8                   cube    : 1;
        u8                   array   : 1;
        u8                   _static : 1;
    };

    struct TextureMemory final
    {
        Array<Texture> textures;

        REV_INLINE TextureMemory(Allocator *allocator) : textures(allocator) {}
        REV_INLINE ~TextureMemory() {}

        REV_INLINE TextureMemory& operator=(TextureMemory&& other) noexcept { textures = RTTI::move(other.textures); return *this; }
    };

    struct Sampler final
    {
        D3D12_SAMPLER_DESC desc;
        bool               _static;
    };

    struct SamplerMemory final
    {
        Array<Sampler> samplers;

        REV_INLINE SamplerMemory(Allocator *allocator) : samplers(allocator) {}
        REV_INLINE ~SamplerMemory() {}

        REV_INLINE SamplerMemory& operator=(SamplerMemory&& other) noexcept { samplers = RTTI::move(other.samplers); return *this; }
    };

    struct ResourceMemory final
    {
        BufferMemory  buffer_memory;
        TextureMemory texture_memory;
        SamplerMemory sampler_memory;

        REV_INLINE ResourceMemory(Allocator *allocator)
            : buffer_memory(allocator),
              texture_memory(allocator),
              sampler_memory(allocator)
        {}

        REV_INLINE ~ResourceMemory() {}

        REV_INLINE ResourceMemory& operator=(ResourceMemory&& other) noexcept
        {
            buffer_memory    = RTTI::move(other.buffer_memory);
            texture_memory   = RTTI::move(other.texture_memory);
            sampler_memory   = RTTI::move(other.sampler_memory);
            return *this;
        }
    };

    class MemoryManager final
    {
    public:
        MemoryManager(Allocator *allocator);
        ~MemoryManager();

        u64 AllocateVertexBuffer(u32 vertex_count, bool _static, const ConstString& name = null);
        u64 AllocateIndexBuffer(u32 index_count, bool _static, const ConstString& name = null);
        u64 AllocateConstantBuffer(u32 bytes, bool _static, const ConstString& name = null);

        u64 AllocateTexture1D(  u16 width,                                        DXGI_FORMAT texture_format, const ConstString& name, bool _static);
        u64 AllocateTexture2D(  u16 width, u16 height,            u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static);
        u64 AllocateTexture3D(  u16 width, u16 height, u16 depth, u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static);
        u64 AllocateTextureCube(u16 width, u16 height,            u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static);

        u64 AllocateTexture1DArray(  u16 width,             u16 count,                 DXGI_FORMAT texture_format, const ConstString& name, bool _static);
        u64 AllocateTexture2DArray(  u16 width, u16 height, u16 count, u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static);
        u64 AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static);

        u64 AllocateSampler(GPU::TEXTURE_ADDRESS_MODE address_mode, Math::v4 border_color, Math::v2 min_max_lod, bool _static);

        void SetBufferData(const Buffer& buffer, const void *data);
        void SetBufferDataImmediately(const Buffer& buffer, const void *data);

        void SetTextureData(Texture *texture, GPU::TextureDesc *texture_desc);
        void SetTextureDataImmediately(Texture *texture, GPU::TextureDesc *texture_desc);

        void StartImmediateExecution();
        void EndImmediateExecution();

        void FreeSceneMemory();
        void FreeStaticMemory();

        REV_INLINE D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUVirtualAddress(const GPU::ResourceHandle& resource_handle) const
        {
            REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_BUFFER, "Resource handle is not handling a buffer");
            if (resource_handle.kind & GPU::RESOURCE_KIND_STATIC)
            {
                const Buffer *buffer = m_StaticMemory.buffer_memory.buffers + resource_handle.index;
                return m_StaticMemory.buffer_memory.pages[buffer->page_index].def_mem->GetGPUVirtualAddress() + buffer->offset;
            }
            else
            {
                const Buffer *buffer = m_SceneMemory.buffer_memory.buffers + resource_handle.index;
                return m_SceneMemory.buffer_memory.pages[buffer->page_index].def_mem->GetGPUVirtualAddress() + buffer->offset;
            }
        }

        REV_INLINE D3D12_GPU_VIRTUAL_ADDRESS GetTextureGPUVirtualAddress(const GPU::ResourceHandle& resource_handle) const
        {
            REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_SR, "Resource handle is not handling a texture");
            return resource_handle.kind & GPU::RESOURCE_KIND_STATIC
                 ? m_StaticMemory.texture_memory.textures[resource_handle.index].def_resource->GetGPUVirtualAddress()
                 : m_SceneMemory.texture_memory.textures[resource_handle.index].def_resource->GetGPUVirtualAddress();
        }

        REV_INLINE const Buffer& GetBuffer(const GPU::ResourceHandle& resource_handle)   const
        {
            REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_BUFFER, "Resource handle is not handling a buffer");
            return resource_handle.kind & GPU::RESOURCE_KIND_STATIC
                 ? m_StaticMemory.buffer_memory.buffers[resource_handle.index]
                 : m_SceneMemory.buffer_memory.buffers[resource_handle.index];
        }

        REV_INLINE const Texture& GetTexture(const GPU::ResourceHandle& resource_handle)  const
        {
            REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_SR, "Resource handle is not handling a texture");
            return resource_handle.kind & GPU::RESOURCE_KIND_STATIC
                 ? m_StaticMemory.texture_memory.textures[resource_handle.index]
                 : m_SceneMemory.texture_memory.textures[resource_handle.index];
        }

        REV_INLINE const Sampler& GetSampler(const GPU::ResourceHandle& resource_handle) const
        {
            REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_SAMPLER, "Resource handle is not handling a sampler");
            return resource_handle.kind & GPU::RESOURCE_KIND_STATIC
                 ? m_StaticMemory.sampler_memory.samplers[resource_handle.index]
                 : m_SceneMemory.sampler_memory.samplers[resource_handle.index];
        }

        REV_INLINE Buffer& GetBuffer(const GPU::ResourceHandle& resource_handle)
        {
            REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_BUFFER, "Resource handle is not handling a buffer");
            return resource_handle.kind & GPU::RESOURCE_KIND_STATIC
                 ? m_StaticMemory.buffer_memory.buffers[resource_handle.index]
                 : m_SceneMemory.buffer_memory.buffers[resource_handle.index];
        }

        REV_INLINE Texture& GetTexture(const GPU::ResourceHandle& resource_handle)
        {
            REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_SR, "Resource handle is not handling a texture");
            return resource_handle.kind & GPU::RESOURCE_KIND_STATIC
                 ? m_StaticMemory.texture_memory.textures[resource_handle.index]
                 : m_SceneMemory.texture_memory.textures[resource_handle.index];
        }

        REV_INLINE Sampler& GetSampler(const GPU::ResourceHandle& resource_handle)
        {
            REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_SAMPLER, "Resource handle is not handling a sampler");
            return resource_handle.kind & GPU::RESOURCE_KIND_STATIC
                 ? m_StaticMemory.sampler_memory.samplers[resource_handle.index]
                 : m_SceneMemory.sampler_memory.samplers[resource_handle.index];
        }

    private:
        void CreateNewPage(BufferMemory *buffer_memory, D3D12_RESOURCE_STATES initial_state);

        Buffer *AllocateBuffer(BufferMemory *buffer_memory, u64 size, D3D12_RESOURCE_STATES initial_state, u64& index, const ConstString& name);
        Texture *AllocateTexture(TextureMemory *texture_memory, const D3D12_RESOURCE_DESC& desc, u64& index, const ConstString& name);

        void UploadBufferData(ID3D12GraphicsCommandList *command_list, const Buffer& buffer, const void *data);
        void UploadTextureData(ID3D12GraphicsCommandList *command_list, Texture *texture, u32 subres_count, D3D12_SUBRESOURCE_DATA *subresources);

        void SetBufferName(BufferMemory *buffer_memory, Buffer *buffer, const ConstString& name);

        REV_DELETE_CONSTRS_AND_OPS(MemoryManager);

    private:
        DeviceContext             *m_DeviceContext;
        ID3D12CommandAllocator    *m_CommandAllocator;
        ID3D12GraphicsCommandList *m_CommandList;
        ID3D12Fence               *m_Fence;
        Event                      m_FenceEvent;
        ResourceMemory             m_StaticMemory;
        ResourceMemory             m_SceneMemory;
    };
}
