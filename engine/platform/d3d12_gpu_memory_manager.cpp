//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/pch.h"
#include "platform/d3d12_gpu_memory_manager.h"
#include "renderer/graphics_api.h"

namespace D3D12 {

//
// GPUResource
//

GPUResource::GPUResource()
    : m_DefaultResource(null),
      m_UploadResources{null},
      m_UploadPointers{null},
      m_Kind(KIND::UNKNOWN),
      m_AllocationState(ALLOCATION_STATE::NONE),
      m_DescHeap(null),
      m_VertexIndexCount(0),
      m_VertexIndexStride(0),
      m_InitialState(D3D12_RESOURCE_STATE_COMMON),
      m_Name(),
      m_NextFree(null),
      m_ResourceDesc()
{
}

GPUResource::~GPUResource()
{
    SafeRelease(m_DefaultResource);

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(m_UploadResources[i]);
        m_UploadPointers[i] = null;
    }
    
    m_Kind            = KIND::UNKNOWN;
    m_AllocationState = ALLOCATION_STATE::NONE;
    m_DescHeap        = null;
    m_NextFree        = null;
}

void GPUResource::SetName(const StaticString<64>& name)
{
    m_Name = name;

    StaticString<16 + 64> ext_name;
    ext_name += "__default__";
    ext_name += m_Name;

    HRESULT error = m_DefaultResource->SetPrivateData(WKPDID_D3DDebugObjectName, cast<u32>(ext_name.Length()), ext_name.Data());
    Check(SUCCEEDED(error));

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ext_name.Clear();

        char str_i[64];
        _itoa(i, str_i, 10);

        ext_name += "__upload_";
        ext_name += str_i;
        ext_name += "__";
        ext_name += m_Name;

        error = m_UploadResources[i]->SetPrivateData(WKPDID_D3DDebugObjectName, cast<u32>(ext_name.Length()), ext_name.Data());
        Check(SUCCEEDED(error));
    }
}

void GPUResource::SetData(const void *data)
{
    Renderer                  *renderer      = cast<Renderer *>(GraphicsAPI::GetRenderer());
    ID3D12GraphicsCommandList *graphics_list = renderer->CurrentGraphicsList();

    if (!data)
    {
        D3D12_DISCARD_REGION discard_region;
        discard_region.NumRects         = 0;
        discard_region.pRects           = null;
        discard_region.FirstSubresource = 0;
        discard_region.NumSubresources  = 1;

        graphics_list->DiscardResource(m_DefaultResource, &discard_region);
        graphics_list->DiscardResource(m_UploadResources[renderer->CurrentBuffer()], &discard_region);
    }
    else
    {
        u64 resource_size = m_ResourceDesc.Width
                          * m_ResourceDesc.Height
                          * m_ResourceDesc.DepthOrArraySize;

        CopyMemory(m_UploadPointers[renderer->CurrentBuffer()], data, resource_size);

        D3D12_RESOURCE_BARRIER resource_barrier;
        resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resource_barrier.Transition.pResource   = m_DefaultResource;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        resource_barrier.Transition.StateBefore = m_InitialState;
        resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

        graphics_list->ResourceBarrier(1, &resource_barrier);

        graphics_list->CopyResource(m_DefaultResource, m_UploadResources[renderer->CurrentBuffer()]);

        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        resource_barrier.Transition.StateAfter  = m_InitialState;

        graphics_list->ResourceBarrier(1, &resource_barrier);
    }
}

