// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/array.hpp"
#include "graphics/memory_manager.h"
#include "memory/arena.h"

#include "platform/d3d12/d3d12_device_context.h"

// @TODO(Roman): Thread safety

namespace REV
{
    enum
    {
        D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / 6,
    };
}

namespace REV::D3D12
{
    enum RO_RW_BUFFER_TYPE : u32
    {
        RO_RW_BUFFER_TYPE_NONE,         // @NOTE(Roman): For not RO/RW buffer type
        RO_RW_BUFFER_TYPE_STRUCTURED,   // @NOTE(Roman): HLSL StructuredBuffer type
        RO_RW_BUFFER_TYPE_BYTE_ADDRESS, // @NOTE(Roman): HLSL ByteAddressBuffer type
        RO_RW_BUFFER_TYPE_BUFFER,       // @NOTE(Roman): HLSL Buffer type
    };

    struct Buffer final
    {
        u64 default_page_index  = REV_INVALID_U64_INDEX;
        u64 upload_page_index   = REV_INVALID_U64_INDEX;
        u64 readback_page_index = REV_INVALID_U64_INDEX;

        u64 default_page_offset  = REV_INVALID_U64_OFFSET;
        u64 upload_page_offset   = REV_INVALID_U64_OFFSET;
        u64 readback_page_offset = REV_INVALID_U64_OFFSET;

        GPU::RESOURCE_FLAG flags      = GPU::RESOURCE_FLAG_NONE;
        RO_RW_BUFFER_TYPE  ro_rw_type = RO_RW_BUFFER_TYPE_NONE;
        DXGI_FORMAT        format     = DXGI_FORMAT_UNKNOWN;
        u32                stride     = 0;

        u64 actual_size  = 0;
        u64 aligned_size = 0;

        StaticString<64> name;
    };

    enum
    {
        DEFAULT_BUFFER_MEMORY_PAGE_SIZE  = MB(512),
        UPLOAD_BUFFER_MEMORY_PAGE_SIZE   = MB(512),
        READBACK_BUFFER_MEMORY_PAGE_SIZE = MB(512),
    };

    struct DefaultBufferMemoryPage final
    {
        ID3D12Resource        *gpu_mem        = null;
        u64                    occupied_bytes = 0;
        D3D12_RESOURCE_STATES  initial_state  = D3D12_RESOURCE_STATE_COMMON;
        DXGI_FORMAT            format         = DXGI_FORMAT_UNKNOWN;
    };

    struct UploadBufferMemoryPage final
    {
        ID3D12Resource *gpu_mem        = null;
        byte           *cpu_mem        = null;
        u64             occupied_bytes = 0;
    };

    struct ReadBackBufferMemoryPage final
    {
        ID3D12Resource *gpu_mem        = null;
        byte           *cpu_mem        = null;
        u64             occupied_bytes = 0;
    };

    struct BufferMemory final
    {
        Array<Buffer>                   buffers;
        Array<DefaultBufferMemoryPage>  default_pages;
        Array<UploadBufferMemoryPage>   upload_pages;
        Array<ReadBackBufferMemoryPage> readback_pages;

        REV_INLINE BufferMemory(Allocator *allocator)
            : buffers(allocator),
              default_pages(allocator),
              upload_pages(allocator),
              readback_pages(allocator)
        {
        }

        REV_INLINE ~BufferMemory()
        {
        }

        REV_DELETE_CONSTRS_AND_OPS(BufferMemory);
    };

    enum TEXTURE_DIMENSION : u32
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

    struct Texture final
    {
        ID3D12Resource     *default_gpu_mem = null;

        ID3D12Resource     *upload_gpu_mem = null;
        byte               *upload_cpu_mem = null;

        ID3D12Resource     *readback_gpu_mem = null;
        byte               *readback_cpu_mem = null;

        u64                 first_subresource_offset = 0;
        u64                 buffer_total_bytes       = 0;

        TEXTURE_DIMENSION   dimension = TEXTURE_DIMENSION_UNKNOWN;
        DXGI_FORMAT         format    = DXGI_FORMAT_UNKNOWN;

