//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "renderer/gpu_memory_manager.h"
#include "core/memory.h"
#include "core/allocator.h"
#include "tools/list.hpp"
#include "platform/d3d12_renderer.h"

// @TODO(Roman): Aliased (overlapped) resources?
// @TODO(Roman): Arrays as a single memory block, e.g. array of 2D textures?
// @TODO(Roman): Check memory capacity before pushing resources and descriptors
// @TODO(Roman): AllocateSR[Buffer|Texture],
//               AllocateUA[Buffer|Texture],
//               AllocateSampler
// @TOOD(Roman): Move Draw (and Bind probably) stuff somewhere.
//               This functionality doesn't belong to memory management.

// @Issue(Roman): It's not efficient to preallocate so much memory
//                even for using the fastest way of resource allocation D3D12
//                (Using placed resources with preallocated heaps for them).
//                Because ID3D12Heap is an interface used to manage PHYSICAL memory, NOT virtual.
//                And when we trying to allocate x bytes we're commiting ALL the memory we want to reserve.
//
//                Double Bufferisation:
//                     1 GB ->  3 GB
//                     2 GB ->  6 GB
//                     4 GB -> 12 GB
//                     8 GB -> 24 GB
//                    16 GB -> 48 GB
//                    24 GB -> 72 GB
//
//                Triple Bufferisation:
//                     1 GB ->  4 GB
//                     2 GB ->  8 GB
//                     4 GB -> 16 GB
//                     8 GB -> 32 GB
//                    16 GB -> 64 GB
//                    24 GB -> 96 GB
//
//                Placed resources can be overlapped, so if we'll find a way
//                to "place" upload resources from different frames into one memory,
//                entire_bytes will be equals to:
//
//                                      buffers         textures
//                                     def    upl      def    upl
//                     entire_bytes = (0.4x + 0.4x) + (0.6x + 0.6x).
//                     entire_bytes = 2x;
//
//                And it's seems much better than it is now:
//
//                     N = D3D12::SWAP_CHAIN_BUFFERS_COUNT.
//                                       buffers          textures
//                                     def     upl       def    upl
//                     entire_bytes = (0.4x + 0.4Nx) + (0.6x + 0.6Nx).
//                     entire_bytes = (1+N)x.
//
//                The first variant is to use fences to check is our work from previous frame with resource is completed.
//                But won't it slow down all the performance I want to achieve?

namespace D3D12
{
    class GPUDescHeap;

    class ENGINE_API GPUResource final : public IGPUResource
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

        GPUResource();
        ~GPUResource();

        virtual void SetName(const StaticString<64>& name) override;
        virtual void SetData(const void *data) override;
        virtual void SetDataImmediate(const void *data) override;

        constexpr const ID3D12Resource      *Handle()       const { return m_DefaultResource;   }
        constexpr const GPUDescHeap         *DescHeap()     const { return m_DescHeap;          }
        constexpr       KIND                 Kind()         const { return m_Kind;              }
        constexpr const StaticString<64>&    Name()         const { return m_Name;              }
        constexpr       u32                  VertexCount()  const { return m_VertexIndexCount;  }
        constexpr       u32                  IndexCount()   const { return m_VertexIndexCount;  }
        constexpr       u32                  VertexStride() const { return m_VertexIndexStride; }
        constexpr       u32                  IndexStride()  const { return m_VertexIndexStride; }
        constexpr const D3D12_RESOURCE_DESC& Desc()         const { return m_ResourceDesc;      }

        constexpr ID3D12Resource      *Handle()   { return m_DefaultResource; }
        constexpr GPUDescHeap         *DescHeap() { return m_DescHeap;        }
        constexpr D3D12_RESOURCE_DESC& Desc()     { return m_ResourceDesc;    }

        GPUResource& operator=(GPUResource&& other) noexcept;

    private:
        void StartImmediateExecution();
        void EndImmediateExecution();

        GPUResource(const GPUResource&) = delete;
        GPUResource(GPUResource&&)      = delete;

        GPUResource& operator=(const GPUResource&) = delete;

    private:
        ID3D12Resource            *m_DefaultResource;
        ID3D12Resource            *m_UploadResources[SWAP_CHAIN_BUFFERS_COUNT];
        void                      *m_UploadPointers[SWAP_CHAIN_BUFFERS_COUNT];
        KIND                       m_Kind;
        ALLOCATION_STATE           m_AllocationState;
        GPUDescHeap               *m_DescHeap;
        u32                        m_VertexIndexCount;
        u32                        m_VertexIndexStride;
        D3D12_RESOURCE_STATES      m_InitialState;
        StaticString<64>           m_Name;
        GPUResource               *m_NextFree;
        ID3D12CommandAllocator    *m_CommandAllocator;
        ID3D12GraphicsCommandList *m_CommandList;
        ID3D12Fence               *m_Fence;
        Event                      m_FenceEvent;
        D3D12_RESOURCE_DESC        m_ResourceDesc;