void GPUResource::SetDataImmediate(const void *data)
{
    Renderer                  *renderer           = cast<Renderer *>(GraphicsAPI::GetRenderer());
    GPUMemoryManager          *gpu_memory_manager = cast<GPUMemoryManager *>(GraphicsAPI::GetGPUMemoryManager());
    ID3D12GraphicsCommandList *command_list       = gpu_memory_manager->CommandList();
    
    if (!data)
    {
        D3D12_DISCARD_REGION discard_region;
        discard_region.NumRects         = 0;
        discard_region.pRects           = null;
        discard_region.FirstSubresource = 0;
        discard_region.NumSubresources  = 1;

        command_list->DiscardResource(m_DefaultResource, &discard_region);
        command_list->DiscardResource(m_UploadResources[renderer->CurrentBuffer()], &discard_region);
    }
    else
    {
        u64 resource_size = m_ResourceDesc.Width
                          * m_ResourceDesc.Height
                          * m_ResourceDesc.DepthOrArraySize;

        CopyMemory(m_UploadPointers[renderer->CurrentBuffer()], data, resource_size);

        D3D12_RESOURCE_BARRIER resource_barrier;
        resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resource_barrier.Transition.pResource   = m_DefaultResource;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        resource_barrier.Transition.StateBefore = m_InitialState;
        resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

        command_list->ResourceBarrier(1, &resource_barrier);

        command_list->CopyResource(m_DefaultResource, m_UploadResources[renderer->CurrentBuffer()]);

        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        resource_barrier.Transition.StateAfter  = m_InitialState;

        command_list->ResourceBarrier(1, &resource_barrier);
    }
}

GPUResource& GPUResource::operator=(GPUResource&& other) noexcept
{
    if (this != &other)
    {
        // moving vtable
        *cast<u64 *>(this)   = *cast<u64 *>(&other);
        *cast<u64 *>(&other) = 0;

        m_DefaultResource = other.m_DefaultResource;
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            ID3D12Resource *& other_upload_resource = other.m_UploadResources[i];
            void           *& other_upload_pointer  = other.m_UploadPointers[i];

            m_UploadResources[i] = other_upload_resource;
            m_UploadPointers[i]  = other_upload_pointer;

            other_upload_resource = null;
            other_upload_pointer  = null;
        }
        m_Kind              = other.m_Kind;
        m_AllocationState   = other.m_AllocationState;
        m_DescHeap          = other.m_DescHeap;
        m_VertexIndexCount  = other.m_VertexIndexCount;
        m_VertexIndexStride = other.m_VertexIndexStride;
        m_InitialState      = other.m_InitialState;
        m_Name              = other.m_Name;
        m_NextFree          = other.m_NextFree;
        m_ResourceDesc      = other.m_ResourceDesc;

        other.m_DefaultResource = null;
        other.m_DescHeap        = null;
        other.m_NextFree        = null;
    }
    return *this;
}

//
// GPUDescHeap
//

GPUDescHeap::GPUDescHeap()
    : m_Handle(null),
      m_Resource(null),
      m_NextFree(null),
      m_AllocationState(ALLOCATION_STATE::NONE),
      m_DescHeapDesc()
{
}

GPUDescHeap::~GPUDescHeap()
{
    if (m_Handle) SafeRelease(m_Handle);
    m_Resource        = null;
    m_NextFree        = null;
    m_AllocationState = ALLOCATION_STATE::NONE;
}

GPUDescHeap& GPUDescHeap::operator=(GPUDescHeap&& other) noexcept
{
    if (this != &other)
    {
        m_Handle          = other.m_Handle;
        m_Resource        = other.m_Resource;
        m_NextFree        = other.m_NextFree;
        m_AllocationState = other.m_AllocationState;
        m_DescHeapDesc    = RTTI::move(other.m_DescHeapDesc);

        other.m_Handle          = null;
        other.m_Resource        = null;
        other.m_NextFree        = null;
        other.m_AllocationState = ALLOCATION_STATE::NONE;
    }
    return *this;
}

//
// GPUResourceMemory
//

GPUResourceMemory::GPUResourceMemory(Allocator *allocator)
    : m_Resources(allocator),
      m_DefaultHeap(null),
      m_DefaultOffset(0),
      m_DefaultCapacity(0),
      m_UploadHeaps{null},
      m_UploadOffsets{0},
      m_UploadCapacities{0},
      m_AllowOnlyBuffers(false)
{
}

GPUResourceMemory::~GPUResourceMemory()
{
    SafeRelease(m_DefaultHeap);

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(m_UploadHeaps[i]);
    }
}

