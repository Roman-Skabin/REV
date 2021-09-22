// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/graphics_api.h"
#include "memory/memory.h"
#include "core/settings.h"

#include "platform/d3d12/d3d12_memory_manager.h"
#include "platform/d3d12/d3d12_common.h"

namespace REV::D3D12
{

MemoryManager::MemoryManager(Allocator *allocator)
    : m_DeviceContext(cast(DeviceContext *, GraphicsAPI::GetDeviceContext())),
      m_CommandAllocator(null),
      m_CommandList(null),
      m_Fence(null),
      m_FenceEvent(null),
      m_StaticMemory(allocator),
      m_SceneMemory(allocator)
{
    HRESULT error = m_DeviceContext->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator));
    REV_CHECK(CheckResultAndPrintMessages(error));

    error = m_DeviceContext->Device()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator, null, IID_PPV_ARGS(&m_CommandList));
    REV_CHECK(CheckResultAndPrintMessages(error));

    error = m_CommandList->Close();
    REV_CHECK(CheckResultAndPrintMessages(error));

    error = m_DeviceContext->Device()->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_Fence));
    REV_CHECK(CheckResultAndPrintMessages(error));

    REV_DEBUG_RESULT(m_FenceEvent = CreateEventExA(null, "REV::D3D12::MemoryManager Event", 0, EVENT_ALL_ACCESS));
}

MemoryManager::~MemoryManager()
{
    if (m_CommandAllocator)
    {
        FreeSceneMemory();
        FreeStaticMemory();
        SafeRelease(m_CommandAllocator);
        SafeRelease(m_CommandList);
        SafeRelease(m_Fence);
        if (m_FenceEvent)
        {
            REV_DEBUG_RESULT(CloseHandle(m_FenceEvent));
            m_FenceEvent = null;
        }
    }
}

u64 MemoryManager::AllocateVertexBuffer(u32 vertex_count, bool _static, const ConstString& name)
{
    REV_CHECK(vertex_count);

    BUFFER_KIND   kind          = BUFFER_KIND_VERTEX_BUFFER;
    BufferMemory *buffer_memory = null;

    if (_static)
    {
        kind          |= BUFFER_KIND_STATIC;
        buffer_memory  = &m_StaticMemory.buffer_memory;
    }
    else
    {
        buffer_memory = &m_SceneMemory.buffer_memory;
    }

    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(buffer_memory, vertex_count * sizeof(Vertex), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, index, name);

    buffer->kind    = kind;
    buffer->vcount  = vertex_count;
    buffer->vstride = sizeof(Vertex);
    buffer->name    = name;

    return index;
}

u64 MemoryManager::AllocateIndexBuffer(u32 index_count, bool _static, const ConstString& name)
{
    REV_CHECK(index_count);

    BUFFER_KIND   kind          = BUFFER_KIND_INDEX_BUFFER;
    BufferMemory *buffer_memory = null;

    if (_static)
    {
        kind          |= BUFFER_KIND_STATIC;
        buffer_memory  = &m_StaticMemory.buffer_memory;
    }
    else
    {
        buffer_memory = &m_SceneMemory.buffer_memory;
    }

    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(buffer_memory, index_count * sizeof(Index), D3D12_RESOURCE_STATE_INDEX_BUFFER, index, name);

    buffer->kind    = kind;
    buffer->icount  = index_count;
    buffer->istride = sizeof(Index);
    buffer->name    = name;

    return index;
}

u64 MemoryManager::AllocateConstantBuffer(u32 bytes, bool _static, const ConstString& name)
{
    REV_CHECK(bytes);

    BUFFER_KIND   kind          = BUFFER_KIND_CONSTANT_BUFFER;
    BufferMemory *buffer_memory = null;

    if (_static)
    {
        kind          |= BUFFER_KIND_STATIC;
        buffer_memory  = &m_StaticMemory.buffer_memory;
    }
    else
    {
        buffer_memory = &m_SceneMemory.buffer_memory;
    }

    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(buffer_memory, bytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, index, name);

    buffer->kind = kind;
    buffer->name = name;

    return index;
}

