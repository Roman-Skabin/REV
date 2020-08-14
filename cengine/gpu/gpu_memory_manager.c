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
    in Engine *engine)
{
    Check(engine);

    // buffers
    for (GPUResource *it = engine->gpu_memory_manager.buffer_memory.resources;
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

    // textures
    for (GPUResource *it = engine->gpu_memory_manager.texture_memory.resources;
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

void ReleaseGPUMemory(
    in Engine *engine)
{
    Check(engine);

    // buffers
    for (GPUResource *it = engine->gpu_memory_manager.buffer_memory.resources;
         it;
         )
    {
        SafeRelease(it->default_resource);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(it->upload_resources[i]);
        }

        GPUResource *next = it->next;
        DeAlloc(engine->gpu_memory_manager.allocator, it);
        it = next;
    }

    // textures
    for (GPUResource *it = engine->gpu_memory_manager.texture_memory.resources;
         it;
         )
    {
        SafeRelease(it->default_resource);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(it->upload_resources[i]);
        }

        GPUResource *next = it->next;
        DeAlloc(engine->gpu_memory_manager.allocator, it);
        it = next;
    }

    // desc heaps
    for (GPUDescHeap *it = engine->gpu_memory_manager.desc_heap_memory.desc_heaps;
         it;
         )
    {
        SafeRelease(it->desc_heap);

        GPUDescHeap *next = it->next;
        DeAlloc(engine->gpu_memory_manager.allocator, it);
        it = next;
    }

    engine->gpu_memory_manager.buffer_memory.resources     = null;
    engine->gpu_memory_manager.texture_memory.resources    = null;
    engine->gpu_memory_manager.desc_heap_memory.desc_heaps = null;

    engine->gpu_memory_manager.buffer_memory.default_offset  = 0;
    engine->gpu_memory_manager.texture_memory.default_offset = 0;

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->gpu_memory_manager.buffer_memory.upload_offset[i] = 0;
        engine->gpu_memory_manager.texture_memory.upload_offset[i] = 0;
    }
}

internal GPUResource *FindOrAllocateResource(
    in  Engine              *engine,
    in  D3D12_RESOURCE_DESC *resource_desc,
    out b32                 *allocated)
{
    Check(engine);
    Check(resource_desc);
    Check(allocated);

    switch (resource_desc->Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
        {
            GPUResource *prev = 0;

            for (GPUResource *it = engine->gpu_memory_manager.buffer_memory.resources;
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
                engine->gpu_memory_manager.buffer_memory.resources = resource;
            }

            *allocated = true;
            return resource;
        } break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        {
            GPUResource *prev = 0;

            for (GPUResource *it = engine->gpu_memory_manager.texture_memory.resources;
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
                engine->gpu_memory_manager.texture_memory.resources = resource;
            }

            *allocated = true;
            return resource;
        } break;

        default:
        {
            FailedM("Unknown GPU resource dimension");
        } break;
    }

    return null;
}

internal GPUDescHeap *FindOrAllocateDescHeap(
    in  Engine                     *engine,
    in  D3D12_DESCRIPTOR_HEAP_DESC *desc_heap_desc,
    out b32                        *allocated)
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

internal void CreateGPUResource(
    in     Engine            *engine,
    in     GPUResourceMemory *resource_memory,
    in out GPUResource       *resource)
{
    Check(engine);
    Check(resource_memory);
    Check(resource);
    Check(resource->allocation_state == GPU_RESOURCE_ALLOCATION_STATE_ALLOCATED);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    D3D12_DISCARD_REGION discard_region;
    discard_region.NumRects         = 0;
    discard_region.pRects           = null;
    discard_region.FirstSubresource = 0;
    discard_region.NumSubresources  = 1;

    engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                resource_memory->default_heap,
                                                                                                resource_memory->default_offset,
                                                                                                &resource->resource_desc,
                                                                                                resource->initial_state,
                                                                                                null,
                                                                                                &IID_ID3D12Resource,
                                                                                                &resource->default_resource);
    CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Default Buffer Memory Overflow");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    resource_memory->default_offset += resource->resource_desc.Width
                                    *  resource->resource_desc.Height
                                    *  resource->resource_desc.DepthOrArraySize;
    resource_memory->default_offset  = ALIGN_UP(resource_memory->default_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    graphics_list->lpVtbl->DiscardResource(graphics_list,
                                           resource->default_resource,
                                           &discard_region);

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreatePlacedResource(engine->gpu_manager.device,
                                                                                                    resource_memory->upload_heap[i],
                                                                                                    resource_memory->upload_offset[i],
                                                                                                    &resource->resource_desc,
                                                                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                                                    null,
                                                                                                    &IID_ID3D12Resource,
                                                                                                    resource->upload_resources + i);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "Upload Buffer Memory Overflow");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        resource_memory->upload_offset[i] += resource->resource_desc.Width
                                          *  resource->resource_desc.Height
                                          *  resource->resource_desc.DepthOrArraySize;
        resource_memory->upload_offset[i]  = ALIGN_UP(resource_memory->upload_offset[i], D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        graphics_list->lpVtbl->DiscardResource(graphics_list,
                                               resource->upload_resources[i],
                                               &discard_region);

        D3D12_RANGE read_range = { 0, 0 };
        engine->gpu_memory_manager.error = resource->upload_resources[i]->lpVtbl->Map(resource->upload_resources[i],
                                                                                      0,
                                                                                      &read_range,
                                                                                      resource->upload_pointers + i);
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }
}

