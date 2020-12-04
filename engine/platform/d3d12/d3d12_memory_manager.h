//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/array.hpp"
#include "platform/d3d12/d3d12_renderer.h"

// @TODO(Roman): Thread safety

namespace D3D12
{
    enum class BUFFER_KIND
    {
        UNKNOWN,
        VERTEX_BUFFER,
        INDEX_BUFFER,
        CONSTANT_BUFFER,
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
        ID3D12Resource      *def_resoucre;
        ID3D12Resource      *upl_resource[SWAP_CHAIN_BUFFERS_COUNT];
        u64                  desc_heap_index;
        StaticString<64>     name;
        D3D12_RESOURCE_DESC  desc;
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
        u64                         resoucre_index;
        D3D12_DESCRIPTOR_HEAP_DESC  desc;
    };

    struct DescHeapMemory final
    {
        Array<DescHeap> desc_heaps;

        DescHeapMemory(Allocator *allocator) : desc_heaps(allocator) {}
        ~DescHeapMemory() {}

        DescHeapMemory& operator=(DescHeapMemory&& other) noexcept { desc_heaps = RTTI::move(other.desc_heaps); return *this; }
    };

    class ENGINE_API MemoryManager final
    {
    public:
        MemoryManager(Allocator *allocator);
        ~MemoryManager();

        u64 AllocateVertexBuffer(u32 vertex_count, u32 vertex_stride, const StaticString<64>& name = null);
        u64 AllocateIndexBuffer(u32 index_count, const StaticString<64>& name = null);
        u64 AllocateConstantBuffer(u32 bytes, const StaticString<64>& name);

        void SetBufferData(const Buffer& buffer, const void *data);
        void SetBufferDataImmediate(const Buffer& buffer, const void *data);

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
            return texture.def_resoucre->GetGPUVirtualAddress();
        }

        const Buffer&   GetBuffer(u64 index)   const { return m_BufferMemory.buffers[index];      }
        const Texture&  GetTexture(u64 index)  const { return m_TextureMemory.textures[index];    }
        const DescHeap& GetDescHeap(u64 index) const { return m_DescHeapMemory.desc_heaps[index]; }

        Buffer&   GetBuffer(u64 index)   { return m_BufferMemory.buffers[index];      }
        Texture&  GetTexture(u64 index)  { return m_TextureMemory.textures[index];    }
        DescHeap& GetDescHeap(u64 index) { return m_DescHeapMemory.desc_heaps[index]; }

        MemoryManager& operator=(MemoryManager&& other) noexcept;

    private:
        void CreateNewPage(D3D12_RESOURCE_STATES initial_state);

        Buffer *AllocateBuffer(u64 size, D3D12_RESOURCE_STATES initial_state, u64& index);

        DescHeap *CreateDescHeapForConstantBuffer(Buffer *buffer, u64 resource_index);

        void SetBufferData(ID3D12GraphicsCommandList *command_list, const Buffer& buffer, const void *data);

        MemoryManager(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&)      = delete;

        MemoryManager& operator=(const MemoryManager&) = delete;

    private:
        ID3D12CommandAllocator    *m_CommandAllocator;
        ID3D12GraphicsCommandList *m_CommandList;
        ID3D12Fence               *m_Fence;
        Event                      m_FenceEvent;
        DescHeapMemory             m_DescHeapMemory;
        BufferMemory               m_BufferMemory;
        TextureMemory              m_TextureMemory;
    };
}