REV_INTERNAL REV_INLINE u16 GetMaxMipLevels(u16 width, u16 height)
{
    u32 count = 0;
    _BitScanReverse(&count, width & height);
    return cast(u16, count + 1);
}

REV_INTERNAL REV_INLINE u16 GetMaxMipLevels(u16 width, u16 height, u16 depth)
{
    u32 count = 0;
    _BitScanReverse(&count, width & height & depth);
    return cast(u16, count + 1);
}

u64 MemoryManager::AllocateTexture1D(u16 width, DXGI_FORMAT texture_format, const ConstString& name, bool _static)
{
    REV_CHECK_M(1 <= width && width <= D3D12_REQ_TEXTURE1D_U_DIMENSION, "Width gotta be = [1, %hu].", D3D12_REQ_TEXTURE1D_U_DIMENSION);

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = 1;
    desc.Format             = texture_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    u64      index   = REV_U64_MAX;
    Texture *texture = AllocateTexture(_static ? &m_StaticMemory.texture_memory : &m_SceneMemory.texture_memory, desc, index, name);

    texture->kind    = TEXTURE_KIND_1D;
    texture->desc    = desc;
    texture->cube    = false;
    texture->array   = false;
    texture->_static = _static;

    return index;
}

u64 MemoryManager::AllocateTexture2D(u16 width, u16 height, u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        PrintDebugMessage(DEBUG_COLOR::WARNING, "Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = mip_levels;
    desc.Format             = texture_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    u64      index   = REV_U64_MAX;
    Texture *texture = AllocateTexture(_static ? &m_StaticMemory.texture_memory : &m_SceneMemory.texture_memory, desc, index, name);

    texture->kind    = TEXTURE_KIND_2D;
    texture->desc    = desc;
    texture->cube    = false;
    texture->array   = false;
    texture->_static = _static;

    return index;
}

u64 MemoryManager::AllocateTexture3D(u16 width, u16 height, u16 depth, u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    REV_CHECK_M(1 <= depth  && depth  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Depth gotta be = [1, %hu].",  D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height, depth);
    if (mip_levels > max_mip_levels)
    {
        PrintDebugMessage(DEBUG_COLOR::WARNING, "Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = depth;
    desc.MipLevels          = mip_levels;
    desc.Format             = texture_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    u64      index   = REV_U64_MAX;
    Texture *texture = AllocateTexture(_static ? &m_StaticMemory.texture_memory : &m_SceneMemory.texture_memory, desc, index, name);

    texture->kind    = TEXTURE_KIND_3D;
    texture->desc    = desc;
    texture->cube    = false;
    texture->array   = false;
    texture->_static = _static;

    return index;
}

u64 MemoryManager::AllocateTextureCube(u16 width, u16 height, u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        PrintDebugMessage(DEBUG_COLOR::WARNING, "Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = 6;
    desc.MipLevels          = mip_levels;
    desc.Format             = texture_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    u64      index   = REV_U64_MAX;
    Texture *texture = AllocateTexture(_static ? &m_StaticMemory.texture_memory : &m_SceneMemory.texture_memory, desc, index, name);

    texture->kind    = TEXTURE_KIND_CUBE;
    texture->desc    = desc;
    texture->cube    = true;
    texture->array   = false;
    texture->_static = _static;

    return index;
}

u64 MemoryManager::AllocateTexture1DArray(u16 width, u16 count, DXGI_FORMAT texture_format, const ConstString& name, bool _static)
{
    REV_CHECK_M(1 <= width && width <= D3D12_REQ_TEXTURE1D_U_DIMENSION,          "Width gotta be = [1, %hu].", D3D12_REQ_TEXTURE1D_U_DIMENSION);
    REV_CHECK_M(1 <= count && count <= D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].", D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION);

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = 1;
    desc.DepthOrArraySize   = count;
    desc.MipLevels          = 1;
    desc.Format             = texture_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    u64      index   = REV_U64_MAX;
    Texture *texture = AllocateTexture(_static ? &m_StaticMemory.texture_memory : &m_SceneMemory.texture_memory, desc, index, name);

    texture->kind    = TEXTURE_KIND_1D_ARRAY;
    texture->desc    = desc;
    texture->cube    = false;
    texture->array   = true;
    texture->_static = _static;

    return index;
}

u64 MemoryManager::AllocateTexture2DArray(u16 width, u16 height, u16 count, u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,     "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,     "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= count  && count  <= D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].",  D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        PrintDebugMessage(DEBUG_COLOR::WARNING, "Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = count;
    desc.MipLevels          = 1;
    desc.Format             = texture_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    u64      index   = REV_U64_MAX;
    Texture *texture = AllocateTexture(_static ? &m_StaticMemory.texture_memory : &m_SceneMemory.texture_memory, desc, index, name);

    texture->kind    = TEXTURE_KIND_2D_ARRAY;
    texture->desc    = desc;
    texture->cube    = false;
    texture->array   = true;
    texture->_static = _static;

    return index;
}

u64 MemoryManager::AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, DXGI_FORMAT texture_format, const ConstString& name, bool _static)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURECUBE_DIMENSION,            "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURECUBE_DIMENSION,            "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= count  && count  <= D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].",  D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        PrintDebugMessage(DEBUG_COLOR::WARNING, "Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = 6 * count;
    desc.MipLevels          = 1;
    desc.Format             = texture_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    u64      index   = REV_U64_MAX;
    Texture *texture = AllocateTexture(_static ? &m_StaticMemory.texture_memory : &m_SceneMemory.texture_memory, desc, index, name);

    texture->kind    = TEXTURE_KIND_CUBE_ARRAY;
    texture->desc    = desc;
    texture->cube    = true;
    texture->array   = true;
    texture->_static = _static;

    return index;
}

u64 MemoryManager::AllocateSampler(GPU::TEXTURE_ADDRESS_MODE address_mode, Math::v4 border_color, Math::v2 min_max_lod, bool _static)
{
    Settings      *settings       = Settings::Get();
    SamplerMemory *sampler_memory = _static ? &m_StaticMemory.sampler_memory : &m_SceneMemory.sampler_memory;

    u64      sampler_index = sampler_memory->samplers.Count();
    Sampler *sampler       = sampler_memory->samplers.PushBack();

    switch (settings->filtering)
    {
        case FILTERING::POINT:       sampler->desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;        break;
        case FILTERING::BILINEAR:    sampler->desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT; break;
        case FILTERING::TRILINEAR:   sampler->desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;       break;
        case FILTERING::ANISOTROPIC: sampler->desc.Filter = D3D12_FILTER_ANISOTROPIC;              break;
    }
    sampler->desc.AddressU       = cast(D3D12_TEXTURE_ADDRESS_MODE, address_mode);
    sampler->desc.AddressV       = cast(D3D12_TEXTURE_ADDRESS_MODE, address_mode);
    sampler->desc.AddressW       = cast(D3D12_TEXTURE_ADDRESS_MODE, address_mode);
    sampler->desc.MipLODBias     = 0.0f;
    sampler->desc.MaxAnisotropy  = settings->anisotropy;
    sampler->desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // D3D12_COMPARISON_FUNC_ALWAYS
    sampler->desc.BorderColor[0] = border_color.r;
    sampler->desc.BorderColor[1] = border_color.g;
    sampler->desc.BorderColor[2] = border_color.b;
    sampler->desc.BorderColor[3] = border_color.a;
    sampler->desc.MinLOD         = min_max_lod.x;
    sampler->desc.MaxLOD         = min_max_lod.y;

    return sampler_index;
}

void MemoryManager::SetBufferData(const Buffer& buffer, const void *data)
{
    UploadBufferData(m_DeviceContext->CurrentGraphicsList(), buffer, data);
}

void MemoryManager::SetBufferDataImmediately(const Buffer& buffer, const void *data)
{
    UploadBufferData(m_CommandList, buffer, data);
}

void MemoryManager::SetTextureData(Texture *texture, GPU::TextureDesc *texture_desc)
{
    REV_CHECK_M(texture_desc->subtextures_count <= D3D12_REQ_SUBRESOURCES,
                "Too many subresources: %I64u, max expected: %I64u",
                texture_desc->subtextures_count,
                D3D12_REQ_SUBRESOURCES);

    static_assert(sizeof(GPU::SubTextureDesc) == sizeof(D3D12_SUBRESOURCE_DATA));

    UploadTextureData(m_DeviceContext->CurrentGraphicsList(),
                      texture,
                      cast(u32, texture_desc->subtextures_count),
                      cast(D3D12_SUBRESOURCE_DATA *, texture_desc->subtexture_desc));
}

void MemoryManager::SetTextureDataImmediately(Texture *texture, GPU::TextureDesc *texture_desc)
{
    REV_CHECK_M(texture_desc->subtextures_count <= D3D12_REQ_SUBRESOURCES,
                "Too many subresources: %I64u, max expected: %I64u",
                texture_desc->subtextures_count,
                D3D12_REQ_SUBRESOURCES);

    static_assert(sizeof(GPU::SubTextureDesc) == sizeof(D3D12_SUBRESOURCE_DATA));

    UploadTextureData(m_CommandList,
                      texture,
                      cast(u32, texture_desc->subtextures_count),
                      cast(D3D12_SUBRESOURCE_DATA *, texture_desc->subtexture_desc));
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
    HRESULT error = m_CommandList->Close();
    REV_CHECK(CheckResultAndPrintMessages(error));

    ID3D12CommandList *command_lists[] = { m_CommandList };
    m_DeviceContext->GraphicsQueue()->ExecuteCommandLists(cast(u32, ArrayCount(command_lists)), command_lists);

    u64 fence_value = m_Fence->GetCompletedValue() + 1;

    error = m_DeviceContext->GraphicsQueue()->Signal(m_Fence, fence_value);
    REV_CHECK(CheckResultAndPrintMessages(error));

    if (m_Fence->GetCompletedValue() < fence_value)
    {
        error = m_Fence->SetEventOnCompletion(fence_value, m_FenceEvent);
        REV_CHECK(CheckResultAndPrintMessages(error));

        u32 wait_result = WaitForSingleObjectEx(m_FenceEvent, INFINITE, false);
        REV_CHECK(wait_result == WAIT_OBJECT_0);
    }
}

void MemoryManager::FreeSceneMemory()
{
    m_SceneMemory.buffer_memory.buffers.Clear();
    for (BufferMemoryPage& page : m_SceneMemory.buffer_memory.pages)
    {
        SafeRelease(page.def_mem);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(page.upl_mem[i]);
        }
    }
    m_SceneMemory.buffer_memory.pages.Clear();

    for (Texture& texture : m_SceneMemory.texture_memory.textures)
    {
        SafeRelease(texture.def_resource);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(texture.upl_resources[i]);
        }
    }
    m_SceneMemory.texture_memory.textures.Clear();

    m_SceneMemory.sampler_memory.samplers.Clear();
}