void GPUResourceMemory::Create(const D3D12_HEAP_DESC& default_desc, const D3D12_HEAP_DESC& upload_desc, bool allow_only_buffers)
{
    m_AllowOnlyBuffers = allow_only_buffers;

    Check(  ( m_AllowOnlyBuffers &&  default_desc.Flags & D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS             &&  upload_desc.Flags & D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS)
          ^ (!m_AllowOnlyBuffers && (default_desc.Flags & D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES) && (upload_desc.Flags & D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES)));

    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    if (m_AllowOnlyBuffers)
    {
        m_DefaultCapacity = default_desc.SizeInBytes;

        HRESULT error = renderer->Device()->CreateHeap(&default_desc, IID_PPV_ARGS(&m_DefaultHeap));
        CheckM(error != E_OUTOFMEMORY, "There is no enough GPU memory to create Default Buffer Heap. Requested capacity: %I64u", m_DefaultCapacity);
        Check(SUCCEEDED(error));

        error = m_DefaultHeap->SetPrivateData(WKPDID_D3DDebugObjectName, CSTRLEN("Default Buffer Heap"), "Default Buffer Heap");
        Check(SUCCEEDED(error));

        char heap_name[32];
        s32  heap_name_len = 0;

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            m_UploadCapacities[i] = upload_desc.SizeInBytes;

            ID3D12Heap **upload_heap = m_UploadHeaps + i;

            error = renderer->Device()->CreateHeap(&upload_desc, IID_PPV_ARGS(upload_heap));
            CheckM(error != E_OUTOFMEMORY, "There is no enough GPU memory to create Upload Buffer Heap. Requested capacity: %I64u", upload_desc.SizeInBytes);
            Check(SUCCEEDED(error));

            heap_name_len = sprintf(heap_name, "Upload Buffer Heap #%I32u", i);
            error = (*upload_heap)->SetPrivateData(WKPDID_D3DDebugObjectName, heap_name_len, heap_name);
            Check(SUCCEEDED(error));
        }
    }
    else
    {
        m_DefaultCapacity = default_desc.SizeInBytes;

        HRESULT error = renderer->Device()->CreateHeap(&default_desc, IID_PPV_ARGS(&m_DefaultHeap));
        CheckM(error != E_OUTOFMEMORY, "There is no enough GPU memory to create Default Texture Heap. Requested capacity: %I64u", m_DefaultCapacity);
        Check(SUCCEEDED(error));

        error = m_DefaultHeap->SetPrivateData(WKPDID_D3DDebugObjectName, CSTRLEN("Default Texture Heap"), "Default Texture Heap");
        Check(SUCCEEDED(error));

        char heap_name[32];
        s32  heap_name_len = 0;

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            m_UploadCapacities[i] = upload_desc.SizeInBytes;

            ID3D12Heap **upload_heap = m_UploadHeaps + i;

            error = renderer->Device()->CreateHeap(&upload_desc, IID_PPV_ARGS(upload_heap));
            CheckM(error != E_OUTOFMEMORY, "There is no enough GPU memory to create Upload Texture Heap. Requested capacity: %I64u", upload_desc.SizeInBytes);
            Check(SUCCEEDED(error));

            heap_name_len = sprintf(heap_name, "Upload Texture Heap #%I32u", i);
            error = (*upload_heap)->SetPrivateData(WKPDID_D3DDebugObjectName, heap_name_len, heap_name);
            Check(SUCCEEDED(error));
        }
    }
}

void GPUResourceMemory::Reset()
{
    m_Resources.ForEach([&](GPUResourceList::Node *it)
    {
        GPUResource& resource = it->Data();

        if (resource.m_AllocationState == GPUResource::ALLOCATION_STATE::ALLOCATED)
        {
            resource.m_AllocationState = GPUResource::ALLOCATION_STATE::IN_FREE_LIST;

            GPUResourceList::Node *prev_free = null;

            for (GPUResourceList::Node *inner_it = it->Prev(); inner_it; inner_it = inner_it->Prev())
            {
                if (inner_it->Data().m_AllocationState == GPUResource::ALLOCATION_STATE::IN_FREE_LIST)
                {
                    prev_free = inner_it;
                    break;
                }
            }

            if (prev_free)
            {
                prev_free->Data().m_NextFree = &resource;
            }
            else
            {
                m_FreeList = &resource;
            }

            if (resource.m_DescHeap)
            {
                resource.m_DescHeap->m_Resource        = null;
                resource.m_DescHeap->m_AllocationState = GPUDescHeap::ALLOCATION_STATE::IN_FREE_LIST;
                resource.m_DescHeap                    = null;
            }
        }
    });
}

