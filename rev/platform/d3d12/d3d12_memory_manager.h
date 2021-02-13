//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/array.hpp"
#include "platform/d3d12/d3d12_renderer.h"

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
    enum class BUFFER_KIND
    {
        UNKNOWN,
        VERTEX_BUFFER,
        INDEX_BUFFER,
        CONSTANT_BUFFER,
        RW_BUFFER        // @TODO(Roman): ...
    };

    struct Buffer final
    {
        u64          page_index;
        u64          offset;
        BUFFER_KIND  kind;
        u64          desc_heap_index;
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

        BufferMemory(Allocator *allocator) : buffers(allocator), pages(allocator) {}
        ~BufferMemory() {}

        BufferMemory& operator=(BufferMemory&& other) noexcept
        {
            buffers = RTTI::move(other.buffers);
            pages   = RTTI::move(other.pages);
            return *this;
        }
    };

    struct Texture final
    {
        ID3D12Resource      *def_resource;
        ID3D12Resource      *upl_resource;
        u64                  desc_heap_index;
        StaticString<64>     name;
        D3D12_RESOURCE_DESC  desc; // @Cleanup(Roman): choose data that we really need to store
        u8                   cube  : 1;
        u8                   array : 1;
    };

    struct TextureMemory final
    {
        Array<Texture> textures;

        TextureMemory(Allocator *allocator) : textures(allocator) {}
        ~TextureMemory() {}

        TextureMemory& operator=(TextureMemory&& other) noexcept { textures = RTTI::move(other.textures); return *this; }
    };

    struct DescHeap final
    {
        ID3D12DescriptorHeap       *handle;
        union
        {
            u64                     resoucre_index;
            u64                     sampler_index;
        };
        D3D12_DESCRIPTOR_HEAP_DESC  desc;
    };

    struct DescHeapMemory final
    {
        Array<DescHeap> desc_heaps;

        DescHeapMemory(Allocator *allocator) : desc_heaps(allocator) {}
        ~DescHeapMemory() {}

        DescHeapMemory& operator=(DescHeapMemory&& other) noexcept { desc_heaps = RTTI::move(other.desc_heaps); return *this; }
    };

    struct Sampler final
    {
        u64                desc_heap_index;
        D3D12_SAMPLER_DESC desc;
    };

    struct SamplerMemory final
    {
        Array<Sampler> samplers;

        SamplerMemory(Allocator *allocator) : samplers(allocator) {}
        ~SamplerMemory() {}

        SamplerMemory& operator=(SamplerMemory&& other) noexcept { samplers = RTTI::move(other.samplers); return *this; }
    };

    class MemoryManager final
    {
    public:
        MemoryManager(Allocator *allocator);
        ~MemoryManager();

        u64 AllocateVertexBuffer(u32 vertex_count, const StaticString<64>& name = null);
        u64 AllocateIndexBuffer(u32 index_count, const StaticString<64>& name = null);
        u64 AllocateConstantBuffer(u32 bytes, const StaticString<64>& name = null);

        u64 AllocateTexture1D(  u16 width,                                        DXGI_FORMAT texture_format, const StaticString<64>& name = null);
        u64 AllocateTexture2D(  u16 width, u16 height,            u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name = null);
        u64 AllocateTexture3D(  u16 width, u16 height, u16 depth, u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name = null);
        u64 AllocateTextureCube(u16 width, u16 height,            u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name = null);

        u64 AllocateTexture1DArray(  u16 width,             u16 count,                 DXGI_FORMAT texture_format, const StaticString<64>& name = null);
        u64 AllocateTexture2DArray(  u16 width, u16 height, u16 count, u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name = null);
        u64 AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name = null);

        // @TOOD(Roman): #Settings: Read most of the stuff from Settings.
        // @TODO(Roman): Pass only address method
        u64 AllocateSampler(const D3D12_SAMPLER_DESC& sampler_desc);

        void SetBufferData(const Buffer& buffer, const void *data);
        void SetBufferDataImmediately(const Buffer& buffer, const void *data);

        void SetTextureData(Texture *texture, GPU::TextureDesc *texture_desc);
        void SetTextureDataImmediately(Texture *texture, GPU::TextureDesc *texture_desc);

        void StartImmediateExecution();
        void EndImmediateExecution();

        void FreeMemory();

        D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUVirtualAddress(u64 index) const
        {
            const Buffer& buffer = m_BufferMemory.buffers[index];
            return m_BufferMemory.pages[buffer.page_index].def_mem->GetGPUVirtualAddress() + buffer.offset;
        }

        D3D12_GPU_VIRTUAL_ADDRESS GetTextureGPUVirtualAddress(u64 index) const
        {
            const Texture& texture = m_TextureMemory.textures[index];
            return texture.def_resource->GetGPUVirtualAddress();
        }

        const Buffer&   GetBuffer(u64 index)   const { return m_BufferMemory.buffers[index];      }
        const Texture&  GetTexture(u64 index)  const { return m_TextureMemory.textures[index];    }
        const DescHeap& GetDescHeap(u64 index) const { return m_DescHeapMemory.desc_heaps[index]; }

        Buffer&   GetBuffer(u64 index)   { return m_BufferMemory.buffers[index];      }
        Texture&  GetTexture(u64 index)  { return m_TextureMemory.textures[index];    }
        DescHeap& GetDescHeap(u64 index) { return m_DescHeapMemory.desc_heaps[index]; }

    private:
        void CreateNewPage(D3D12_RESOURCE_STATES initial_state);

        Buffer *AllocateBuffer(u64 size, D3D12_RESOURCE_STATES initial_state, u64& index);
        Texture *AllocateTexture(const D3D12_RESOURCE_DESC& desc, u64& index);

        DescHeap *CreateDescHeapForResource(u64 resource_index, u64& desc_heap_index);
        DescHeap *CreateDescHeapForSampler(u64 sampler_index, u64& desc_heap_index);

        void UploadBufferData(ID3D12GraphicsCommandList *command_list, const Buffer& buffer, const void *data);
        void UploadTextureData(ID3D12GraphicsCommandList *command_list, Texture *texture, u32 subres_count, D3D12_SUBRESOURCE_DATA *subresources);

        MemoryManager(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&)      = delete;

        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager& operator=(MemoryManager&&)      = delete;

    private:
        ID3D12CommandAllocator    *m_CommandAllocator;
        ID3D12GraphicsCommandList *m_CommandList;
        ID3D12Fence               *m_Fence;
        Event                      m_FenceEvent;
        DescHeapMemory             m_DescHeapMemory;
        SamplerMemory              m_SamplerMemory;
        BufferMemory               m_BufferMemory;
        TextureMemory              m_TextureMemory;
    };
}