GPUResource *PushVertexBuffer(
    in Engine *engine,
    in u32     count,
    in u32     stride)
{
    Check(engine);
    Check(count);
    Check(stride);

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

#if 0
    CheckM(1 <= resource_desc.Width && resource_desc.Width <= engine->gpu_manager.features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           "Too big vertex buffer. Required vertex buffer size in bytes = [1, %I32u], got: %I32u",
           engine->gpu_manager.features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           resource_desc.Width);
#endif

    b32          allocated = false;
    GPUResource *resource  = FindOrAllocateResource(engine,
                                                    &resource_desc,
                                                    &allocated);

    ZeroMemory(resource->name, sizeof(resource->name));
    resource->name_len            = 0;
    resource->desc_heap           = null;
    resource->kind                = GPU_RESOURCE_KIND_VB;
    resource->vertex_index_count  = count;
    resource->vertex_index_stride = stride;
    resource->initial_state       = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    if (allocated)
    {
        CreateGPUResource(engine,
                          &engine->gpu_memory_manager.buffer_memory,
                          resource);
    }

    return resource;
}

GPUResource *PushIndexBuffer(
    in Engine *engine,
    in u32     count)
{
    Check(engine);
    Check(count);

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

#if 0
    CheckM(1 <= resource_desc.Width && resource_desc.Width <= engine->gpu_manager.features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           "Too big index buffer. Required indices buffer size in bytes = [1, %I32u], got: %I32u",
           engine->gpu_manager.features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           resource_desc.Width);
#endif

    b32          allocated = false;
    GPUResource *resource  = FindOrAllocateResource(engine,
                                                    &resource_desc,
                                                    &allocated);

    ZeroMemory(resource->name, sizeof(resource->name));
    resource->name_len            = 0;
    resource->desc_heap           = null;
    resource->kind                = GPU_RESOURCE_KIND_IB;
    resource->vertex_index_count  = count;
    resource->vertex_index_stride = sizeof(u32);
    resource->initial_state       = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    if (allocated)
    {
        CreateGPUResource(engine,
                          &engine->gpu_memory_manager.buffer_memory,
                          resource);
    }

    return resource;
}

GPUResource *PushConstantBuffer(
    in Engine     *engine,
    in u32         size_in_bytes,
    in const char *name,
    in u32         name_len)
{
    Check(engine);
    Check(size_in_bytes && size_in_bytes <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

#if 0
    CheckM(1 <= size_in_bytes && size_in_bytes <= engine->gpu_manager.features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           "Too big constant buffer. Required constant buffer size in bytes = [1, %I32u], got: %I32u",
           engine->gpu_manager.features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           size_in_bytes);
#endif

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

    ZeroMemory(resource->name, sizeof(resource->name));
    resource->name_len            = 0;
    resource->desc_heap           = null;
    resource->kind                = GPU_RESOURCE_KIND_CB;
    resource->vertex_index_count  = 0;
    resource->vertex_index_stride = 0;
    resource->initial_state       = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    if (allocated)
    {
        CreateGPUResource(engine,
                          &engine->gpu_memory_manager.buffer_memory,
                          resource);
    }

    SetGPUResourceName(engine, resource, name, name_len);

    D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc;
    desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc_heap_desc.NumDescriptors = 1;
    desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc_heap_desc.NodeMask       = 0;

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
        cbv_desc.BufferLocation = resource->default_resource->lpVtbl->GetGPUVirtualAddress(resource->default_resource);
        cbv_desc.SizeInBytes    = size_in_bytes;

        D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle = {0};
        desc_heap->desc_heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(desc_heap->desc_heap, &cpu_desc_handle);

        engine->gpu_manager.device->lpVtbl->CreateConstantBufferView(engine->gpu_manager.device,
                                                                     &cbv_desc,
                                                                     cpu_desc_handle);
    }

    resource->desc_heap = desc_heap;
    desc_heap->resource = resource;

    return resource;
}