void GPUResourceMemory::Release()
{
    m_Resources.Clear();
    m_FreeList = null;
}

GPUResource *GPUResourceMemory::FindOrAllocateResource(const D3D12_RESOURCE_DESC& resource_desc, bool& allocated)
{
    GPUResource *prev_free = null;

    for (GPUResource *it = m_FreeList; it; prev_free = it, it = it->m_NextFree)
    {
        if (RtlEqualMemory(&it->m_ResourceDesc, &resource_desc, sizeof(D3D12_RESOURCE_DESC)))
        {
            it->m_AllocationState = GPUResource::ALLOCATION_STATE::ALLOCATED;
            allocated             = false;

            if (prev_free)
            {
                prev_free = it->m_NextFree;
            }

            return it;
        }
    }

    GPUResource resource;
    resource.m_AllocationState = GPUResource::ALLOCATION_STATE::ALLOCATED;
    resource.m_ResourceDesc    = resource_desc;

    m_Resources.PushBack(RTTI::move(resource));

    allocated = true;
    return &m_Resources.Last()->Data();
}

void GPUResourceMemory::CreateGPUResource(GPUResource *resource)
{
    Check(resource->m_AllocationState == GPUResource::ALLOCATION_STATE::ALLOCATED);

    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    D3D12_DISCARD_REGION discard_region;
    discard_region.NumRects         = 0;
    discard_region.pRects           = null;
    discard_region.FirstSubresource = 0;
    discard_region.NumSubresources  = 1;

    HRESULT error = renderer->Device()->CreatePlacedResource(m_DefaultHeap,
                                                             m_DefaultOffset,
                                                             &resource->m_ResourceDesc,
                                                             resource->m_InitialState,
                                                             null,
                                                             IID_PPV_ARGS(&resource->m_DefaultResource));

    if (m_AllowOnlyBuffers) { CheckM(error != E_OUTOFMEMORY, "Default Buffer Memory Overflow");  }
    else                    { CheckM(error != E_OUTOFMEMORY, "Default Texture Memory Overflow"); }
    Check(SUCCEEDED(error));

    m_DefaultOffset += resource->m_ResourceDesc.Width
                    *  resource->m_ResourceDesc.Height
                    *  resource->m_ResourceDesc.DepthOrArraySize;
    m_DefaultOffset  = AlignUp(m_DefaultOffset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    if (m_AllowOnlyBuffers)
    {
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            u64& upload_offset = m_UploadOffsets[i];

            error = renderer->Device()->CreatePlacedResource(m_UploadHeaps[i],
                                                             upload_offset,
                                                             &resource->m_ResourceDesc,
                                                             D3D12_RESOURCE_STATE_GENERIC_READ,
                                                             null,
                                                             IID_PPV_ARGS(resource->m_UploadResources + i));
            CheckM(error != E_OUTOFMEMORY, "Upload Buffer Memory Overflow");
            Check(SUCCEEDED(error));

            upload_offset += resource->m_ResourceDesc.Width
                          *  resource->m_ResourceDesc.Height
                          *  resource->m_ResourceDesc.DepthOrArraySize;
            upload_offset  = AlignUp(upload_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

            D3D12_RANGE read_range = { 0, 0 };
            error = resource->m_UploadResources[i]->Map(0, &read_range, resource->m_UploadPointers + i);
            Check(SUCCEEDED(error));
        }
    }
    else
    {
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            u64& upload_offset = m_UploadOffsets[i];

            error = renderer->Device()->CreatePlacedResource(m_UploadHeaps[i],
                                                             upload_offset,
                                                             &resource->m_ResourceDesc,
                                                             D3D12_RESOURCE_STATE_GENERIC_READ,
                                                             null,
                                                             IID_PPV_ARGS(resource->m_UploadResources + i));
            CheckM(error != E_OUTOFMEMORY, "Upload Texture Memory Overflow");
            Check(SUCCEEDED(error));

            upload_offset += resource->m_ResourceDesc.Width
                          *  resource->m_ResourceDesc.Height
                          *  resource->m_ResourceDesc.DepthOrArraySize;
            upload_offset  = AlignUp(upload_offset, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

            D3D12_RANGE read_range = { 0, 0 };
            error = resource->m_UploadResources[i]->Map(0, &read_range, resource->m_UploadPointers + i);
            Check(SUCCEEDED(error));
        }
    }
}

GPUResource *GPUResourceMemory::FindGPUResource(const StaticString<64>& name)
{
    for (GPUResourceList::Node *it = m_Resources.First(); it; it = it->Next())
    {
        GPUResource& resource = it->Data();
        if (resource.m_Name == name)
        {
            return &resource;
        }
    }
    return null;
}

//
// GPUDescHeapMemory
//

GPUDescHeapMemory::GPUDescHeapMemory(Allocator *allocator)
    : m_DescHeaps(allocator),
      m_FreeList(null)
{
}

GPUDescHeapMemory::~GPUDescHeapMemory()
{
    Release();
}

void GPUDescHeapMemory::Release()
{
    m_DescHeaps.Clear();
    m_FreeList = null;
}

GPUDescHeap *GPUDescHeapMemory::FindOrAllocateDescHeap(const D3D12_DESCRIPTOR_HEAP_DESC& desc_heap_desc, bool& allocated)
{
    GPUDescHeap *prev_free = null;

    for (GPUDescHeap *it = m_FreeList; it; prev_free = it, it = it->m_NextFree)
    {
        if (RtlEqualMemory(&it->m_DescHeapDesc, &desc_heap_desc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC)))
        {
            it->m_AllocationState = GPUDescHeap::ALLOCATION_STATE::ALLOCATED;
            allocated             = false;

            if (prev_free)
            {
                prev_free = it->m_NextFree;
            }

            return it;
        }
    }

    GPUDescHeap desc_heap;
    desc_heap.m_AllocationState = GPUDescHeap::ALLOCATION_STATE::ALLOCATED;
    desc_heap.m_DescHeapDesc    = desc_heap_desc;

    m_DescHeaps.PushBack(RTTI::move(desc_heap));

    allocated = true;
    return &m_DescHeaps.Last()->Data();
}

