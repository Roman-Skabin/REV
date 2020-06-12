//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "gpu/gpu_manager.h"

// @Optimize(Roman): Free list for descriptors.
//                   Free list for resources?

struct GPUMemoryBlock
{
    // @TODO(Roman): Not safety: memory block can be created
    //               on stack in some function => after function
    //               completed <next> pointer will be invaild.
    GPUMemoryBlock        *next;
    ID3D12Resource        *resource;
    ID3D12DescriptorHeap  *desc;
    u32                    kind;
    union
    {
        struct { u32 width, height; u16 depth; };
        struct { u32 count, stride; };
    };
    D3D12_RESOURCE_BARRIER barrier;
    D3D12_RESOURCE_STATES  state;
};

typedef enum GPU_MEMORY
{
    GPU_MEMORY_VB = 1,
    GPU_MEMORY_IB,
    GPU_MEMORY_CBV,
    GPU_MEMORY_SRV,
    GPU_MEMORY_UAV,
    GPU_MEMORY_SAMPLER,
    GPU_MEMORY_COPY,
    GPU_MEMORY_ALL,
} GPU_MEMORY;

typedef struct GPUMemory
{
    ID3D12Heap     *heap;
    u64             offset;
    u64             capacity;
    // @TODO(Roman): Not safety: memory block can be created
    //               on stack in some function => after function
    //               completed <first> pointer will be invaild.
    GPUMemoryBlock *first;
} GPUMemory;

typedef struct GPUMemoryManager
{
    GPUMemory vb_memory;
    GPUMemory ib_memory;
    GPUMemory cbv_memory;
    GPUMemory srv_memory;
    GPUMemory uav_memory;
    GPUMemory sampler_memory;

    GPUMemory copy_memory[SWAP_CHAIN_BUFFERS_COUNT]; // being cleared every frame

    HRESULT   error;
} GPUMemoryManager;

CENGINE_FUN void ClearGPUMemory(
    IN Engine     *engine,
    IN GPU_MEMORY  memory_kind
);

CENGINE_FUN void PushVB(
    IN  Engine         *engine,
    IN  u32             count,
    IN  u32             stride,
    OUT GPUMemoryBlock *buffer
);

CENGINE_FUN void PushIB(
    IN  Engine         *engine,
    IN  u32             count,
    OUT GPUMemoryBlock *buffer
);

CENGINE_FUN void PushCBV(
    IN  Engine         *engine,
    IN  u32             size_in_bytes,
    OUT GPUMemoryBlock *buffer
);

// @TODO(Roman): PushSRV, PushUAV, PushSampler

CENGINE_FUN void SetGPUMemoryBlockData(
    IN Engine           *engine,
    IN GPUMemoryBlock   *buffer,
    IN void             *data
);

CENGINE_FUN void BindVB(
    IN Engine         *engine,
    IN GPUMemoryBlock *buffer
);

CENGINE_FUN void BindIB(
    IN Engine         *engine,
    IN GPUMemoryBlock *buffer
);

CENGINE_FUN void DrawVertices(
    IN Engine         *engine,
    IN GPUMemoryBlock *buffer
);

CENGINE_FUN void DrawIndices(
    IN Engine         *engine,
    IN GPUMemoryBlock *buffer
);
