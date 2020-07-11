//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "gpu/gpu_memory_manager.h"
#include "cengine.h"

#define SafeRelease(directx_interface)                           \
{                                                                \
    if (directx_interface)                                       \
    {                                                            \
        (directx_interface)->lpVtbl->Release(directx_interface); \
        (directx_interface) = null;                              \
    }                                                            \
}

void ResetGPUMemory(
    IN Engine          *engine,
    IN GPU_MEMORY_TYPE  type)
{
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    if (type & GPU_MEMORY_TYPE_PERMANENT)
    {
        for (GPUResource *it = engine->gpu_memory_manager.permanent_memory.resources;
             it;
             it = it->next)
        {
            if (it->allocation_state == GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED)
            {
                it->allocation_state = GPU_RESOURCE_ALLOCATION_STATE_IN_FREE_LIST;

                if (it->desc_heap)
                {
                    it->desc_heap->resource         = null;
                    it->desc_heap->allocation_state = GPU_DESC_HEAP_ALLOCATION_STATE_IN_FREE_LIST;
                    it->desc_heap                   = null;
                }
            }
        }
    }
    if (type & GPU_MEMORY_TYPE_TRANSIENT_CURRENT)
    {
        GPUResourceMemory *transient_memory = engine->gpu_memory_manager.transient_memory
                                            + engine->gpu_manager.current_buffer;

        for (GPUResource *it = transient_memory->resources;
             it;
             it = it->next)
        {
            if (it->allocation_state == GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED)
            {
                it->allocation_state = GPU_RESOURCE_ALLOCATION_STATE_IN_FREE_LIST;
            }
        }
    }
    else if (type & GPU_MEMORY_TYPE_TRANSIENT_ALL)
    {
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            GPUResourceMemory *transient_memory = engine->gpu_memory_manager.transient_memory
                                                + i;

            for (GPUResource *it = transient_memory->resources;
                it;
                it = it->next)
            {
                if (it->allocation_state == GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED)
                {
                    it->allocation_state = GPU_RESOURCE_ALLOCATION_STATE_IN_FREE_LIST;
                }
            }
        }
    }
}

void ReleaseGPUMemory(
    IN Engine          *engine,
    IN GPU_MEMORY_TYPE  type)
{
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    if (type & GPU_MEMORY_TYPE_PERMANENT)
    {
        GPUResource *it = engine->gpu_memory_manager.permanent_memory.resources;
        while (it)
        {
            SafeRelease(it->resource);

            if (it->desc_heap)
            {
                SafeRelease(it->desc_heap->desc_heap);
                DeAlloc(engine->gpu_memory_manager.allocator, it->desc_heap);
            }

            GPUResource *next = it->next;
            DeAlloc(engine->gpu_memory_manager.allocator, it);
            it = next;
        }
        engine->gpu_memory_manager.permanent_memory.resources       = null;
        engine->gpu_memory_manager.permanent_memory.gpu_heap_offset = 0;
    }
    if (type & GPU_MEMORY_TYPE_TRANSIENT_CURRENT)
    {
        GPUResourceMemory *transient_memory = engine->gpu_memory_manager.transient_memory
                                            + engine->gpu_manager.current_buffer;

        GPUResource *it = transient_memory->resources;
        while (it)
        {
            SafeRelease(it->resource);

            GPUResource *next = it->next;
            DeAlloc(engine->gpu_memory_manager.allocator, it);
            it = next;
        }
        transient_memory->resources       = null;
        transient_memory->gpu_heap_offset = 0;
    }
    else if (type & GPU_MEMORY_TYPE_TRANSIENT_ALL)
    {
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            GPUResourceMemory *transient_memory = engine->gpu_memory_manager.transient_memory
                                                + i;

            GPUResource *it = transient_memory->resources;
            while (it)
            {
                SafeRelease(it->resource);

                GPUResource *next = it->next;
                DeAlloc(engine->gpu_memory_manager.allocator, it);
                it = next;
            }
            transient_memory->resources       = null;
            transient_memory->gpu_heap_offset = 0;
        }
    }
}