void MemoryManager::FreeStaticMemory()
{
    m_StaticMemory.buffer_memory.buffers.Clear();
    for (BufferMemoryPage& page : m_StaticMemory.buffer_memory.pages)
    {
        SafeRelease(page.def_mem);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(page.upl_mem[i]);
        }
    }
    m_StaticMemory.buffer_memory.pages.Clear();

    for (Texture& texture : m_StaticMemory.texture_memory.textures)
    {
        SafeRelease(texture.def_resource);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(texture.upl_resources[i]);
        }
    }
    m_StaticMemory.texture_memory.textures.Clear();

    m_StaticMemory.sampler_memory.samplers.Clear();
}

void MemoryManager::CreateNewPage(BufferMemory *buffer_memory, D3D12_RESOURCE_STATES initial_state)
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

    BufferMemoryPage *page = buffer_memory->pages.PushBack();
    page->initial_state    = initial_state;

    ID3D12Device *device = m_DeviceContext->Device();

    HRESULT error = device->CreateCommittedResource(&default_heap_properties,
                                                    D3D12_HEAP_FLAG_SHARED,
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

        error = page->upl_mem[i]->Map(0, &read_range, cast(void **, page->upl_ptrs + i));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }
}

Buffer *MemoryManager::AllocateBuffer(BufferMemory *buffer_memory, u64 size, D3D12_RESOURCE_STATES initial_state, u64& index, const ConstString& name)
{
    index = buffer_memory->buffers.Count();

    Buffer *buffer       = buffer_memory->buffers.PushBack();
    buffer->page_index   = REV_U64_MAX;
    buffer->offset       = REV_U64_MAX;
    buffer->kind         = BUFFER_KIND_UNKNOWN;
    buffer->vcount       = 0;
    buffer->vstride      = 0;
    buffer->actual_size  = size;
    buffer->aligned_size = AlignUp(size, 256);

    u64 page_index = 0;
    for (BufferMemoryPage& page : buffer_memory->pages)
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
        CreateNewPage(buffer_memory, initial_state);

        buffer->page_index = buffer_memory->pages.Count() - 1;
        buffer->offset     = 0;

        buffer_memory->pages.Last().occupied_bytes = buffer->aligned_size;
    }

    SetBufferName(buffer_memory, buffer, name);

    return buffer;
}