        GPU::RESOURCE_FLAG  flags = GPU::RESOURCE_FLAG_NONE;

        u16                 depth             = 0;
        u16                 mip_levels        = 0;
        u16                 subtextures_count = 0;
        u8                  planes_count      = 0;

        StaticString<64>    name;
    };

    struct TextureMemory final
    {
        Array<Texture> textures;

        REV_INLINE TextureMemory(Allocator *allocator) : textures(allocator) {}
        REV_INLINE ~TextureMemory() {}

        REV_DELETE_CONSTRS_AND_OPS(TextureMemory);
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

        REV_DELETE_CONSTRS_AND_OPS(SamplerMemory);
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

        REV_DELETE_CONSTRS_AND_OPS(ResourceMemory);
    };

    class MemoryManager final
    {
    public:
        MemoryManager(Allocator *allocator);
        ~MemoryManager();

        u64 AllocateVertexBuffer(u32 count,                 GPU::RESOURCE_FLAG flags, const ConstString& name = null);
        u64 AllocateIndexBuffer(u32 count,                  GPU::RESOURCE_FLAG flags, const ConstString& name = null);
        u64 AllocateConstantBuffer(u32 bytes,               GPU::RESOURCE_FLAG flags, const ConstString& name = null);
        u64 AllocateStructuredBuffer(u32 count, u32 stride, GPU::RESOURCE_FLAG flags, const ConstString& name = null);
        u64 AllocateByteAddressBuffer(u32 bytes,            GPU::RESOURCE_FLAG flags, const ConstString& name = null);

        u64 AllocateBuffer(u32 bytes, GPU::BUFFER_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name = null);

        u64 AllocateTexture1D(u16   width,                        u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name);
        u64 AllocateTexture2D(u16   width, u16 height,            u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name);
        u64 AllocateTexture3D(u16   width, u16 height, u16 depth, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name);
        u64 AllocateTextureCube(u16 width, u16 height,            u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name);

        u64 AllocateTexture1DArray(u16   width,             u16 count, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name);
        u64 AllocateTexture2DArray(u16   width, u16 height, u16 count, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name);
        u64 AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name);

        u64 AllocateSampler(GPU::TEXTURE_ADDRESS_MODE address_mode, Math::v4 border_color, Math::v2 min_max_lod, bool _static);

        void LoadResources(const ConstArray<GPU::ResourceHandle>& resources);
        void StoreResources(const ConstArray<GPU::ResourceHandle>& resources);

        void ResizeRenderTarget(GPU::ResourceHandle resource, u16 width, u16 height, u16 depth);

        // @NOTE(Roman): Zero copy return. Just a readback pointer.
        const void *GetBufferData(const GPU::ResourceHandle& resource);
        void SetBufferData(const GPU::ResourceHandle& resource, const void *data);

        // @NOTE(Roman): Is pushed onto frame arena.
        GPU::TextureData *GetTextureData(const GPU::ResourceHandle& resource);
        void SetTextureData(const GPU::ResourceHandle& resource, const GPU::TextureData *data);

        void FreeSceneMemory();
        void FreeStaticMemory();

        ConstString GetResourceName(const GPU::ResourceHandle& resource);
        ConstString GetBufferName(const ResourceHandle& resource);
        ConstString GetTextureName(const ResourceHandle& resource);

        #pragma region inline_getters
        REV_INLINE Allocator *GetAllocator() { return m_Allocator; }

        REV_INLINE D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUVirtualAddress(const GPU::ResourceHandle& resource)
        {
            REV_CHECK_M(GPU::MemoryManager::IsBuffer(resource), "Resource is not a buffer");
            if (GPU::MemoryManager::IsStatic(resource))
            {
                const Buffer *buffer = m_StaticMemory.buffer_memory.buffers.GetPointer(resource.index);
                return m_StaticMemory.buffer_memory.default_pages[buffer->default_page_index].gpu_mem->GetGPUVirtualAddress() + buffer->default_page_offset;
            }
            else
            {
                const Buffer *buffer = m_SceneMemory.buffer_memory.buffers.GetPointer(resource.index);
                return m_SceneMemory.buffer_memory.default_pages[buffer->default_page_index].gpu_mem->GetGPUVirtualAddress() + buffer->default_page_offset;
            }
        }

