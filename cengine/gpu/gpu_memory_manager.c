//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "gpu/gpu_memory_manager.h"
#include "cengine.h"

enum GPU_MEMORY_BLOCK_KIND
{
    GPU_MEMORY_BLOCK_KIND_VB  = 1,
    GPU_MEMORY_BLOCK_KIND_IB,
    GPU_MEMORY_BLOCK_KIND_CBV,
    GPU_MEMORY_BLOCK_KIND_SRV,
    GPU_MEMORY_BLOCK_KIND_UAV,
    GPU_MEMORY_BLOCK_KIND_SAMPLER,
};

#define SafeRelease(directx_interface)                           \
{                                                                \
    if (directx_interface)                                       \
    {                                                            \
        (directx_interface)->lpVtbl->Release(directx_interface); \
        (directx_interface) = 0;                                 \
    }                                                            \
}

#define ClearDefaultGPUMemory(_engine, _gpu_memory_manager_field)                            \
{                                                                                            \
    for (GPUMemoryBlock *it = (_engine)->gpu_memory_manager._gpu_memory_manager_field.first; \
         it;                                                                                 \
         it = it->next)                                                                      \
    {                                                                                        \
        SafeRelease(it->resource);                                                           \
        SafeRelease(it->desc);                                                               \
    }                                                                                        \
    (_engine)->gpu_memory_manager._gpu_memory_manager_field.first = 0;                       \
    (_engine)->gpu_memory_manager._gpu_memory_manager_field.offset = 0;                      \
}

void ClearGPUMemory(
    IN Engine     *engine,
    IN GPU_MEMORY  memory_kind)
{
    switch (memory_kind)
    {
        case GPU_MEMORY_VB:
        {
            ClearDefaultGPUMemory(engine, vb_memory);
        } break;

        case GPU_MEMORY_IB:
        {
            ClearDefaultGPUMemory(engine, ib_memory);
        } break;

        case GPU_MEMORY_CBV:
        {
            ClearDefaultGPUMemory(engine, cbv_memory);
        } break;

        case GPU_MEMORY_SRV:
        {
            ClearDefaultGPUMemory(engine, srv_memory);
        } break;

        case GPU_MEMORY_UAV:
        {
            ClearDefaultGPUMemory(engine, uav_memory);
        } break;

        case GPU_MEMORY_SAMPLER:
        {
            ClearDefaultGPUMemory(engine, sampler_memory);
        } break;

        case GPU_MEMORY_COPY:
        {
            GPUMemory *current_copy_memory = engine->gpu_memory_manager.copy_memory + engine->gpu_manager.current_buffer;

            for (GPUMemoryBlock *it = current_copy_memory->first; it; it = it->next)
            {
                SafeRelease(it->resource);
                SafeRelease(it->desc);

                GPUMemoryBlock *temp = it;
                DeAlloc(&engine->allocator, temp);
            }
            current_copy_memory->first  = 0;
            current_copy_memory->offset = 0;
        } break;

        case GPU_MEMORY_ALL:
        {
            ClearDefaultGPUMemory(engine, vb_memory);
            ClearDefaultGPUMemory(engine, ib_memory);
            ClearDefaultGPUMemory(engine, cbv_memory);
            ClearDefaultGPUMemory(engine, srv_memory);
            ClearDefaultGPUMemory(engine, uav_memory);
            ClearDefaultGPUMemory(engine, sampler_memory);

            for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
            {
                GPUMemory *current_copy_memory = engine->gpu_memory_manager.copy_memory + i;
                for (GPUMemoryBlock *it = current_copy_memory->first; it; it = it->next)
                {
                    SafeRelease(it->resource);
                    SafeRelease(it->desc);

                    GPUMemoryBlock *temp = it;
                    DeAlloc(&engine->allocator, temp);
                }
                current_copy_memory->first  = 0;
                current_copy_memory->offset = 0;
            }
        } break;
    }
}