Texture *MemoryManager::AllocateTexture(TextureMemory *texture_memory, const D3D12_RESOURCE_DESC& desc, u64& index, const ConstString& name)
{
    index = texture_memory->textures.Count();

    Texture *texture = texture_memory->textures.PushBack();

    D3D12_HEAP_PROPERTIES default_heap_properties;
    default_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    default_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    default_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    default_heap_properties.CreationNodeMask     = 0;
    default_heap_properties.VisibleNodeMask      = 0;

    HRESULT error = m_DeviceContext->Device()->CreateCommittedResource(&default_heap_properties,
                                                                       D3D12_HEAP_FLAG_SHARED,
                                                                       &desc,
                                                                       D3D12_RESOURCE_STATE_COMMON,
                                                                       null,
                                                                       IID_PPV_ARGS(&texture->def_resource));
    REV_CHECK(CheckResultAndPrintMessages(error));

    u32      wname_length = cast(u32, 4 + name.Length());
    wchar_t *wname        = Memory::Get()->PushToFA<wchar_t>(wname_length + 1);
    CopyMemory(wname, REV_CSTR_ARGS(L"DEF "));
    MultiByteToWideChar(CP_ACP, 0, name.Data(), cast(u32, name.Length()), wname + 4, wname_length - 4);

    error = texture->def_resource->SetName(wname);
    REV_CHECK(CheckResultAndPrintMessages(error));

    return texture;
}