void GPUDescHeapMemory::CreateDescHeapForCB(GPUDescHeap *desc_heap, GPUResource *resource)
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());
    
    HRESULT error = renderer->Device()->CreateDescriptorHeap(&desc_heap->m_DescHeapDesc, IID_PPV_ARGS(&desc_heap->m_Handle));
    Check(SUCCEEDED(error));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    cbv_desc.BufferLocation = resource->m_DefaultResource->GetGPUVirtualAddress();
    cbv_desc.SizeInBytes    = cast<u32>(resource->m_ResourceDesc.Width);

    renderer->Device()->CreateConstantBufferView(&cbv_desc, desc_heap->m_Handle->GetCPUDescriptorHandleForHeapStart());
}

//
// GPUMemoryManager
//

GPUMemoryManager::GPUMemoryManager(Allocator *allocator, const Logger& logger, u64 gpu_memory_capacity)
    : m_Allocator(allocator),
      m_CommandAllocator(null),
      m_CommandList(null),
      m_Fence(null),
      m_FenceEvent(),
      m_Logger(logger),
      m_DescHeapMemory(allocator),
      m_BufferMemory(allocator),
      m_TextureMemory(allocator)
{
    CheckM(gpu_memory_capacity, "GPU memory capacity can't be 0");

    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    m_Logger.LogInfo("GPU Resource Heap Tier: D3D12_RESOURCE_HEAP_TIER_%I32d", renderer->ResourceHeapTier());

    gpu_memory_capacity = AlignUp(gpu_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    u64 gpu_buffer_memory_cap  = AlignUp(cast<u64>(0.4f * gpu_memory_capacity), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    u64 gpu_texture_memory_cap = AlignUp(cast<u64>(0.6f * gpu_memory_capacity), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    D3D12_HEAP_DESC default_buffer_heap_desc = {0};
    default_buffer_heap_desc.SizeInBytes                     = gpu_buffer_memory_cap;
    default_buffer_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    default_buffer_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    default_buffer_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    default_buffer_heap_desc.Properties.CreationNodeMask     = 0;
    default_buffer_heap_desc.Properties.VisibleNodeMask      = 0;
    default_buffer_heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    default_buffer_heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS
                                                             | D3D12_HEAP_FLAG_SHARED;

    D3D12_HEAP_DESC default_texture_heap_desc = {0};
    default_texture_heap_desc.SizeInBytes                     = gpu_texture_memory_cap;
    default_texture_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    default_texture_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    default_texture_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    default_texture_heap_desc.Properties.CreationNodeMask     = 0;
    default_texture_heap_desc.Properties.VisibleNodeMask      = 0;
    default_texture_heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    default_texture_heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES
                                                              | D3D12_HEAP_FLAG_SHARED;

    D3D12_HEAP_DESC upload_buffer_heap_desc = {0};
    upload_buffer_heap_desc.SizeInBytes                     = gpu_buffer_memory_cap;
    upload_buffer_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    upload_buffer_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    upload_buffer_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    upload_buffer_heap_desc.Properties.CreationNodeMask     = 0;
    upload_buffer_heap_desc.Properties.VisibleNodeMask      = 0;
    upload_buffer_heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    upload_buffer_heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    D3D12_HEAP_DESC upload_texture_heap_desc = {0};
    upload_texture_heap_desc.SizeInBytes                     = gpu_texture_memory_cap;
    upload_texture_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    upload_texture_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    upload_texture_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    upload_texture_heap_desc.Properties.CreationNodeMask     = 0;
    upload_texture_heap_desc.Properties.VisibleNodeMask      = 0;
    upload_texture_heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    upload_texture_heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;

    m_BufferMemory.Create(default_buffer_heap_desc, upload_buffer_heap_desc, true);
    m_TextureMemory.Create(default_texture_heap_desc, upload_texture_heap_desc, false);
    
    HRESULT error = renderer->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator));
    Check(SUCCEEDED(error));

    error = renderer->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator, null, IID_PPV_ARGS(&m_CommandList));
    Check(SUCCEEDED(error));

    error = m_CommandList->Close();
    Check(SUCCEEDED(error));

    error = renderer->Device()->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_Fence));
    Check(SUCCEEDED(error));
}

