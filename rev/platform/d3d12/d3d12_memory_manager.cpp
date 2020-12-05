//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "platform/d3d12/d3d12_memory_manager.h"
#include "graphics/graphics_api.h"
#include "core/memory.h"
#include "platform/d3d12/d3d12_common.h"

namespace REV::D3D12
{

MemoryManager::MemoryManager(Allocator *allocator)
    : m_CommandAllocator(null),
      m_CommandList(null),
      m_Fence(null),
      m_FenceEvent("D3D12::MemoryManager Event"),
      m_DescHeapMemory(allocator),
      m_BufferMemory(allocator),
      m_TextureMemory(allocator)
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    HRESULT error = renderer->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator));
    REV_CHECK(CheckResultAndPrintMessages(error));

    error = renderer->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator, null, IID_PPV_ARGS(&m_CommandList));
    REV_CHECK(CheckResultAndPrintMessages(error));

    error = m_CommandList->Close();
    REV_CHECK(CheckResultAndPrintMessages(error));

    error = renderer->Device()->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_Fence));
    REV_CHECK(CheckResultAndPrintMessages(error));
}

MemoryManager::~MemoryManager()
{
    if (m_CommandAllocator)
    {
        FreeMemory();
        SafeRelease(m_CommandAllocator);
        SafeRelease(m_CommandList);
        SafeRelease(m_Fence);
    }
}

u64 MemoryManager::AllocateVertexBuffer(u32 vertex_count, u32 vertex_stride, const StaticString<64>& name)
{
    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(vertex_count * vertex_stride, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, index);

    buffer->kind    = BUFFER_KIND::VERTEX_BUFFER;
    buffer->vcount  = vertex_count;
    buffer->vstride = vertex_stride;
    buffer->name    = name;

    return index;
}

u64 MemoryManager::AllocateIndexBuffer(u32 index_count, const StaticString<64>& name)
{
    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(index_count * sizeof(u32), D3D12_RESOURCE_STATE_INDEX_BUFFER, index);

    buffer->kind    = BUFFER_KIND::INDEX_BUFFER;
    buffer->vcount  = index_count;
    buffer->vstride = sizeof(u32);
    buffer->name    = name;

    return index;
}

u64 MemoryManager::AllocateConstantBuffer(u32 bytes, const StaticString<64>& name)
{
    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(bytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, index);

    buffer->kind = BUFFER_KIND::CONSTANT_BUFFER;
    buffer->name = name;

    DescHeap *desc_heap = CreateDescHeapForConstantBuffer(buffer, index);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    cbv_desc.BufferLocation = m_BufferMemory.pages[buffer->page_index].def_mem->GetGPUVirtualAddress() + buffer->offset;
    cbv_desc.SizeInBytes    = cast<u32>(buffer->aligned_size);

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateConstantBufferView(&cbv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

void MemoryManager::SetBufferData(const Buffer& buffer, const void *data)
{
    SetBufferData(cast<Renderer *>(GraphicsAPI::GetRenderer())->CurrentGraphicsList(), buffer, data);
}

void MemoryManager::SetBufferDataImmediate(const Buffer& buffer, const void *data)
{
    SetBufferData(m_CommandList, buffer, data);
}

void MemoryManager::StartImmediateExecution()
{
    HRESULT error = m_CommandAllocator->Reset();
    REV_CHECK(CheckResultAndPrintMessages(error));

    error = m_CommandList->Reset(m_CommandAllocator, null);
    REV_CHECK(CheckResultAndPrintMessages(error));
}

void MemoryManager::EndImmediateExecution()
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    HRESULT error = m_CommandList->Close();
    REV_CHECK(CheckResultAndPrintMessages(error));

    ID3D12CommandList *command_lists[] = { m_CommandList };
    renderer->GraphicsQueue()->ExecuteCommandLists(cast<u32>(ArrayCount(command_lists)), command_lists);

    u64 fence_value = m_Fence->GetCompletedValue() + 1;

    error = renderer->GraphicsQueue()->Signal(m_Fence, fence_value);
    REV_CHECK(CheckResultAndPrintMessages(error));

    if (m_Fence->GetCompletedValue() < fence_value)
    {
        error = m_Fence->SetEventOnCompletion(fence_value, m_FenceEvent.Handle());
        REV_CHECK(CheckResultAndPrintMessages(error));

        m_FenceEvent.Wait(INFINITE, true);
    }
}

void MemoryManager::FreeMemory()
{
    m_BufferMemory.buffers.Clear();
    for (BufferMemoryPage& page : m_BufferMemory.pages)
    {
        SafeRelease(page.def_mem);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(page.upl_mem[i]);
        }
    }
    m_BufferMemory.pages.Clear();

    for (Texture& texture : m_TextureMemory.textures)
    {
        SafeRelease(texture.def_resoucre);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(texture.upl_resource[i]);
        }
    }
    m_TextureMemory.textures.Clear();

    for (DescHeap& desc_heap : m_DescHeapMemory.desc_heaps)
    {
        SafeRelease(desc_heap.handle);
    }
    m_DescHeapMemory.desc_heaps.Clear();
}

MemoryManager& MemoryManager::operator=(MemoryManager&& other) noexcept
{
    if (this != &other)
    {
        m_CommandAllocator = other.m_CommandAllocator;
        m_CommandList      = other.m_CommandList;
        m_Fence            = other.m_Fence;
        m_FenceEvent       = RTTI::move(other.m_FenceEvent);
        m_DescHeapMemory   = RTTI::move(other.m_DescHeapMemory);
        m_BufferMemory     = RTTI::move(other.m_BufferMemory);
        m_TextureMemory    = RTTI::move(other.m_TextureMemory);

        other.m_CommandAllocator = null;
        other.m_CommandList      = null;
        other.m_Fence            = null;
    }
    return *this;
}