        friend class GPUResourceMemory;
        friend class GPUDescHeapMemory;
        friend class GPUMemoryManager;
    };

    class ENGINE_API GPUDescHeap final
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

        constexpr const ID3D12DescriptorHeap *Handle()   const { return m_Handle;   }
        constexpr const GPUResource          *Resource() const { return m_Resource; }

        constexpr ID3D12DescriptorHeap *Handle()   { return m_Handle;   }
        constexpr GPUResource          *Resource() { return m_Resource; }

        GPUDescHeap& operator=(GPUDescHeap&& other) noexcept;

    private:
        GPUDescHeap(const GPUDescHeap&) = delete;
        GPUDescHeap(GPUDescHeap&&)      = delete;

        GPUDescHeap& operator=(const GPUDescHeap&) = delete;

    private:
        ID3D12DescriptorHeap       *m_Handle;
        GPUResource                *m_Resource;
        GPUDescHeap                *m_NextFree;
        ALLOCATION_STATE            m_AllocationState;
        D3D12_DESCRIPTOR_HEAP_DESC  m_DescHeapDesc;

        friend class GPUResourceMemory;
        friend class GPUDescHeapMemory;
        friend class GPUMemoryManager;
    };

    class ENGINE_API GPUResourceMemory final
    {
    public:
        using GPUResourceList = List<GPUResource, true>;

        GPUResourceMemory(Allocator *allocator);
        ~GPUResourceMemory();

        void Create(const D3D12_HEAP_DESC& default_desc, const D3D12_HEAP_DESC& upload_desc, bool allow_only_buffers);

        void Reset();
        void Release();

        GPUResource *FindOrAllocateResource(const D3D12_RESOURCE_DESC& resource_desc, bool& allocated);
        void         CreateGPUResource(GPUResource *resource);

        GPUResource *FindGPUResource(const StaticString<64>& name);
    
    private:
        GPUResourceMemory(const GPUResourceMemory&) = delete;
        GPUResourceMemory(GPUResourceMemory&&)      = delete;

        GPUResourceMemory& operator=(const GPUResourceMemory&) = delete;
        GPUResourceMemory& operator=(GPUResourceMemory&&)      = delete;

    private:
        GPUResourceList  m_Resources;

        GPUResource     *m_FreeList;

        ID3D12Heap      *m_DefaultHeap;
        u64              m_DefaultOffset;
        u64              m_DefaultCapacity; // @Cleanup(Roman): Redundant?

        ID3D12Heap      *m_UploadHeaps[SWAP_CHAIN_BUFFERS_COUNT];
        u64              m_UploadOffsets[SWAP_CHAIN_BUFFERS_COUNT];
        u64              m_UploadCapacities[SWAP_CHAIN_BUFFERS_COUNT]; // @Cleanup(Roman): Redundant?

        bool             m_AllowOnlyBuffers;
    };

    class ENGINE_API GPUDescHeapMemory final
    {
    public:
        using GPUDescHeapList = List<GPUDescHeap>;

        GPUDescHeapMemory(Allocator *allocator);
        ~GPUDescHeapMemory();

        void Release();

        GPUDescHeap *FindOrAllocateDescHeap(const D3D12_DESCRIPTOR_HEAP_DESC& desc_heap_desc, bool& allocated);
        void         CreateDescHeapForCB(GPUDescHeap *desc_heap, GPUResource *resource);
    
    private:
        GPUDescHeapMemory(const GPUDescHeapMemory&) = delete;
        GPUDescHeapMemory(GPUDescHeapMemory&&)      = delete;

        GPUDescHeapMemory& operator=(const GPUDescHeapMemory&) = delete;
        GPUDescHeapMemory& operator=(GPUDescHeapMemory&&)      = delete;

    private:
        GPUDescHeapList  m_DescHeaps;
        GPUDescHeap     *m_FreeList;
    };

    class ENGINE_API GPUMemoryManager final : public IGPUMemoryManager
    {
    public:
        GPUMemoryManager(Allocator *allocator, const Logger& logger, u64 gpu_memory_capacity);
        ~GPUMemoryManager();

        virtual void Destroy() override;

        virtual IGPUResource *AllocateVB(u32 vertex_count, u32 stride)            override;
        virtual IGPUResource *AllocateIB(u32 indecies_count)                      override;
        virtual IGPUResource *AllocateCB(u32 bytes, const StaticString<64>& name) override;

        virtual void SetGPUResourceData(const StaticString<64>& name, const void *data) override;
        virtual void SetGPUResourceDataImmediate(const StaticString<64>& name, const void *data) override;

        virtual void Reset() override;
        virtual void Release() override;

        void *operator new(size_t) { return Memory::Get()->PushToPA<GPUMemoryManager>(); }
        void  operator delete(void *) {}

    private:
        GPUMemoryManager(const GPUMemoryManager&) = delete;
        GPUMemoryManager(GPUMemoryManager&&)      = delete;

        GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;
        GPUMemoryManager& operator=(GPUMemoryManager&&)      = delete;

    private:
        Allocator        *m_Allocator;
        Logger            m_Logger;
        GPUDescHeapMemory m_DescHeapMemory;
        GPUResourceMemory m_BufferMemory;
        GPUResourceMemory m_TextureMemory;
    };
};