void MemoryManager::UploadBufferData(ID3D12GraphicsCommandList *command_list, const Buffer& buffer, const void *data)
{
    BufferMemory *buffer_memory = buffer.kind & BUFFER_KIND_STATIC ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory;

    BufferMemoryPage *page = buffer_memory->pages.begin() + buffer.page_index;

    ID3D12Resource *upl_mem = page->upl_mem[m_DeviceContext->CurrentBuffer()];
    byte           *upl_ptr = page->upl_ptrs[m_DeviceContext->CurrentBuffer()] + buffer.offset;

    if (data) CopyMemory(upl_ptr, data, buffer.actual_size);
    else      FillMemoryU8(upl_ptr, 0, buffer.actual_size);

    D3D12_RESOURCE_BARRIER resource_barrier;
    resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barrier.Transition.pResource   = page->def_mem;
    resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    resource_barrier.Transition.StateBefore = page->initial_state;
    resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

    command_list->ResourceBarrier(1, &resource_barrier);

    command_list->CopyBufferRegion(page->def_mem, buffer.offset, upl_mem, buffer.offset, buffer.actual_size);

    resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    resource_barrier.Transition.StateAfter  = page->initial_state;

    command_list->ResourceBarrier(1, &resource_barrier);
}

void MemoryManager::UploadTextureData(ID3D12GraphicsCommandList *command_list, Texture *texture, u32 subres_count, D3D12_SUBRESOURCE_DATA *subresources)
{
    ID3D12Device *device = m_DeviceContext->Device();

    ID3D12Device4 *device_4 = null;
    HRESULT        error    = device->QueryInterface(&device_4);
    REV_CHECK(CheckResultAndPrintMessages(error));

    D3D12_RESOURCE_ALLOCATION_INFO1 info1{};
    device_4->GetResourceAllocationInfo1(0, 1, &texture->desc, &info1);

    SafeRelease(device_4);

    byte *push_memory = Memory::Get()->PushToFA<byte>(subres_count * (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(u32) + sizeof(u64) + sizeof(u64)));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints  = cast(D3D12_PLACED_SUBRESOURCE_FOOTPRINT *, push_memory);
    u32                                *num_rows    = cast(u32 *, footprints + subres_count);
    u64                                *row_bytes   = cast(u64 *, num_rows   + subres_count);
    u64                                *total_bytes = cast(u64 *, row_bytes  + subres_count);
    device->GetCopyableFootprints(&texture->desc, 0, subres_count, info1.Offset, footprints, cast(UINT *, num_rows), row_bytes, total_bytes);

    ID3D12Resource **upload_resource = texture->upl_resources + m_DeviceContext->CurrentBuffer();
    byte           **upload_pointer  = texture->upl_pointers  + m_DeviceContext->CurrentBuffer();

    if (!*upload_resource)
    {
        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                  = D3D12_HEAP_TYPE_UPLOAD;
        heap_properties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask      = 0;
        heap_properties.VisibleNodeMask       = 0;

        D3D12_RESOURCE_DESC resource_desc;
        resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Width              = *total_bytes;
        resource_desc.Height             = 1;
        resource_desc.DepthOrArraySize   = 1;
        resource_desc.MipLevels          = 1;
        resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count   = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &resource_desc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        null,
                                                        IID_PPV_ARGS(upload_resource));
        REV_CHECK(CheckResultAndPrintMessages(error));

        // @NOTE(Roman): SetName stuff.
        {
            UINT def_length = 0;
            error = texture->def_resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &def_length, null);
            REV_CHECK(CheckResultAndPrintMessages(error));

            def_length = def_length / sizeof(wchar_t) - 1;

            u32      wname_length = cast(u32, 7 + def_length);
            wchar_t *wname        = Memory::Get()->PushToFA<wchar_t>(wname_length + 1);

            UINT data_size = (def_length + 1) * sizeof(wchar_t);
            error = texture->def_resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, wname + 3);
            REV_CHECK(CheckResultAndPrintMessages(error));
            REV_CHECK(def_length == data_size / sizeof(wchar_t) - 1);

            _snwprintf(wname, 7, L"UPL #%I32u ", m_DeviceContext->CurrentBuffer());

            error = (*upload_resource)->SetName(wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }

        D3D12_RANGE read_range = {0, 0};

        error = (*upload_resource)->Map(0, &read_range, cast(void **, upload_pointer));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    for (u32 i = 0; i < subres_count; ++i)
    {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprint = footprints + i;

        u32 rows_in_dest_subres      = num_rows[i];
        u64 row_bytes_in_dest_subres = row_bytes[i];

        byte *dest_subres_data        = (*upload_pointer) + footprint->Offset;
        u64   dest_subres_slice_pitch = footprint->Footprint.RowPitch * rows_in_dest_subres;

        D3D12_SUBRESOURCE_DATA *subres_data = subresources + i;

        for (u32 z = 0; z < footprint->Footprint.Depth; ++z)
        {
            byte *dest_slice =              dest_subres_data    + dest_subres_slice_pitch * z;
            byte *src_slice  = cast(byte *, subres_data->pData) + subres_data->SlicePitch * z;

            for (u32 y = 0; y < rows_in_dest_subres; ++y)
            {
                CopyMemory(dest_slice + footprint->Footprint.RowPitch * y,
                           src_slice  + subres_data->RowPitch         * y,
                           row_bytes_in_dest_subres);
            }
        }
    }

    D3D12_TEXTURE_COPY_LOCATION dest_location;
    dest_location.pResource = texture->def_resource;
    dest_location.Type      = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    D3D12_TEXTURE_COPY_LOCATION source_location;
    source_location.pResource = *upload_resource;
    source_location.Type      = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    for (u32 i = 0; i < subres_count; ++i)
    {
        dest_location.SubresourceIndex  = i;
        source_location.PlacedFootprint = footprints[i];

        command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, null);
    }
}