GPUMemoryManager::~GPUMemoryManager()
{
    Release();

    SafeRelease(m_CommandAllocator);
    SafeRelease(m_CommandList);
    SafeRelease(m_Fence);
}

void GPUMemoryManager::Destroy()
{
    this->~GPUMemoryManager();
}

IGPUResource *GPUMemoryManager::AllocateVB(u32 vertex_count, u32 stride)
{
    Check(vertex_count);
    Check(stride);

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = vertex_count * stride;
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

#if 0
    CheckM(1 <= resource_desc.Width && resource_desc.Width <= .features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           "Too big vertex buffer. Required vertex buffer size bytes = [1, %I32u], got: %I32u",
           .features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           resource_desc.Width);
#endif

    bool         allocated = false;
    GPUResource *resource  = m_BufferMemory.FindOrAllocateResource(resource_desc, allocated);

    resource->m_DescHeap          = null;
    resource->m_Kind              = GPUResource::KIND::VB;
    resource->m_VertexIndexCount  = vertex_count;
    resource->m_VertexIndexStride = stride;
    resource->m_InitialState      = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    if (allocated)
    {
        m_BufferMemory.CreateGPUResource(resource);
    }

    return resource;
}

IGPUResource *GPUMemoryManager::AllocateIB(u32 indecies_count)
{
    Check(indecies_count);

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = indecies_count * sizeof(u32);
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

#if 0
    CheckM(1 <= resource_desc.Width && resource_desc.Width <= .features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           "Too big index buffer. Required indices buffer size bytes = [1, %I32u], got: %I32u",
           .features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           resource_desc.Width);
#endif

    bool         allocated = false;
    GPUResource *resource  = m_BufferMemory.FindOrAllocateResource(resource_desc, allocated);

    resource->m_DescHeap          = null;
    resource->m_Kind              = GPUResource::KIND::IB;
    resource->m_VertexIndexCount  = indecies_count;
    resource->m_VertexIndexStride = sizeof(u32);
    resource->m_InitialState      = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    if (allocated)
    {
        m_BufferMemory.CreateGPUResource(resource);
    }

    return resource;
}