void MemoryManager::CreateNewPage(D3D12_RESOURCE_STATES initial_state)
{
    D3D12_HEAP_PROPERTIES default_heap_properties;
    default_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    default_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    default_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    default_heap_properties.CreationNodeMask     = 0;
    default_heap_properties.VisibleNodeMask      = 0;

    D3D12_RESOURCE_DESC resource_desc;
    resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resource_desc.Width              = BUFFER_MEMORY_PAGE_SIZE;
    resource_desc.Height             = 1;
    resource_desc.DepthOrArraySize   = 1;
    resource_desc.MipLevels          = 1;
    resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count   = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    BufferMemoryPage *page = m_BufferMemory.pages.PushBack();
    page->initial_state    = initial_state;

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();

    HRESULT error = device->CreateCommittedResource(&default_heap_properties,
                                                    D3D12_HEAP_FLAG_NONE | D3D12_HEAP_FLAG_SHARED,
                                                    &resource_desc,
                                                    initial_state,
                                                    null,
                                                    IID_PPV_ARGS(&page->def_mem));
    REV_CHECK(CheckResultAndPrintMessages(error));

    D3D12_HEAP_PROPERTIES upload_heap_properties;
    upload_heap_properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    upload_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    upload_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    upload_heap_properties.CreationNodeMask     = 0;
    upload_heap_properties.VisibleNodeMask      = 0;

    D3D12_RANGE read_range = {0, 0};

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        error = device->CreateCommittedResource(&upload_heap_properties,
                                                D3D12_HEAP_FLAG_NONE,
                                                &resource_desc,
                                                D3D12_RESOURCE_STATE_GENERIC_READ,
                                                null,
                                                IID_PPV_ARGS(page->upl_mem + i));
        REV_CHECK(CheckResultAndPrintMessages(error));

        error = page->upl_mem[i]->Map(0, &read_range, cast<void **>(page->upl_ptrs + i));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }
}

Buffer *MemoryManager::AllocateBuffer(u64 size, D3D12_RESOURCE_STATES initial_state, u64& index)
{
    index = m_BufferMemory.buffers.Count();

    Buffer *buffer          = m_BufferMemory.buffers.PushBack();
    buffer->page_index      = REV_U64_MAX;
    buffer->offset          = REV_U64_MAX;
    buffer->kind            = BUFFER_KIND::UNKNOWN;
    buffer->desc_heap_index = REV_U64_MAX;
    buffer->vcount          = 0;
    buffer->vstride         = 0;
    buffer->actual_size     = size;
    buffer->aligned_size    = AlignUp(size, 256);

    u64 page_index = 0;
    for (BufferMemoryPage& page : m_BufferMemory.pages)
    {
        if (page.initial_state == initial_state && page.occupied_bytes <= BUFFER_MEMORY_PAGE_SIZE - buffer->aligned_size)
        {
            buffer->page_index = page_index;
            buffer->offset     = page.occupied_bytes;

            page.occupied_bytes += buffer->aligned_size;
            break;
        }
        ++page_index;
    }

    if (buffer->page_index == REV_U64_MAX)
    {
        CreateNewPage(initial_state);

        buffer->page_index = m_BufferMemory.pages.Count() - 1;
        buffer->offset     = 0;

        m_BufferMemory.pages.Last().occupied_bytes = buffer->aligned_size;
    }

    return buffer;
}

DescHeap *MemoryManager::CreateDescHeapForConstantBuffer(Buffer *buffer, u64 resource_index)
{
    buffer->desc_heap_index = m_DescHeapMemory.desc_heaps.Count();

    D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc;
    desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc_heap_desc.NumDescriptors = 1;
    desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc_heap_desc.NodeMask       = 0;

    DescHeap *desc_heap       = m_DescHeapMemory.desc_heaps.PushBack();
    desc_heap->handle         = null;
    desc_heap->resoucre_index = resource_index;
    desc_heap->desc           = desc_heap_desc;

    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());
    
    HRESULT error = renderer->Device()->CreateDescriptorHeap(&desc_heap->desc, IID_PPV_ARGS(&desc_heap->handle));
    REV_CHECK(CheckResultAndPrintMessages(error));

    return desc_heap;
}

void MemoryManager::SetBufferData(ID3D12GraphicsCommandList *command_list, const Buffer& buffer, const void *data)
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    BufferMemoryPage& page = m_BufferMemory.pages[buffer.page_index];

    ID3D12Resource *upl_mem = page.upl_mem[renderer->CurrentBuffer()];
    byte           *upl_ptr = page.upl_ptrs[renderer->CurrentBuffer()] + buffer.offset;

    if (data) CopyMemory(upl_ptr, data, buffer.actual_size);
    else      memset_any(upl_ptr, 0ui8, buffer.actual_size);

    D3D12_RESOURCE_BARRIER resource_barrier;
    resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barrier.Transition.pResource   = page.def_mem;
    resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    resource_barrier.Transition.StateBefore = page.initial_state;
    resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

    command_list->ResourceBarrier(1, &resource_barrier);

    command_list->CopyBufferRegion(page.def_mem, buffer.offset, upl_mem, buffer.offset, buffer.actual_size);

    resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barrier.Transition.StateAfter  = page.initial_state;

    command_list->ResourceBarrier(1, &resource_barrier);
}

};