internal GPUResource *FindOrAllocateResource(
    IN  Engine              *engine,
    IN  D3D12_RESOURCE_DESC *resource_desc,
    OUT b32                 *allocated)
{
    Check(engine);
    Check(resource_desc);
    Check(allocated);

    GPUResource *prev = 0;

    for (GPUResource *it = engine->gpu_memory_manager.permanent_memory.resources;
         it;
         prev = it, it = it->next)
    {
        if (it->allocation_state == GPU_RESOURCE_ALLOCATION_STATE_IN_FREE_LIST
        &&  RtlEqualMemory(&it->resource_desc, &resource_desc, sizeof(D3D12_RESOURCE_DESC)))
        {
            it->allocation_state = GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED;
            *allocated = false;
            return it;
        }
    }

    GPUResource *resource      = Alloc(GPUResource, engine->gpu_memory_manager.allocator, 1);
    resource->allocation_state = GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED;
    resource->resource_desc    = *resource_desc;

    if (prev)
    {
        prev->next = resource;
    }
    else
    {
        engine->gpu_memory_manager.permanent_memory.resources = resource;
    }

    *allocated = true;
    return resource;
}

internal GPUResource *FindOrAllocateCopyResource(
    IN  Engine              *engine,
    IN  D3D12_RESOURCE_DESC *resource_desc,
    OUT b32                 *allocated)
{
    Check(engine);
    Check(resource_desc);
    Check(allocated);

    GPUResourceMemory *transient_memory = engine->gpu_memory_manager.transient_memory
                                        + engine->gpu_manager.current_buffer;

    GPUResource *prev = 0;

    for (GPUResource *it = transient_memory->resources;
         it;
         prev = it, it = it->next)
    {
        if (it->allocation_state == GPU_RESOURCE_ALLOCATION_STATE_IN_FREE_LIST
        &&  RtlEqualMemory(&it->resource_desc, &resource_desc, sizeof(D3D12_RESOURCE_DESC)))
        {
            it->allocation_state = GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED;
            *allocated = false;
            return it;
        }
    }

    GPUResource *resource      = Alloc(GPUResource, engine->gpu_memory_manager.allocator, 1);
    resource->allocation_state = GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED;
    resource->resource_desc    = *resource_desc;

    if (prev)
    {
        prev->next = resource;
    }
    else
    {
        transient_memory->resources = resource;
    }

    *allocated = true;
    return resource;
}

internal GPUDescHeap *FindOrAllocateDescHeap(
    IN  Engine                     *engine,
    IN  D3D12_DESCRIPTOR_HEAP_DESC *desc_heap_desc,
    OUT b32                        *allocated)
{
    Check(engine);
    Check(desc_heap_desc);
    Check(allocated);
#if CENGINE_ISA >= CENGINE_ISA_SSE
    Check(sizeof(D3D12_DESCRIPTOR_HEAP_DESC) == sizeof(__m128i));
#endif

    GPUDescHeap *prev = 0;

    for (GPUDescHeap *it = engine->gpu_memory_manager.desc_heap_memory.desc_heaps;
         it;
         prev = it, it = it->next)
    {
        if (it->allocation_state == GPU_DESC_HEAP_ALLOCATION_STATE_IN_FREE_LIST
        &&  mm_equals(&it->desc_heap_desc, desc_heap_desc))
        {
            it->allocation_state = GPU_DESC_HEAP_ALLOCATION_STATE_ALLOCATED;
            *allocated = false;
            return it;
        }
    }

    GPUDescHeap *desc_heap      = Alloc(GPUDescHeap, engine->gpu_memory_manager.allocator, 1);
    desc_heap->allocation_state = GPU_DESC_HEAP_ALLOCATION_STATE_ALLOCATED;
    desc_heap->desc_heap_desc   = *desc_heap_desc;

    if (prev)
    {
        prev->next = desc_heap;
    }
    else
    {
        engine->gpu_memory_manager.desc_heap_memory.desc_heaps = desc_heap;
    }

    *allocated = true;
    return desc_heap;
}

