//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "renderer/gpu_memory_manager.h"

#include "core/memory.h"
#include "core/allocator.h"

#include "tools/list.hpp"

#include "platform/d3d12_renderer.h"

namespace D3D12
{
    class GPUDescHeap;

    class GPUResource final : public IGPUResource
    {
    public:
        enum class KIND
        {
            UNKNOWN,
            VB,
            IB,
            CB,
            SR,
            UA, // Gotta be OR'd with anothers?
            SAMPLER,
        };

        enum class ALLOCATION_STATE
        {
            NONE,
            ALLOCATED,
            IN_FREE_LIST,
        };

        GPUResource(const char *name, u32 name_length);
        ~GPUResource();

    private:
        ID3D12Resource *m_DefaultResource;

        ID3D12Resource *m_UploadResources[SWAP_CHAIN_BUFFERS_COUNT];
        byte           *m_UploadPointers[SWAP_CHAIN_BUFFERS_COUNT];

        KIND             m_Kind;
        ALLOCATION_STATE m_AllocationState;

        GPUDescHeap *m_DescHeap;

        u32 m_VertexIndexCount;
        u32 m_VertexIndexStride;

        D3D12_RESOURCE_STATES m_InitialState;

        u32  m_NameLen;
        char m_Name[64];

        D3D12_RESOURCE_DESC m_ResourceDesc;
    };

    class GPUDescHeap final
    {
    public:
        enum class ALLOCATION_STATE
        {
            NONE,
            ALLOCATED,
            IN_FREE_LIST,
        };

        GPUDescHeap();
        ~GPUDescHeap();

    private:
        ID3D12DescriptorHeap       *m_Handle;
        GPUResource                *m_Resource;
        D3D12_DESCRIPTOR_HEAP_DESC  m_DescHeapDesc;
        ALLOCATION_STATE            m_AllocationState;
    };

    class GPUResourceMemory final
    {
    public:
        GPUResourceMemory(Allocator *allocator);
        ~GPUResourceMemory();

    private:
        List<GPUResource> m_Resources;

        ID3D12Heap      *m_DefaultHeap;
        u64              m_DefaultOffset;
        u64              m_DefaultCapacity; // @Cleanup(Roman): Redundant?

        ID3D12Heap      *m_UploadHeap[SWAP_CHAIN_BUFFERS_COUNT];
        u64              m_UploadOffset[SWAP_CHAIN_BUFFERS_COUNT];
        u64              m_UploadCapacity[SWAP_CHAIN_BUFFERS_COUNT]; // @Cleanup(Roman): Redundant?
    };

    class GPUDescHeapMemory final
    {
    public:
        GPUDescHeapMemory(Allocator *allocator);
        ~GPUDescHeapMemory();

    private:
        List<GPUDescHeap> m_DescHeaps;
    };

    class GPUMemoryManager final : public IGPUMemoryManager
    {
    public:
        GPUMemoryManager(Allocator *allocator);
        ~GPUMemoryManager();

        virtual void Destroy() override;

        void *operator new(size_t) { return Memory::Get()->PushToPA<GPUMemoryManager>(); }
        void  operator delete(void *) {}

    private:
        GPUMemoryManager(const GPUMemoryManager&) = delete;
        GPUMemoryManager(GPUMemoryManager&&)      = delete;

        GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;
        GPUMemoryManager& operator=(GPUMemoryManager&&)      = delete;

    private:
        Allocator        *m_Allocator;

        GPUDescHeapMemory m_DescHeapMemory;

        GPUResourceMemory m_BufferMemory;
        GPUResourceMemory m_TextureMemory;

        HRESULT           m_Error;
    };
};