        REV_INLINE D3D12_GPU_VIRTUAL_ADDRESS GetTextureGPUVirtualAddress(const GPU::ResourceHandle& resource)
        {
            REV_CHECK_M(GPU::MemoryManager::IsTexture(resource), "Resource is not a texture");
            return GPU::MemoryManager::IsStatic(resource)
                 ? m_StaticMemory.texture_memory.textures[resource.index].default_gpu_mem->GetGPUVirtualAddress()
                 : m_SceneMemory.texture_memory.textures[resource.index].default_gpu_mem->GetGPUVirtualAddress();
        }

        REV_INLINE ID3D12Resource *GetBufferDefaultGPUMem(const GPU::ResourceHandle& resource)
        {
            REV_CHECK_M(GPU::MemoryManager::IsBuffer(resource), "Resource is not a buffer");
            if (GPU::MemoryManager::IsStatic(resource))
            {
                const Buffer *buffer = m_StaticMemory.buffer_memory.buffers.GetPointer(resource.index);
                return m_StaticMemory.buffer_memory.default_pages[buffer->default_page_index].gpu_mem;
            }
            else
            {
                const Buffer *buffer = m_SceneMemory.buffer_memory.buffers.GetPointer(resource.index);
                return m_SceneMemory.buffer_memory.default_pages[buffer->default_page_index].gpu_mem;
            }
        }

        REV_INLINE Buffer& GetBuffer(const GPU::ResourceHandle& resource)
        {
            REV_CHECK_M(GPU::MemoryManager::IsBuffer(resource), "Resource is not a buffer");
            return GPU::MemoryManager::IsStatic(resource)
                 ? m_StaticMemory.buffer_memory.buffers[resource.index]
                 : m_SceneMemory.buffer_memory.buffers[resource.index];
        }

        REV_INLINE Texture& GetTexture(const GPU::ResourceHandle& resource)
        {
            REV_CHECK_M(GPU::MemoryManager::IsTexture(resource), "Resource is not a texture");
            return GPU::MemoryManager::IsStatic(resource)
                 ? m_StaticMemory.texture_memory.textures[resource.index]
                 : m_SceneMemory.texture_memory.textures[resource.index];
        }

        REV_INLINE Sampler& GetSampler(const GPU::ResourceHandle& resource)
        {
            REV_CHECK_M(GPU::MemoryManager::IsSampler(resource), "Resource is not a sampler");
            return GPU::MemoryManager::IsStatic(resource)
                 ? m_StaticMemory.sampler_memory.samplers[resource.index]
                 : m_SceneMemory.sampler_memory.samplers[resource.index];
        }
        #pragma endregion inline_getters

    private:
        Buffer  *AllocateBuffer(u64 size, DXGI_FORMAT format, D3D12_RESOURCE_STATES initial_state, GPU::RESOURCE_FLAG flags, u64& index, const ConstString& name);
        Texture *AllocateTexture(TEXTURE_DIMENSION dimension, const D3D12_RESOURCE_DESC& desc, GPU::RESOURCE_FLAG flags, u8 planes_count, u64& index, const ConstString& name);

        void UploadResources(ID3D12GraphicsCommandList *command_list, const ConstArray<GPU::ResourceHandle>& resources);
        void ReadbackResources(ID3D12GraphicsCommandList *command_list, const ConstArray<GPU::ResourceHandle>& resources);

        void SetBufferName(BufferMemory *buffer_memory, Buffer *buffer, GPU::RESOURCE_FLAG flags, const ConstString& name);
        void SetTextureName(TextureMemory *texture_memory, Texture *texture, GPU::RESOURCE_FLAG flags, const ConstString& name);

        void FreeMemory(ResourceMemory *memory);

        REV_DELETE_CONSTRS_AND_OPS(MemoryManager);

    private:
        Allocator      *m_Allocator;
        DeviceContext  *m_DeviceContext;
        ResourceMemory  m_StaticMemory;
        ResourceMemory  m_SceneMemory;
    };
}