GPUResource *PushVertexBuffer(
    IN Engine *engine,
    IN u32     count,
    IN u32     stride)
{
    Check(engine);
    Check(count);
    Check(stride);
    Check(engine);

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = count * stride;
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    b32          allocated = false;
    GPUResource *resource  = FindOrAllocateResource(engine,
                                                    &resource_desc,
                                                    &allocated);
    
    ZeroMemory(resource->name, ArrayCount(resource->name));
    resource->name_len      = 0;
    resource->desc_heap     = null;
    resource->kind          = GPU_RESOURCE_KIND_VB;
    resource->vi_count      = count;
    resource->vi_stride     = stride;
    resource->initial_state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    if (allocated)
    {
        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                    engine->gpu_memory_manager.permanent_memory.gpu_heap,
                                                                                                    engine->gpu_memory_manager.permanent_memory.gpu_heap_offset,
                                                                                                    &resource->resource_desc,
                                                                                                    resource->initial_state,
                                                                                                    null,
                                                                                                    &IID_ID3D12Resource,
                                                                                                    &resource->resource);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Permanent Memory Overflow");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.permanent_memory.gpu_heap_offset += resource->resource_desc.Width;
        engine->gpu_memory_manager.permanent_memory.gpu_heap_offset  = ALIGN_UP(engine->gpu_memory_manager.permanent_memory.gpu_heap_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

        D3D12_DISCARD_REGION region;
        region.NumRects         = 0;
        region.pRects           = null;
        region.FirstSubresource = 0;
        region.NumSubresources  = 1;

        graphics_list->lpVtbl->DiscardResource(graphics_list,
                                               resource->resource,
                                               &region);
    }

    return resource;
}

GPUResource *PushIndexBuffer(
    IN Engine *engine,
    IN u32     count)
{
    Check(engine);
    Check(count);
    Check(engine);

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = count * sizeof(u32);
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_R32_UINT;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    b32          allocated = false;
    GPUResource *resource  = FindOrAllocateResource(engine,
                                                    &resource_desc,
                                                    &allocated);

    ZeroMemory(resource->name, ArrayCount(resource->name));
    resource->name_len      = 0;
    resource->desc_heap     = null;
    resource->kind          = GPU_RESOURCE_KIND_IB;
    resource->vi_count      = count;
    resource->vi_stride     = sizeof(u32);
    resource->initial_state = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    if (allocated)
    {
        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                    engine->gpu_memory_manager.permanent_memory.gpu_heap,
                                                                                                    engine->gpu_memory_manager.permanent_memory.gpu_heap_offset,
                                                                                                    &resource->resource_desc,
                                                                                                    resource->initial_state,
                                                                                                    null,
                                                                                                    &IID_ID3D12Resource,
                                                                                                    &resource->resource);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Permanent Memory Overflow");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.permanent_memory.gpu_heap_offset += resource->resource_desc.Width;
        engine->gpu_memory_manager.permanent_memory.gpu_heap_offset  = ALIGN_UP(engine->gpu_memory_manager.permanent_memory.gpu_heap_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

        D3D12_DISCARD_REGION region;
        region.NumRects         = 0;
        region.pRects           = null;
        region.FirstSubresource = 0;
        region.NumSubresources  = 1;

        graphics_list->lpVtbl->DiscardResource(graphics_list,
                                               resource->resource,
                                               &region);
    }

    return resource;
}

