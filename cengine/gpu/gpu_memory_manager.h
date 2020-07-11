//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "gpu/gpu_manager.h"
#include "core/allocator.h"

// @TODO(Roman): Aliased (overlapped) resources
// @TODO(Roman): Arrays as a single memory block
// @TODO(Roman): Check memory capacity before pushing resources and descriptors
// @TODO(Roman): Push ShaderResource [buffer|texture],
//               Push UnorderedAccess [buffer|texture],
//               Push Sampler
// @TOOD(Roman): Move Draw (and Bind probably) stuff somewhere.
//               This functionality doesn't belong to memory management.
// @TODO(Roman): One common enum for resource and descriptor heap
//               allocation states. Short name required.

typedef enum GPU_RESOURCE_ALLOCATION_STATE
{
    GPU_RESOURCE_ALLOCATION_STATE_NONE,
    GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED,
    GPU_RESOURCE_ALLOCATION_STATE_IN_FREE_LIST,
} GPU_RESOURCE_ALLOCATION_STATE;

typedef enum GPU_RESOURCE_KIND
{
    GPU_RESOURCE_KIND_UNKNOWN,
    GPU_RESOURCE_KIND_VB,
    GPU_RESOURCE_KIND_IB,
    GPU_RESOURCE_KIND_CB,
    GPU_RESOURCE_KIND_SR,
    GPU_RESOURCE_KIND_UA, // Gotta be OR'd with anothers?
    GPU_RESOURCE_KIND_SAMPLER,
} GPU_RESOURCE_KIND;

typedef struct GPUResource GPUResource;
typedef struct GPUDescHeap GPUDescHeap;

struct GPUResource
{
    ExtendsList(GPUResource);
    ID3D12Resource                *resource;
    char                           name[64];
    u64                            name_len;
    GPUDescHeap                   *desc_heap;
    GPU_RESOURCE_ALLOCATION_STATE  allocation_state;
    GPU_RESOURCE_KIND              kind;
    u32                            vi_count;
    u32                            vi_stride;
    D3D12_RESOURCE_STATES          initial_state;
    D3D12_RESOURCE_DESC            resource_desc;
};

typedef enum GPU_DESC_HEAP_ALLOCATION_STATE
{
    GPU_DESC_HEAP_ALLOCATION_STATE_NONE,
    GPU_DESC_HEAP_ALLOCATION_STATE_ALLOCATED,
    GPU_DESC_HEAP_ALLOCATION_STATE_IN_FREE_LIST,
} GPU_DESC_HEAP_ALLOCATION_STATE;

struct GPUDescHeap
{
    ExtendsList(GPUDescHeap);
    ID3D12DescriptorHeap           *desc_heap;
    GPUResource                    *resource;
    D3D12_DESCRIPTOR_HEAP_DESC      desc_heap_desc;
    GPU_DESC_HEAP_ALLOCATION_STATE  allocation_state;
};

typedef struct GPUResourceMemory
{
    LIST GPUResource *resources;

    ID3D12Heap  *gpu_heap;
    u64          gpu_heap_offset;
    u64          gpu_heap_capacity;
} GPUResourceMemory;

typedef struct GPUDescHeapMemory
{
    LIST GPUDescHeap *desc_heaps;
} GPUDescHeapMemory;

typedef struct GPUMemoryManager
{
    Allocator         *allocator;
    GPUResourceMemory  permanent_memory;
    GPUDescHeapMemory  desc_heap_memory;
    GPUResourceMemory  transient_memory[SWAP_CHAIN_BUFFERS_COUNT];
    HRESULT            error;
} GPUMemoryManager;

typedef enum GPU_MEMORY_TYPE
{
    GPU_MEMORY_TYPE_PERMANENT         = BIT(0),
    GPU_MEMORY_TYPE_TRANSIENT_CURRENT = BIT(1), // Can't be OR'd with GPU_MEMORY_TYPE_TRANSIENT_ALL
    GPU_MEMORY_TYPE_TRANSIENT_ALL     = BIT(2), // Can't be OR'd with GPU_MEMORY_TYPE_TRANSIENT_CURRENT
} GPU_MEMORY_TYPE;

CENGINE_FUN void ResetGPUMemory(
    IN Engine          *engine,
    IN GPU_MEMORY_TYPE  type
);

CENGINE_FUN void ReleaseGPUMemory(
    IN Engine          *engine,
    IN GPU_MEMORY_TYPE  type
);

CENGINE_FUN GPUResource *PushVertexBuffer(
    IN Engine *engine,
    IN u32     count,
    IN u32     stride
);

CENGINE_FUN GPUResource *PushIndexBuffer(
    IN Engine *engine,
    IN u32     count
);

CENGINE_FUN GPUResource *PushConstantBuffer(
    IN Engine     *engine,
    IN u32         size_in_bytes,
    IN const char *name,
    IN u64         name_len
);

CENGINE_FUN void SetGPUResourceData(
    IN Engine      *engine,
    IN GPUResource *resource,
    IN void        *data
);

CENGINE_FUN void SetGPUResourceDataByName(
    IN Engine      *engine,
    IN const char  *name,
    IN u64          name_len,
    IN void        *data
);

CENGINE_FUN void BindVertexBuffer(
    IN Engine      *engine,
    IN GPUResource *resource
);

CENGINE_FUN void BindIndxeBuffer(
    IN Engine      *engine,
    IN GPUResource *resource
);

CENGINE_FUN void DrawVertices(
    IN Engine      *engine,
    IN GPUResource *resource
);

CENGINE_FUN void DrawIndices(
    IN Engine      *engine,
    IN GPUResource *resource
);
