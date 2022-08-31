// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/chunk_list.hpp"
#include "graphics/memory_manager.h"

#include "platform/d3d12/d3d12_device_context.h"

// @TODO(Roman): Thread safety

#ifndef D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION
#define D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION (D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION / 6)
#endif

namespace REV
{
    namespace D3D12
    {
        struct Buffer
        {
            DefaultBufferMemoryPage  *default_page  = null;
            UploadBufferMemoryPage   *upload_page   = null;
            ReadBackBufferMemoryPage *readback_page = null;

            u64 default_page_offset  = REV_INVALID_U64_OFFSET;
            u64 upload_page_offset   = REV_INVALID_U64_OFFSET;
            u64 readback_page_offset = REV_INVALID_U64_OFFSET;

            u64           size       = 0;
            u32           stride     = 0;
            RESOURCE_FLAG flags      = RESOURCE_FLAG_NONE;

            // @NOTE(Roman): A handle to this buffer. We have to store it here
            //               to invalidate it on buffer destruction.
            // @TODO(Roman): #MultipleHandlesToTheSameResource
            ResourceHandle handle;

            StaticString<64> name;
        };

        enum
        {
            DEFAULT_BUFFER_MEMORY_PAGE_SIZE  = AlignUp(MB(16), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
            UPLOAD_BUFFER_MEMORY_PAGE_SIZE   = AlignUp(MB(16), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
            READBACK_BUFFER_MEMORY_PAGE_SIZE = AlignUp(MB(16), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
        };

        struct DefaultBufferMemoryPage
        {
            ID3D12Resource        *gpu_mem        = null;
            u64                    occupied_bytes = 0;
            D3D12_RESOURCE_STATES  default_state  = D3D12_RESOURCE_STATE_COMMON;
        };

        struct UploadBufferMemoryPage
        {
            ID3D12Resource *gpu_mem                          = null;
            byte           *cpu_mem                          = null;
            u64             occupied_bytes                   = 0;
            u64             upload_once_resources_left_count = 0;
            bool            upload_once                      = false;
        };

        struct ReadBackBufferMemoryPage
        {
            ID3D12Resource *gpu_mem                            = null;
            byte           *cpu_mem                            = null;
            u64             occupied_bytes                     = 0;
            u64             readback_once_resources_left_count = 0;
            bool            readback_once                      = false;
        };

        struct BufferMemory final
        {
            ChunkList<Buffer>                   buffers;
            ChunkList<DefaultBufferMemoryPage>  default_pages;
            ChunkList<UploadBufferMemoryPage>   upload_pages;
            ChunkList<ReadBackBufferMemoryPage> readback_pages;

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

        struct Texture final
        {
            ID3D12Resource        *default_gpu_mem      = null;

            ID3D12Resource        *upload_gpu_mem       = null;
            byte                  *upload_cpu_mem       = null;

            ID3D12Resource        *readback_gpu_mem     = null;
            byte                  *readback_cpu_mem     = null;

            u64                    first_surface_offset = 0;
            u64                    buffer_total_bytes   = 0;

            DXGI_FORMAT            format               = DXGI_FORMAT_UNKNOWN;
            D3D12_RESOURCE_STATES  default_state        = D3D12_RESOURCE_STATE_COMMON;

            RESOURCE_FLAG          flags                = RESOURCE_FLAG_NONE;

            u16                    depth                = 0;
            u16                    mip_levels           = 1;
            u16                    array_size           = 0;
            u8                     planes_count         = 1;

            TEXTURE_DIMENSION      dimension            = TEXTURE_DIMENSION_UNKNOWN;

            // @NOTE(Roman): A handle to this texture. We have to store it here
            //               to invalidate it on texture destruction.
            // @TODO(Roman): #MultipleHandlesToTheSameResource
            ResourceHandle         handle = null;

            StaticString<64>       name;
        };

        struct TextureMemory final
        {
            ChunkList<Texture> textures;

            REV_INLINE TextureMemory(Allocator *allocator) : textures(allocator) {}
            REV_INLINE ~TextureMemory() {}

            REV_DELETE_CONSTRS_AND_OPS(TextureMemory);
        };

        struct Sampler final
        {
            // @TODO(Roman): Replace with REV::SamplerDesc?
            D3D12_SAMPLER_DESC desc  = {};

            RESOURCE_FLAG      flags = RESOURCE_FLAG_NONE;

            // @NOTE(Roman): A handle to this sampler. We have to store it here
            //               to invalidate it on sampler destruction.
            // @TODO(Roman): #MultipleHandlesToTheSameResource
            ResourceHandle     handle = null;

            StaticString<64>   name;
        };

        struct SamplerMemory final
        {
            ChunkList<Sampler> samplers;

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

            //==================== Interface stuff ====================

            const ResourceHandle& AllocateBuffer(const BufferDesc& desc, const ConstString& name = null);
            const ResourceHandle& AllocateTexture(TextureDesc& desc, const ConstString& name = null);
            const ResourceHandle& AllocateSampler(const SamplerDesc& desc, const ConstString& name = null);

            void UpdateBuffer(const ResourceHandle& resource, const void *data);
            void UpdateTexture(const ResourceHandle& resource, const TextureData& data);

            void *ReadBuffer(const ResourceHandle& resource);
            TextureData ReadTexture(const ResourceHandle& resource);

            void UploadResources(const ConstArray<ResourceHandle>& resources);
            void ReadbackResources(const ConstArray<ResourceHandle>& resources);

            void ResizeBuffer(ResourceHandle resource, u32 bytes);
            void ResizeTexture(ResourceHandle resource, u16 width, u16 height = 0, u16 depth = 0);

            void FreePerFrameMemory();
            void FreePerSceneMemory();
            void FreePermanentMemory();

            ConstString GetResourceName(const ResourceHandle& resource);
            ConstString GetBufferName(const ResourceHandle& buffer);
            ConstString GetTextureName(const ResourceHandle& texture);
            ConstString GetSamplerName(const ResourceHandle& sampler);

            //==================== Interanl stuff ====================

            D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUVirtualAddress(const ResourceHandle& resource);
            D3D12_GPU_VIRTUAL_ADDRESS GetBufferGPUVirtualAddress(const ResourceHandle& resource);
            D3D12_GPU_VIRTUAL_ADDRESS GetTextureGPUVirtualAddress(const ResourceHandle& resource);

            ID3D12Resource *GetBufferDefaultGPUMem(const ResourceHandle& resource);

            Buffer  *GetBuffer(const ResourceHandle& resource);
            Texture *GetTexture(const ResourceHandle& resource);
            Sampler *GetSampler(const ResourceHandle& resource);

        private:
            void SetBufferName(Buffer *buffer);
            void SetTextureName(Texture *texture);

            void FreeMemory(ResourceMemory *memory);

            D3D12_CLEAR_VALUE GetOptimizedClearValue(RESOURCE_FLAG flags, TEXTURE_FORMAT format);
            ResourceMemory *GetResourceMemory(RESOURCE_FLAG flags, const ConstString& name = null);
            D3D12_RESOURCE_STATES GetResourceDefaultState(RESOURCE_KIND kind, RESOURCE_FLAG flags);
            D3D12_RESOURCE_FLAGS GetResourceFlags(RESOURCE_KIND kind, RESOURCE_FLAG flags);

            REV_DELETE_CONSTRS_AND_OPS(MemoryManager);

        private:
            Allocator      *m_Allocator;
            DeviceContext  *m_DeviceContext;
            ResourceMemory  m_PerFrameMemory;
            ResourceMemory  m_PerSceneMemory;
            ResourceMemory  m_PermanentMemory;
        };
    }
}

#include "platform/d3d12/d3d12_memory_manager.inl"