GPUResource *PushConstantBuffer(
    IN Engine     *engine,
    IN u32         size_in_bytes,
    IN const char *name,
    IN u64         name_len)
{
    Check(engine);
    Check(size_in_bytes && size_in_bytes <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    Check(name);
    Check(name_len);

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = size_in_bytes;
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    b32          allocated = false;
    GPUResource *resource  = FindOrAllocateResource(engine,
                                                    &resource_desc,
                                                    &allocated);

    CopyMemory(resource->name, name, name_len);
    resource->name_len      = name_len;
    resource->desc_heap     = null;
    resource->kind          = GPU_RESOURCE_KIND_CB;
    resource->vi_count      = 0;
    resource->vi_stride     = 0;
    resource->initial_state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    if (allocated)
    {
        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                    engine->gpu_memory_manager.permanent_memory.gpu_heap,
                                                                                                    engine->gpu_memory_manager.permanent_memory.gpu_heap_offset,
                                                                                                    &resource->resource_desc,
                                                                                                    resource->initial_state,
                                                                                                    null,
                                                                                                    &IID_ID3D12Resource,
                                                                                                    &resource->resource);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Permanent Memory Overflow");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.permanent_memory.gpu_heap_offset += resource->resource_desc.Width;
        engine->gpu_memory_manager.permanent_memory.gpu_heap_offset  = ALIGN_UP(engine->gpu_memory_manager.permanent_memory.gpu_heap_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

        D3D12_DISCARD_REGION region;
        region.NumRects         = 0;
        region.pRects           = null;
        region.FirstSubresource = 0;
        region.NumSubresources  = 1;

        graphics_list->lpVtbl->DiscardResource(graphics_list,
                                               resource->resource,
                                               &region);
    }

    engine->gpu_memory_manager.error = resource->resource->lpVtbl->SetPrivateData(resource->resource,
                                                                                  &WKPDID_D3DDebugObjectName,
                                                                                  cast(u32, resource->name_len),
                                                                                  resource->name);
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc;
    desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc_heap_desc.NumDescriptors = 1;
    desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc_heap_desc.NodeMask       = 1;

    GPUDescHeap *desc_heap = FindOrAllocateDescHeap(engine,
                                                    &desc_heap_desc,
                                                    &allocated);

    if (allocated)
    {
        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateDescriptorHeap(engine->gpu_manager.device,
                                                                                                    &desc_heap->desc_heap_desc,
                                                                                                    &IID_ID3D12DescriptorHeap,
                                                                                                    &desc_heap->desc_heap);
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
        cbv_desc.BufferLocation = resource->resource->lpVtbl->GetGPUVirtualAddress(resource->resource);
        cbv_desc.SizeInBytes    = size_in_bytes;

        D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle = {0};
        #pragma warning(suppress: 4020) // I know what I'm doing.
        desc_heap->desc_heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(desc_heap->desc_heap, &cpu_desc_handle);

        engine->gpu_manager.device->lpVtbl->CreateConstantBufferView(engine->gpu_manager.device,
                                                                     &cbv_desc,
                                                                     cpu_desc_handle);
    }

    resource->desc_heap = desc_heap;
    desc_heap->resource = resource;

    return resource;
}

internal GPUResource *PushCopy(
    IN Engine      *engine,
    IN GPUResource *source)
{
    Check(engine);
    Check(source);

    GPUResourceMemory *transient_memory = engine->gpu_memory_manager.transient_memory + engine->gpu_manager.current_buffer;

    b32          allocated = false;
    GPUResource *resource  = FindOrAllocateCopyResource(engine,
                                                        &source->resource_desc,
                                                        &allocated);

    resource->kind                = source->kind;
    resource->vi_count            = source->vi_count;
    resource->vi_stride           = source->vi_stride;
    resource->initial_state       = D3D12_RESOURCE_STATE_COMMON;
//  resource->resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE; ?

    if (allocated)
    {
        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                    transient_memory->gpu_heap,
                                                                                                    transient_memory->gpu_heap_offset,
                                                                                                    &resource->resource_desc,
                                                                                                    resource->initial_state,
                                                                                                    null,
                                                                                                    &IID_ID3D12Resource,
                                                                                                    &resource->resource);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Transient Memory Overflow");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        transient_memory->gpu_heap_offset += resource->resource_desc.Width
                                          *  resource->resource_desc.Height
                                          *  resource->resource_desc.DepthOrArraySize;
        transient_memory->gpu_heap_offset  = ALIGN_UP(transient_memory->gpu_heap_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

        D3D12_DISCARD_REGION region;
        region.NumRects         = 0;
        region.pRects           = null;
        region.FirstSubresource = 0;
        region.NumSubresources  = 1;

        graphics_list->lpVtbl->DiscardResource(graphics_list,
                                               resource->resource,
                                               &region);
    }

    return resource;
}

void SetGPUResourceData(
    IN Engine      *engine,
    IN GPUResource *resource,
    IN void        *data)
{
    Check(engine);
    Check(resource);
    Check(data);

    // @TODO(Roman): Discard resource if data == null.
    
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    GPUResource *copy_resource = PushCopy(engine, resource);

    u64 copy_resource_size = copy_resource->resource_desc.Width
                           * copy_resource->resource_desc.Height
                           * copy_resource->resource_desc.DepthOrArraySize;

    void        *copy_resource_data  = 0;
    D3D12_RANGE  read_range          = { 0, copy_resource_size };
    engine->gpu_memory_manager.error = copy_resource->resource->lpVtbl->Map(copy_resource->resource,
                                                                            0,
                                                                            &read_range,
                                                                            &copy_resource_data);
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    CopyMemory(copy_resource_data, data, copy_resource_size);

    copy_resource->resource->lpVtbl->Unmap(copy_resource->resource, 0, &read_range);

    D3D12_RESOURCE_BARRIER copy_resource_barrier;
    copy_resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    copy_resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    copy_resource_barrier.Transition.pResource   = copy_resource->resource;
    copy_resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    copy_resource_barrier.Transition.StateBefore = copy_resource->initial_state;
    copy_resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

    D3D12_RESOURCE_BARRIER resource_barrier;
    resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barrier.Transition.pResource   = resource->resource;
    resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    resource_barrier.Transition.StateBefore = resource->initial_state;
    resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_RESOURCE_BARRIER begin_barriers[] =
    {
        copy_resource_barrier,
        resource_barrier
    };
    graphics_list->lpVtbl->ResourceBarrier(graphics_list, ArrayCount(begin_barriers), begin_barriers);

    graphics_list->lpVtbl->CopyResource(graphics_list, resource->resource, copy_resource->resource);

    copy_resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    copy_resource_barrier.Transition.StateAfter  = copy_resource->initial_state;
    resource_barrier.Transition.StateBefore      = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barrier.Transition.StateAfter       = resource->initial_state;

    D3D12_RESOURCE_BARRIER end_barriers[] =
    {
        resource_barrier,
        copy_resource_barrier
    };
    graphics_list->lpVtbl->ResourceBarrier(graphics_list, ArrayCount(end_barriers), end_barriers);
}

void SetGPUResourceDataByName(
    IN Engine      *engine,
    IN const char  *name,
    IN u64          name_len,
    IN void        *data)
{
    Check(engine);
    Check(name);
    Check(name_len);

    GPUResource *resource = null;

    for (GPUResource *it = engine->gpu_memory_manager.permanent_memory.resources;
         it;
         it = it->next)
    {
        if (it->allocation_state == GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED
        &&  it->name_len == name_len
        &&  RtlEqualMemory(it->name, name, it->name_len))
        {
            resource = it;
            break;
        }
    }

    // @TODO(Roman): Better message?
    CheckM(resource, "Resource not found. Expected name: %s", name);

    SetGPUResourceData(engine, resource, data);
}

void BindVertexBuffer(
    IN Engine      *engine,
    IN GPUResource *resource)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = resource->resource->lpVtbl->GetGPUVirtualAddress(resource->resource);
    vbv.SizeInBytes    = resource->vi_count * resource->vi_stride;
    vbv.StrideInBytes  = resource->vi_stride;

    graphics_list->lpVtbl->IASetVertexBuffers(graphics_list, 0, 1, &vbv);
}

void BindIndxeBuffer(
    IN Engine      *engine,
    IN GPUResource *resource)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = resource->resource->lpVtbl->GetGPUVirtualAddress(resource->resource);
    ibv.SizeInBytes    = resource->vi_count * resource->vi_stride;
    ibv.Format         = resource->resource_desc.Format;

    graphics_list->lpVtbl->IASetIndexBuffer(graphics_list, &ibv);
}

void DrawVertices(
    IN Engine      *engine,
    IN GPUResource *resource)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
    graphics_list->lpVtbl->DrawInstanced(graphics_list, resource->vi_count, 1, 0, 0);
}

void DrawIndices(
    IN Engine      *engine,
    IN GPUResource *resource)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
    graphics_list->lpVtbl->DrawIndexedInstanced(graphics_list, resource->vi_count, 1, 0, 0, 0);
}