void PushVB(
    IN  Engine         *engine,
    IN  u32             count,
    IN  u32             stride,
    OUT GPUMemoryBlock *buffer)
{
    Check(engine);
    Check(count);
    Check(stride);
    Check(buffer);

    buffer->next     = 0;
    buffer->resource = 0;
    buffer->desc     = 0;
    buffer->kind     = GPU_MEMORY_BLOCK_KIND_VB;
    buffer->count    = count;
    buffer->stride   = stride;
    buffer->barrier  = (D3D12_RESOURCE_BARRIER){0};
    buffer->state    = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = buffer->count * buffer->stride;
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                engine->gpu_memory_manager.vb_memory.heap,
                                                                                                engine->gpu_memory_manager.vb_memory.offset,
                                                                                                &resource_desc,
                                                                                                buffer->state,
                                                                                                0,
                                                                                                &IID_ID3D12Resource,
                                                                                                &buffer->resource);
    CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Vertex Buffer Memory Overflow");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    buffer->barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    buffer->barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    buffer->barrier.Transition.pResource   = buffer->resource;
    buffer->barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    buffer->barrier.Transition.StateBefore = buffer->state;
    buffer->barrier.Transition.StateAfter  = buffer->state;

    engine->gpu_memory_manager.vb_memory.offset = ALIGN_UP(engine->gpu_memory_manager.vb_memory.offset + buffer->count * buffer->stride, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    if (!engine->gpu_memory_manager.vb_memory.first)
    {
        engine->gpu_memory_manager.vb_memory.first = buffer;
    }
    else
    {
        GPUMemoryBlock *it = engine->gpu_memory_manager.vb_memory.first;
        while (it->next) it = it->next;
        it->next = buffer;
    }
}

void PushIB(
    IN  Engine         *engine,
    IN  u32             count,
    OUT GPUMemoryBlock *buffer)
{
    Check(engine);
    Check(count);
    Check(buffer);

    buffer->next     = 0;
    buffer->resource = 0;
    buffer->desc     = 0;
    buffer->kind     = GPU_MEMORY_BLOCK_KIND_IB;
    buffer->count    = count;
    buffer->stride   = sizeof(u32);
    buffer->barrier  = (D3D12_RESOURCE_BARRIER){0};
    buffer->state    = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = buffer->count * buffer->stride;
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                engine->gpu_memory_manager.ib_memory.heap,
                                                                                                engine->gpu_memory_manager.ib_memory.offset,
                                                                                                &resource_desc,
                                                                                                buffer->state,
                                                                                                0,
                                                                                                &IID_ID3D12Resource,
                                                                                                &buffer->resource);
    CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Index Buffer Memory Overflow");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    engine->gpu_memory_manager.ib_memory.offset = ALIGN_UP(engine->gpu_memory_manager.ib_memory.offset + buffer->count * buffer->stride, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    if (!engine->gpu_memory_manager.ib_memory.first)
    {
        engine->gpu_memory_manager.ib_memory.first = buffer;
    }
    else
    {
        GPUMemoryBlock *it = engine->gpu_memory_manager.ib_memory.first;
        while (it->next) it = it->next;
        it->next = buffer;
    }
}

void PushCBV(
    IN  Engine         *engine,
    IN  u32             size_in_bytes,
    OUT GPUMemoryBlock *buffer)
{
    Check(engine);
    Check(size_in_bytes);
    Check(buffer);

    buffer->next     = 0;
    buffer->resource = 0;
    buffer->desc     = 0;
    buffer->kind     = GPU_MEMORY_CBV;
    buffer->width    = size_in_bytes;
    buffer->height   = 1;
    buffer->depth    = 1;
    buffer->barrier  = (D3D12_RESOURCE_BARRIER){0};
    buffer->state    = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = buffer->width;
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                engine->gpu_memory_manager.cbv_memory.heap,
                                                                                                engine->gpu_memory_manager.cbv_memory.offset,
                                                                                                &resource_desc,
                                                                                                buffer->state,
                                                                                                0,
                                                                                                &IID_ID3D12Resource,
                                                                                                &buffer->resource);
    CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Constant Buffer Memory Overflow");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    buffer->barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    buffer->barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    buffer->barrier.Transition.pResource   = buffer->resource;
    buffer->barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    buffer->barrier.Transition.StateBefore = buffer->state;
    buffer->barrier.Transition.StateAfter  = buffer->state;

    D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc;
    desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc_heap_desc.NumDescriptors = 1;
    desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc_heap_desc.NodeMask       = 1;

    // @Optimize(Roman): free list for descriptors
    engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateDescriptorHeap(engine->gpu_manager.device,
                                                                                                &desc_heap_desc,
                                                                                                &IID_ID3D12DescriptorHeap,
                                                                                                &buffer->desc);
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    cbv_desc.BufferLocation = buffer->resource->lpVtbl->GetGPUVirtualAddress(buffer->resource);
    cbv_desc.SizeInBytes    = buffer->width;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = {0};

    #pragma warning(suppress: 4020) // I know what I'm doing.
    buffer->desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(buffer->desc, &cpu_handle);

    engine->gpu_manager.device->lpVtbl->CreateConstantBufferView(engine->gpu_manager.device,
                                                                 &cbv_desc,
                                                                 cpu_handle);

    engine->gpu_memory_manager.cbv_memory.offset = ALIGN_UP(engine->gpu_memory_manager.cbv_memory.offset + buffer->width, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    if (!engine->gpu_memory_manager.cbv_memory.first)
    {
        engine->gpu_memory_manager.cbv_memory.first = buffer;
    }
    else
    {
        GPUMemoryBlock *it = engine->gpu_memory_manager.cbv_memory.first;
        while (it->next) it = it->next;
        it->next = buffer;
    }
}

