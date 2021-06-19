//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "platform/d3d12/d3d12_memory_manager.h"
#include "graphics/graphics_api.h"
#include "core/memory.h"
#include "platform/d3d12/d3d12_common.h"
#include "core/scene.h"

namespace REV::D3D12
{

MemoryManager::MemoryManager(Allocator *allocator)
    : m_CommandAllocator(null),
      m_CommandList(null),
      m_Fence(null),
      m_FenceEvent("D3D12::MemoryManager Event"),
      m_DescHeapMemory(allocator),
      m_SamplerMemory(allocator),
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

u64 MemoryManager::AllocateVertexBuffer(u32 vertex_count, const StaticString<64>& name)
{
    REV_CHECK(vertex_count);

    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(vertex_count * sizeof(Vertex), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, index);

    buffer->kind    = BUFFER_KIND::VERTEX_BUFFER;
    buffer->vcount  = vertex_count;
    buffer->vstride = sizeof(Vertex);
    buffer->name    = name;

    return index;
}

u64 MemoryManager::AllocateIndexBuffer(u32 index_count, const StaticString<64>& name)
{
    REV_CHECK(index_count);

    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(index_count * sizeof(Index), D3D12_RESOURCE_STATE_INDEX_BUFFER, index);

    buffer->kind    = BUFFER_KIND::INDEX_BUFFER;
    buffer->icount  = index_count;
    buffer->istride = sizeof(Index);
    buffer->name    = name;

    return index;
}

u64 MemoryManager::AllocateConstantBuffer(u32 bytes, const StaticString<64>& name)
{
    REV_CHECK(bytes);

    u64     index  = REV_U64_MAX;
    Buffer *buffer = AllocateBuffer(bytes, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, index);

    buffer->kind = BUFFER_KIND::CONSTANT_BUFFER;
    buffer->name = name;

    DescHeap *desc_heap = CreateDescHeapForResource(index, buffer->desc_heap_index);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
    cbv_desc.BufferLocation = m_BufferMemory.pages[buffer->page_index].def_mem->GetGPUVirtualAddress() + buffer->offset;
    cbv_desc.SizeInBytes    = cast<u32>(buffer->aligned_size);

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateConstantBufferView(&cbv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

REV_INTERNAL u16 GetMaxMipLevels(u16 width, u16 height)
{
    u32 count = 0;
    _BitScanReverse(&count, width & height);
    return cast<u16>(count + 1);
}

REV_INTERNAL u16 GetMaxMipLevels(u16 width, u16 height, u16 depth)
{
    u32 count = 0;
    _BitScanReverse(&count, width & height & depth);
    return cast<u16>(count + 1);
}

u64 MemoryManager::AllocateTexture1D(u16 width, DXGI_FORMAT texture_format, const StaticString<64>& name)
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
    Texture *texture = AllocateTexture(desc, index);

    texture->name  = name;
    texture->desc  = desc;
    texture->cube  = false;
    texture->array = false;

    DescHeap *desc_heap = CreateDescHeapForResource(index, texture->desc_heap_index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                        = texture_format;
    srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE1D;
    srv_desc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture1D.MostDetailedMip     = 0;    // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
    srv_desc.Texture1D.MipLevels           = 1;    // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
    srv_desc.Texture1D.ResourceMinLODClamp = 0.5f; // scale bound

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateShaderResourceView(texture->def_resource, &srv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

u64 MemoryManager::AllocateTexture2D(u16 width, u16 height, u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        DebugFC(DEBUG_COLOR::WARNING, "Warning: Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
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
    Texture *texture = AllocateTexture(desc, index);

    texture->name  = name;
    texture->desc  = desc;
    texture->cube  = false;
    texture->array = false;

    DescHeap *desc_heap = CreateDescHeapForResource(index, texture->desc_heap_index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                        = texture_format;
    srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2D.MostDetailedMip     = 0;          // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
    srv_desc.Texture2D.MipLevels           = mip_levels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
    srv_desc.Texture2D.PlaneSlice          = 0;          // The index (plane slice number) of the plane to use in the texture.
    srv_desc.Texture2D.ResourceMinLODClamp = 0.5f;       // scale bound

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateShaderResourceView(texture->def_resource, &srv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

u64 MemoryManager::AllocateTexture3D(u16 width, u16 height, u16 depth, u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    REV_CHECK_M(1 <= depth  && depth  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Depth gotta be = [1, %hu].",  D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height, depth);
    if (mip_levels > max_mip_levels)
    {
        DebugFC(DEBUG_COLOR::WARNING, "Warning: Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
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
    Texture *texture = AllocateTexture(desc, index);

    texture->name  = name;
    texture->desc  = desc;
    texture->cube  = false;
    texture->array = false;

    DescHeap *desc_heap = CreateDescHeapForResource(index, texture->desc_heap_index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                        = texture_format;
    srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
    srv_desc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture3D.MostDetailedMip     = 0;          // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
    srv_desc.Texture3D.MipLevels           = mip_levels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
    srv_desc.Texture3D.ResourceMinLODClamp = 0.5f;       // scale bound

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateShaderResourceView(texture->def_resource, &srv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

u64 MemoryManager::AllocateTextureCube(u16 width, u16 height, u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        DebugFC(DEBUG_COLOR::WARNING, "Warning: Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
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
    Texture *texture = AllocateTexture(desc, index);

    texture->name  = name;
    texture->desc  = desc;
    texture->cube  = true;
    texture->array = false;

    DescHeap *desc_heap = CreateDescHeapForResource(index, texture->desc_heap_index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                          = texture_format;
    srv_desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.TextureCube.MostDetailedMip     = 0;          // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
    srv_desc.TextureCube.MipLevels           = mip_levels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
    srv_desc.TextureCube.ResourceMinLODClamp = 0.5f;       // scale bound

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateShaderResourceView(texture->def_resource, &srv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

u64 MemoryManager::AllocateTexture1DArray(u16 width, u16 count, DXGI_FORMAT texture_format, const StaticString<64>& name)
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
    Texture *texture = AllocateTexture(desc, index);

    texture->name  = name;
    texture->desc  = desc;
    texture->cube  = false;
    texture->array = true;

    DescHeap *desc_heap = CreateDescHeapForResource(index, texture->desc_heap_index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                             = texture_format;
    srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
    srv_desc.Shader4ComponentMapping            = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture1DArray.MostDetailedMip     = 0;     // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
    srv_desc.Texture1DArray.MipLevels           = 1;     // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
    srv_desc.Texture1DArray.FirstArraySlice     = 0;     // First texture in an array to access [0, desc.DepthOrArraySize)
    srv_desc.Texture1DArray.ArraySize           = count; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
    srv_desc.Texture1DArray.ResourceMinLODClamp = 0.5f;  // scale bound

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateShaderResourceView(texture->def_resource, &srv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

u64 MemoryManager::AllocateTexture2DArray(u16 width, u16 height, u16 count, u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,     "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,     "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= count  && count  <= D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].",  D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        DebugFC(DEBUG_COLOR::WARNING, "Warning: Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
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
    Texture *texture = AllocateTexture(desc, index);

    texture->name  = name;
    texture->desc  = desc;
    texture->cube  = false;
    texture->array = true;

    DescHeap *desc_heap = CreateDescHeapForResource(index, texture->desc_heap_index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                             = texture_format;
    srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srv_desc.Shader4ComponentMapping            = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Texture2DArray.MostDetailedMip     = 0;          // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
    srv_desc.Texture2DArray.MipLevels           = mip_levels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
    srv_desc.Texture2DArray.FirstArraySlice     = 0;          // First texture in an array to access [0, desc.DepthOrArraySize)
    srv_desc.Texture2DArray.ArraySize           = count;      // Numbre of textrues in array. [1, desc.DepthOrArraySize]
    srv_desc.Texture2DArray.PlaneSlice          = 0;          // The index (plane slice number) of the plane to use in the texture.
    srv_desc.Texture2DArray.ResourceMinLODClamp = 0.5f;       // scale bound

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateShaderResourceView(texture->def_resource, &srv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

u64 MemoryManager::AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, DXGI_FORMAT texture_format, const StaticString<64>& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURECUBE_DIMENSION,            "Width  gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURECUBE_DIMENSION,            "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= count  && count  <= D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].",  D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        DebugFC(DEBUG_COLOR::WARNING, "Warning: Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
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
    Texture *texture = AllocateTexture(desc, index);

    texture->name  = name;
    texture->desc  = desc;
    texture->cube  = true;
    texture->array = true;

    DescHeap *desc_heap = CreateDescHeapForResource(index, texture->desc_heap_index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Format                               = texture_format;
    srv_desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
    srv_desc.Shader4ComponentMapping              = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.TextureCubeArray.MostDetailedMip     = 0;          // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
    srv_desc.TextureCubeArray.MipLevels           = mip_levels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
    srv_desc.TextureCubeArray.First2DArrayFace    = 0;          // First texture in an array to access [0, desc.DepthOrArraySize)
    srv_desc.TextureCubeArray.NumCubes            = count;      // Numbre of textrues in array. [1, desc.DepthOrArraySize]
    srv_desc.TextureCubeArray.ResourceMinLODClamp = 0.5f;       // scale bound

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateShaderResourceView(texture->def_resource, &srv_desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return index;
}

u64 MemoryManager::AllocateSampler(const D3D12_SAMPLER_DESC& sampler_desc)
{
    u64      sampler_index = m_SamplerMemory.samplers.Count();
    Sampler *sampler       = m_SamplerMemory.samplers.PushBack();

    sampler->desc = sampler_desc;

    DescHeap *desc_heap = CreateDescHeapForSampler(sampler_index, sampler->desc_heap_index);

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();
    device->CreateSampler(&sampler->desc, desc_heap->handle->GetCPUDescriptorHandleForHeapStart());

    return sampler_index;
}

void MemoryManager::SetBufferData(const Buffer& buffer, const void *data)
{
    UploadBufferData(cast<Renderer *>(GraphicsAPI::GetRenderer())->CurrentGraphicsList(), buffer, data);
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

    UploadTextureData(cast<Renderer *>(GraphicsAPI::GetRenderer())->CurrentGraphicsList(),
                      texture,
                      cast<u32>(texture_desc->subtextures_count),
                      cast<D3D12_SUBRESOURCE_DATA *>(texture_desc->subtexture_desc));
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
                      cast<u32>(texture_desc->subtextures_count),
                      cast<D3D12_SUBRESOURCE_DATA *>(texture_desc->subtexture_desc));
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
        SafeRelease(texture.def_resource);
    }
    m_TextureMemory.textures.Clear();

    for (DescHeap& desc_heap : m_DescHeapMemory.desc_heaps)
    {
        SafeRelease(desc_heap.handle);
    }
    m_DescHeapMemory.desc_heaps.Clear();
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

Texture *MemoryManager::AllocateTexture(const D3D12_RESOURCE_DESC& desc, u64& index)
{
    index = m_TextureMemory.textures.Count();

    Texture *texture = m_TextureMemory.textures.PushBack();

    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();

    D3D12_HEAP_PROPERTIES default_heap_properties;
    default_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    default_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    default_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    default_heap_properties.CreationNodeMask     = 0;
    default_heap_properties.VisibleNodeMask      = 0;

    HRESULT error = device->CreateCommittedResource(&default_heap_properties,
                                                    D3D12_HEAP_FLAG_SHARED,
                                                    &desc,
                                                    D3D12_RESOURCE_STATE_COMMON,
                                                    null,
                                                    IID_PPV_ARGS(&texture->def_resource));
    REV_CHECK(CheckResultAndPrintMessages(error));

    return texture;
}

DescHeap *MemoryManager::CreateDescHeapForResource(u64 resource_index, u64& desc_heap_index)
{
    desc_heap_index = m_DescHeapMemory.desc_heaps.Count();

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

DescHeap *MemoryManager::CreateDescHeapForSampler(u64 sampler_index, u64& desc_heap_index)
{
    desc_heap_index = m_DescHeapMemory.desc_heaps.Count();

    D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc;
    desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    desc_heap_desc.NumDescriptors = 1;
    desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc_heap_desc.NodeMask       = 0;

    DescHeap *desc_heap      = m_DescHeapMemory.desc_heaps.PushBack();
    desc_heap->handle        = null;
    desc_heap->sampler_index = sampler_index;
    desc_heap->desc          = desc_heap_desc;

    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    HRESULT error = renderer->Device()->CreateDescriptorHeap(&desc_heap->desc, IID_PPV_ARGS(&desc_heap->handle));
    REV_CHECK(CheckResultAndPrintMessages(error));

    return desc_heap;
}

void MemoryManager::UploadBufferData(ID3D12GraphicsCommandList *command_list, const Buffer& buffer, const void *data)
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

REV_INTERNAL ID3D12Resource *AllocateUploadResourceForTexture(u64 bytes)
{
    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();

    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type                  = D3D12_HEAP_TYPE_UPLOAD;
    heap_properties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask      = 0;
    heap_properties.VisibleNodeMask       = 0;

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

    ID3D12Resource *resource = null;
    HRESULT         error    = device->CreateCommittedResource(&heap_properties,
                                                               D3D12_HEAP_FLAG_NONE,
                                                               &resource_desc,
                                                               D3D12_RESOURCE_STATE_GENERIC_READ,
                                                               null,
                                                               IID_PPV_ARGS(&resource));
    REV_CHECK(CheckResultAndPrintMessages(error));

    return resource;
}

void MemoryManager::UploadTextureData(ID3D12GraphicsCommandList *command_list, Texture *texture, u32 subres_count, D3D12_SUBRESOURCE_DATA *subresources)
{
    ID3D12Device *device = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device();

    ID3D12Device4 *device_4 = null;
    HRESULT        error    = device->QueryInterface(&device_4);
    REV_CHECK(CheckResultAndPrintMessages(error));

    D3D12_RESOURCE_ALLOCATION_INFO1 info1{};
    D3D12_RESOURCE_ALLOCATION_INFO  info = device_4->GetResourceAllocationInfo1(0, 1, &texture->desc, &info1);

    SafeRelease(device_4);

    byte *push_memory = Memory::Get()->PushToTA<byte>(subres_count * (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(u32) + sizeof(u64) + sizeof(u64)));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints  = cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT *>(push_memory);
    u32                                *num_rows    = cast<u32 *>(footprints + subres_count);
    u64                                *row_bytes   = cast<u64 *>(num_rows   + subres_count);
    u64                                *total_bytes = cast<u64 *>(row_bytes  + subres_count);
    device->GetCopyableFootprints(&texture->desc, 0, subres_count, info1.Offset, footprints, cast<UINT *>(num_rows), row_bytes, total_bytes);

    if (!texture->upl_resource) texture->upl_resource = AllocateUploadResourceForTexture(*total_bytes);

    D3D12_RANGE  read_range     = {0, 0};
    byte        *upload_pointer = null;
    error = texture->upl_resource->Map(0, &read_range, cast<void **>(&upload_pointer));
    REV_CHECK(CheckResultAndPrintMessages(error));

    for (u32 i = 0; i < subres_count; ++i)
    {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprint = footprints + i;

        u32 rows_in_dest_subres      = num_rows[i];
        u64 row_bytes_in_dest_subres = row_bytes[i];

        byte *dest_subres_data        = upload_pointer + footprint->Offset;
        u64   dest_subres_slice_pitch = footprint->Footprint.RowPitch * rows_in_dest_subres;

        D3D12_SUBRESOURCE_DATA *subres_data = subresources + i;

        for (u32 z = 0; z < footprint->Footprint.Depth; ++z)
        {
            byte *dest_slice =              dest_subres_data    + dest_subres_slice_pitch * z;
            byte *src_slice  = cast<byte *>(subres_data->pData) + subres_data->SlicePitch * z;

            for (u32 y = 0; y < rows_in_dest_subres; ++y)
            {
                CopyMemory(dest_slice + footprint->Footprint.RowPitch * y,
                           src_slice  + subres_data->RowPitch         * y,
                           row_bytes_in_dest_subres);
            }
        }
    }

    texture->upl_resource->Unmap(0, null);

    D3D12_TEXTURE_COPY_LOCATION dest_location;
    dest_location.pResource        = texture->def_resource;
    dest_location.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    D3D12_TEXTURE_COPY_LOCATION source_location;
    source_location.pResource       = texture->upl_resource;
    source_location.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    for (u32 i = 0; i < subres_count; ++i)
    {
        dest_location.SubresourceIndex  = i;
        source_location.PlacedFootprint = footprints[i];

        command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, null);
    }
}

};