void SetGPUResourceName(
    in  Engine      *engine,
    in  GPUResource *resource,
    in  const char  *name,
    opt u32          name_len)
{
    Check(engine);
    Check(resource);
    Check(name);

    if (!name_len) name_len = cast(u32, strlen(name));
    CheckM(name_len < ArrayCountInStruct(GPUResource, name),
           "Too long name, max name length is %I32u.",
           ArrayCountInStruct(GPUResource, name));

    if (resource->name_len > name_len)
    {
        ZeroMemory(resource->name + name_len, resource->name_len - name_len);
    }

    resource->name_len = name_len;
    CopyMemory(resource->name, name, resource->name_len);

    char ext_name[16 + ArrayCountInStruct(GPUResource, name)] = "__default__";
    CopyMemory(ext_name + CSTRLEN("__default__"), resource->name, resource->name_len);

    engine->gpu_memory_manager.error = resource->default_resource->lpVtbl->SetPrivateData(resource->default_resource,
                                                                                          &WKPDID_D3DDebugObjectName,
                                                                                          cast(u32, CSTRLEN("__default__") + resource->name_len),
                                                                                          ext_name);
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        s32 len = sprintf(ext_name, "__upload_%I32u__", i);

        CopyMemory(ext_name + len, resource->name, resource->name_len);
        len += resource->name_len;

        engine->gpu_memory_manager.error = resource->upload_resources[i]->lpVtbl->SetPrivateData(resource->upload_resources[i],
                                                                                                 &WKPDID_D3DDebugObjectName,
                                                                                                 len,
                                                                                                 ext_name);
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }
}

void SetGPUResourceData(
    in Engine      *engine,
    in GPUResource *resource,
    in void        *data)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
    
    // @TODO(Roman): Use fences

    if (!data)
    {
        D3D12_DISCARD_REGION discard_region;
        discard_region.NumRects         = 0;
        discard_region.pRects           = null;
        discard_region.FirstSubresource = 0;
        discard_region.NumSubresources  = 1;

        graphics_list->lpVtbl->DiscardResource(graphics_list,
                                               resource->default_resource,
                                               &discard_region);

        graphics_list->lpVtbl->DiscardResource(graphics_list,
                                               resource->upload_resources[engine->gpu_manager.current_buffer],
                                               &discard_region);
    }
    else
    {
        u64 resource_size = resource->resource_desc.Width
                          * resource->resource_desc.Height
                          * resource->resource_desc.DepthOrArraySize;

        CopyMemory(resource->upload_pointers[engine->gpu_manager.current_buffer],
                   data,
                   resource_size);

        D3D12_RESOURCE_BARRIER resource_barrier;
        resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resource_barrier.Transition.pResource   = resource->default_resource;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        resource_barrier.Transition.StateBefore = resource->initial_state;
        resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

        graphics_list->lpVtbl->ResourceBarrier(graphics_list, 1, &resource_barrier);

        graphics_list->lpVtbl->CopyResource(graphics_list,
                                            resource->default_resource,
                                            resource->upload_resources[engine->gpu_manager.current_buffer]);

        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        resource_barrier.Transition.StateAfter  = resource->initial_state;

        graphics_list->lpVtbl->ResourceBarrier(graphics_list, 1, &resource_barrier);
    }
}

void SetGPUResourceDataByName(
    in  Engine      *engine,
    in  const char  *name,
    opt u32          name_len,
    in  void        *data)
{
    Check(engine);
    Check(name);

    if (!name_len) name_len = cast(u32, strlen(name));
    CheckM(name_len < ArrayCountInStruct(GPUResource, name),
           "Too long name, max name length is %I32u.",
           ArrayCountInStruct(GPUResource, name));

    GPUResource *resource = null;

    for (GPUResource *it = engine->gpu_memory_manager.buffer_memory.resources;
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

    if (!resource)
    {
        for (GPUResource *it = engine->gpu_memory_manager.texture_memory.resources;
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
    }

    // @TODO(Roman): Better message?
    CheckM(resource, "Resource not found. Expected name: %s", name);

    SetGPUResourceData(engine, resource, data);
}

void BindVertexBuffer(
    in Engine      *engine,
    in GPUResource *resource)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = resource->default_resource->lpVtbl->GetGPUVirtualAddress(resource->default_resource);
    vbv.SizeInBytes    = resource->vertex_index_count * resource->vertex_index_stride;
    vbv.StrideInBytes  = resource->vertex_index_stride;

    graphics_list->lpVtbl->IASetVertexBuffers(graphics_list, 0, 1, &vbv);
}

void BindIndxeBuffer(
    in Engine      *engine,
    in GPUResource *resource)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = resource->default_resource->lpVtbl->GetGPUVirtualAddress(resource->default_resource);
    ibv.SizeInBytes    = resource->vertex_index_count * resource->vertex_index_count;
    ibv.Format         = resource->resource_desc.Format;

    graphics_list->lpVtbl->IASetIndexBuffer(graphics_list, &ibv);
}

void DrawVertices(
    in Engine      *engine,
    in GPUResource *resource)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
    graphics_list->lpVtbl->DrawInstanced(graphics_list, resource->vertex_index_count, 1, 0, 0);
}

void DrawIndices(
    in Engine      *engine,
    in GPUResource *resource)
{
    Check(engine);
    Check(resource);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
    graphics_list->lpVtbl->DrawIndexedInstanced(graphics_list, resource->vertex_index_count, 1, 0, 0, 0);
}
