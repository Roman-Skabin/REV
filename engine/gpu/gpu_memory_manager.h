//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "gpu/gpu_manager.h"
#include "core/allocator.h"

// @TODO(Roman): Aliased (overlapped) resources. For transient memory at least for now.
// @TODO(Roman): Arrays as a single memory block, e.g. array of 2D textures.
// @TODO(Roman): Check memory capacity before pushing resources and descriptors
// @TODO(Roman): Push ShaderResource [buffer|texture],
//               Push UnorderedAccess [buffer|texture],
//               Push Sampler
// @TOOD(Roman): Move Draw (and Bind probably) stuff somewhere.
//               This functionality doesn't belong to memory management.
// @TODO(Roman): LIST GPUResource *free_list in GPUResourceMemory.
//               Just saved pointers to freed resources.
// @TODO(Roman): Add ID3D12Fences to the GPUResoucre to be shure
//               the copy operation is completed.

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

    ID3D12Resource *default_resource;

    ID3D12Resource *upload_resources[SWAP_CHAIN_BUFFERS_COUNT];
    byte           *upload_pointers[SWAP_CHAIN_BUFFERS_COUNT];

    GPU_RESOURCE_KIND             kind;
    GPU_RESOURCE_ALLOCATION_STATE allocation_state;

    GPUDescHeap *desc_heap;

    u32 vertex_index_count;
    u32 vertex_index_stride;

    D3D12_RESOURCE_STATES initial_state;

    u32  name_len;
    char name[64];

    D3D12_RESOURCE_DESC resource_desc;
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

    ID3D12Heap      *default_heap;
    u64              default_offset;
    u64              default_capacity; // @Cleanup(Roman): Redundant?

    ID3D12Heap      *upload_heap[SWAP_CHAIN_BUFFERS_COUNT];
    u64              upload_offset[SWAP_CHAIN_BUFFERS_COUNT];
    u64              upload_capacity[SWAP_CHAIN_BUFFERS_COUNT]; // @Cleanup(Roman): Redundant?
} GPUResourceMemory;

typedef struct GPUDescHeapMemory
{
    LIST GPUDescHeap *desc_heaps;
} GPUDescHeapMemory;

typedef struct GPUMemoryManager
{
    Allocator        *allocator;

    GPUDescHeapMemory desc_heap_memory;

    GPUResourceMemory buffer_memory;
    GPUResourceMemory texture_memory;

    HRESULT           error;
} GPUMemoryManager;

ENGINE_FUN void ResetGPUMemory(
    in Engine *engine
);

ENGINE_FUN void ReleaseGPUMemory(
    in Engine *engine
);

ENGINE_FUN GPUResource *PushVertexBuffer(
    in Engine *engine,
    in u32     count,
    in u32     stride
);

ENGINE_FUN GPUResource *PushIndexBuffer(
    in Engine *engine,
    in u32     count
);

// @NOTE(Roman): It would be better if you pass non-null name_len,
//               otherwise strlen will be used.
ENGINE_FUN GPUResource *PushConstantBuffer(
    in  Engine     *engine,
    in  u32         size_in_bytes,
    in  const char *name,
    opt u32         name_len
);

// @NOTE(Roman): It would be better if you pass non-null name_len,
//               otherwise strlen will be used.
ENGINE_FUN void SetGPUResourceName(
    in  Engine      *engine,
    in  GPUResource *resource,
    in  const char  *name,
    opt u32          name_len
);

ENGINE_FUN void SetGPUResourceData(
    in Engine      *engine,
    in GPUResource *resource,
    in void        *data
);

// @NOTE(Roman): It would be better if you pass non-null name_len,
//               otherwise strlen will be used.
ENGINE_FUN void SetGPUResourceDataByName(
    in  Engine     *engine,
    in  const char *name,
    opt u32         name_len,
    in  void       *data
);

ENGINE_FUN void BindVertexBuffer(
    in Engine      *engine,
    in GPUResource *resource
);

ENGINE_FUN void BindIndxeBuffer(
    in Engine      *engine,
    in GPUResource *resource
);

ENGINE_FUN void DrawVertices(
    in Engine      *engine,
    in GPUResource *resource
);

ENGINE_FUN void DrawIndices(
    in Engine      *engine,
    in GPUResource *resource
);