IGPUResource *GPUMemoryManager::AllocateCB(u32 bytes, const StaticString<64>& name)
{
    Check(bytes && bytes <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

#if 0
    CheckM(1 <= bytes && bytes <= .features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           "Too big constant buffer. Required constant buffer size bytes = [1, %I32u], got: %I32u",
           .features.virtual_address.MaxGPUVirtualAddressBitsPerResource,
           bytes);
#endif

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = bytes;
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    bool         allocated = false;
    GPUResource *resource  = m_BufferMemory.FindOrAllocateResource(resource_desc, allocated);

    resource->m_DescHeap          = null;
    resource->m_Kind              = GPUResource::KIND::CB;
    resource->m_VertexIndexCount  = 0;
    resource->m_VertexIndexStride = 0;
    resource->m_InitialState      = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    if (allocated)
    {
        m_BufferMemory.CreateGPUResource(resource);
    }

    resource->SetName(name);

    D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc;
    desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc_heap_desc.NumDescriptors = 1;
    desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc_heap_desc.NodeMask       = 0;

    GPUDescHeap *desc_heap = m_DescHeapMemory.FindOrAllocateDescHeap(desc_heap_desc, allocated);

    if (allocated)
    {
        m_DescHeapMemory.CreateDescHeapForCB(desc_heap, resource);
    }

    resource->m_DescHeap  = desc_heap;
    desc_heap->m_Resource = resource;

    return resource;
}

void GPUMemoryManager::SetGPUResourceData(const StaticString<64>& name, const void *data)
{
    IGPUResource *resource = m_BufferMemory.FindGPUResource(name);
    if (!resource) resource = m_TextureMemory.FindGPUResource(name);

    if (resource) resource->SetData(data);
    else          m_Logger.LogError("There is no buffer nor texture with the name \"%s\"", name);
}

void GPUMemoryManager::SetGPUResourceDataImmediate(const StaticString<64>& name, const void *data)
{
    IGPUResource *resource = m_BufferMemory.FindGPUResource(name);
    if (!resource) resource = m_TextureMemory.FindGPUResource(name);

    if (resource) resource->SetDataImmediate(data);
    else          m_Logger.LogError("There is no buffer nor texture with the name \"%s\"", name);
}

void GPUMemoryManager::StartImmediateExecution()
{
    HRESULT error = m_CommandAllocator->Reset();
    Check(SUCCEEDED(error));

    error = m_CommandList->Reset(m_CommandAllocator, null);
    Check(SUCCEEDED(error));
}

void GPUMemoryManager::EndImmediateExecution()
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    HRESULT error = m_CommandList->Close();
    Check(SUCCEEDED(error));

    ID3D12CommandList *command_lists[] = { m_CommandList };
    renderer->GraphicsQueue()->ExecuteCommandLists(cast<u32>(ArrayCount(command_lists)), command_lists);

    u64 fence_value = m_Fence->GetCompletedValue() + 1;

    error = renderer->GraphicsQueue()->Signal(m_Fence, fence_value);
    Check(SUCCEEDED(error));

    if (m_Fence->GetCompletedValue() < fence_value)
    {
        error = m_Fence->SetEventOnCompletion(fence_value, m_FenceEvent);
        Check(SUCCEEDED(error));

        m_FenceEvent.Wait(INFINITE, true);
    }
}

void GPUMemoryManager::Reset()
{
    m_BufferMemory.Reset();
    m_TextureMemory.Reset();
}

void GPUMemoryManager::Release()
{
    m_BufferMemory.Release();
    m_TextureMemory.Release();
    m_DescHeapMemory.Release();
}

};