// @TODO(Roman): PushSRV, PushUAV, PushSampler

internal void PushCopy(
    IN  Engine         *engine,
    IN  GPUMemoryBlock *source,
    OUT GPUMemoryBlock *copy)
{
    Check(engine);
    Check(source);
    Check(copy);

    GPUMemory *current_copy_memory = engine->gpu_memory_manager.copy_memory + engine->gpu_manager.current_buffer;

    copy->next     = 0;
    copy->resource = 0;
    copy->desc     = 0;
    copy->kind     = source->kind;
    copy->state    = D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_DESC resource_desc   = {0};
    u64                 new_heap_offset = current_copy_memory->offset;

    switch (copy->kind)
    {
        case GPU_MEMORY_BLOCK_KIND_VB:
        case GPU_MEMORY_BLOCK_KIND_IB:
        {
            copy->count  = source->count;
            copy->stride = source->stride;

            resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            resource_desc.Width              = copy->count * copy->stride;
            resource_desc.Height             = 1;
            resource_desc.DepthOrArraySize   = 1;
            resource_desc.MipLevels          = 1;
            resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc.Count   = 1;
            resource_desc.SampleDesc.Quality = 0;
            resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            new_heap_offset += copy->count * copy->stride;
            new_heap_offset  = ALIGN_UP(new_heap_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        } break;

        case GPU_MEMORY_BLOCK_KIND_CBV:
        case GPU_MEMORY_BLOCK_KIND_SRV:
        case GPU_MEMORY_BLOCK_KIND_UAV:
        {
            copy->width  = source->width;
            copy->height = source->height;
            copy->depth  = source->depth;

            resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            resource_desc.Width              = copy->width;
            resource_desc.Height             = copy->height;
            resource_desc.DepthOrArraySize   = copy->depth;
            resource_desc.MipLevels          = 1;
            resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc.Count   = 1;
            resource_desc.SampleDesc.Quality = 0;
            resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            new_heap_offset += copy->width * copy->height * copy->depth;
            new_heap_offset  = ALIGN_UP(new_heap_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
        } break;

        case GPU_MEMORY_BLOCK_KIND_SAMPLER:
        {
            // @TODO(Roman): Copy sampler
            CheckM(false, "Sampler memory management will be soon");
        } break;

        default:
        {
            Check(false);
        } break;
    }

    engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                current_copy_memory->heap,
                                                                                                current_copy_memory->offset,
                                                                                                &resource_desc,
                                                                                                copy->state,
                                                                                                0,
                                                                                                &IID_ID3D12Resource,
                                                                                                &copy->resource);
    CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Copy Memory Overflow");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    copy->barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    copy->barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    copy->barrier.Transition.pResource   = copy->resource;
    copy->barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    copy->barrier.Transition.StateBefore = copy->state;
    copy->barrier.Transition.StateAfter  = copy->state;

    current_copy_memory->offset = new_heap_offset;

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    D3D12_RESOURCE_BARRIER activation_barrier;
    activation_barrier.Type                     = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    activation_barrier.Flags                    = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    activation_barrier.Aliasing.pResourceBefore = 0;
    activation_barrier.Aliasing.pResourceAfter  = copy->resource;
    graphics_list->lpVtbl->ResourceBarrier(graphics_list, 1, &activation_barrier);

    if (!current_copy_memory->first)
    {
        current_copy_memory->first = copy;
    }
    else
    {
        GPUMemoryBlock *it = current_copy_memory->first;
        while (it->next) it = it->next;
        it->next = copy;
    }
}

void SetGPUMemoryBlockData(
    IN Engine           *engine,
    IN GPUMemoryBlock   *buffer,
    IN void             *data)
{
    Check(engine);
    Check(buffer);
    Check(data);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    GPUMemoryBlock *copy_buffer = Alloc(GPUMemoryBlock, &engine->allocator, 1);
    PushCopy(engine, buffer, copy_buffer);

    switch (buffer->kind)
    {
        case GPU_MEMORY_BLOCK_KIND_VB:
        case GPU_MEMORY_BLOCK_KIND_IB:
        {
            void *copy_res_data = 0;
            D3D12_RANGE read_range = {0, 0};
            engine->gpu_memory_manager.error = copy_buffer->resource->lpVtbl->Map(copy_buffer->resource, 0, &read_range, &copy_res_data);
            Check(SUCCEEDED(engine->gpu_memory_manager.error));

            CopyMemory(copy_res_data, data, copy_buffer->count * copy_buffer->stride);

            copy_buffer->resource->lpVtbl->Unmap(copy_buffer->resource, 0, 0);
        } break;

        case GPU_MEMORY_BLOCK_KIND_CBV:
        case GPU_MEMORY_BLOCK_KIND_SRV:
        case GPU_MEMORY_BLOCK_KIND_UAV:
        {
            void *copy_res_data = 0;
            D3D12_RANGE read_range = {0, 0};
            engine->gpu_memory_manager.error = copy_buffer->resource->lpVtbl->Map(copy_buffer->resource, 0, &read_range, &copy_res_data);
            Check(SUCCEEDED(engine->gpu_memory_manager.error));

            CopyMemory(copy_res_data, data, copy_buffer->width * copy_buffer->height * copy_buffer->depth);

            copy_buffer->resource->lpVtbl->Unmap(copy_buffer->resource, 0, 0);
        } break;

        case GPU_MEMORY_BLOCK_KIND_SAMPLER:
        {
            // @TODO(Roman): Copy sampler
            CheckM(false, "Sampler memory management will be soon");
        } break;
        
        default:
        {
            Check(false);
        } break;
    }

    copy_buffer->barrier.Transition.StateBefore = copy_buffer->state;
    copy_buffer->barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
    buffer->barrier.Transition.StateBefore      = buffer->state;
    buffer->barrier.Transition.StateAfter       = D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_RESOURCE_BARRIER begin_barriers[] =
    {
        copy_buffer->barrier,
        buffer->barrier
    };
    graphics_list->lpVtbl->ResourceBarrier(graphics_list, ArrayCount(begin_barriers), begin_barriers);

    graphics_list->lpVtbl->CopyResource(graphics_list, buffer->resource, copy_buffer->resource);

    copy_buffer->barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    copy_buffer->barrier.Transition.StateAfter  = copy_buffer->state;
    buffer->barrier.Transition.StateBefore      = D3D12_RESOURCE_STATE_COPY_DEST;
    buffer->barrier.Transition.StateAfter       = buffer->state;
    D3D12_RESOURCE_BARRIER end_barriers[] =
    {
        copy_buffer->barrier,
        buffer->barrier
    };
    graphics_list->lpVtbl->ResourceBarrier(graphics_list, ArrayCount(end_barriers), end_barriers);
}

void BindVB(
    IN Engine         *engine,
    IN GPUMemoryBlock *buffer)
{
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = buffer->resource->lpVtbl->GetGPUVirtualAddress(buffer->resource);
    vbv.SizeInBytes    = buffer->count * buffer->stride;
    vbv.StrideInBytes  = buffer->stride;

    graphics_list->lpVtbl->IASetVertexBuffers(graphics_list, 0, 1, &vbv);
}

void DrawVertices(
    IN Engine         *engine,
    IN GPUMemoryBlock *buffer)
{
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
    graphics_list->lpVtbl->DrawInstanced(graphics_list, buffer->count, 1, 0, 0);
}

void BindIB(
    IN Engine         *engine,
    IN GPUMemoryBlock *buffer)
{
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = buffer->resource->lpVtbl->GetGPUVirtualAddress(buffer->resource);
    ibv.SizeInBytes    = buffer->count * buffer->stride;
    ibv.Format         = DXGI_FORMAT_R32_UINT;

    graphics_list->lpVtbl->IASetIndexBuffer(graphics_list, &ibv);
}

void DrawIndices(
    IN Engine         *engine,
    IN GPUMemoryBlock *buffer)
{
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
    graphics_list->lpVtbl->DrawIndexedInstanced(graphics_list, buffer->count, 1, 0, 0, 0);
}