void MemoryManager::SetBufferName(BufferMemory *buffer_memory, Buffer *buffer, const ConstString& name)
{
    HRESULT           error = S_OK;
    BufferMemoryPage *page  = buffer_memory->pages.GetPointer(buffer->page_index);

    wchar_t *wname = Memory::Get()->PushToFA<wchar_t>(name.Length() + 1);
    MultiByteToWideChar(CP_ACP, 0, name.Data(), cast(u32, name.Length()), wname, cast(u32, name.Length()));

    UINT gotten_default_wname_length = 0;
    error = page->def_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &gotten_default_wname_length, null);

    if (gotten_default_wname_length)
    {
        gotten_default_wname_length = gotten_default_wname_length / sizeof(wchar_t) - 1;

        u32      new_default_wname_length = cast(u32, gotten_default_wname_length + 1 + name.Length());
        wchar_t *new_default_wname        = Memory::Get()->PushToFA<wchar_t>(new_default_wname_length + 1);

        UINT default_data_size = (new_default_wname_length + 1) * sizeof(wchar_t);
        error = page->def_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &default_data_size, new_default_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
        REV_CHECK(gotten_default_wname_length == default_data_size / sizeof(wchar_t) - 1);

        _snwprintf(new_default_wname + gotten_default_wname_length, new_default_wname_length - gotten_default_wname_length, L" %.*s", cast(u32, name.Length()), wname);

        error = page->def_mem->SetName(new_default_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }
    else
    {
        u32      new_default_wname_length = cast(u32, REV_CSTRLEN("DEF ") + name.Length());
        wchar_t *new_default_wname        = Memory::Get()->PushToFA<wchar_t>(new_default_wname_length + 1);

        _snwprintf(new_default_wname, new_default_wname_length, L"DEF %.*s", cast(u32, name.Length()), wname);

        error = page->def_mem->SetName(new_default_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12Resource *upl_mem = page->upl_mem[i];

        UINT gotten_upload_wname_length = 0;
        error = upl_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &gotten_upload_wname_length, null);

        if (gotten_upload_wname_length)
        {
            gotten_upload_wname_length = gotten_upload_wname_length / sizeof(wchar_t) - 1;

            u32      new_upload_wname_length = cast(u32, gotten_upload_wname_length + 1 + name.Length());
            wchar_t *new_upload_wname        = Memory::Get()->PushToFA<wchar_t>(new_upload_wname_length + 1);

            UINT upload_data_size = (new_upload_wname_length + 1) * sizeof(wchar_t);
            error = upl_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &upload_data_size, new_upload_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
            REV_CHECK(gotten_upload_wname_length == upload_data_size / sizeof(wchar_t) - 1);

            _snwprintf(new_upload_wname + gotten_upload_wname_length, new_upload_wname_length - gotten_upload_wname_length, L" %.*s", cast(u32, name.Length()), wname);

            error = upl_mem->SetName(new_upload_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
        else
        {
            u32      new_upload_wname_length = cast(u32, 7 + name.Length());
            wchar_t *new_upload_wname        = Memory::Get()->PushToFA<wchar_t>(new_upload_wname_length + 1);

            _snwprintf(new_upload_wname, new_upload_wname_length, L"UPL #%I32u %.*s", i, cast(u32, name.Length()), wname);

            error = upl_mem->SetName(new_upload_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }
}

};
