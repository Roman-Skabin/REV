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
    : m_Allocator(allocator),
      m_DeviceContext(cast(DeviceContext *, GraphicsAPI::GetDeviceContext())),
      m_StaticMemory(allocator),
      m_SceneMemory(allocator)
{
}

MemoryManager::~MemoryManager()
{
    FreeMemory(&m_SceneMemory);
    FreeMemory(&m_StaticMemory);
}

REV_INTERNAL REV_INLINE DXGI_FORMAT REVToDXGIBufferFormat(GPU::BUFFER_FORMAT format, u32& size)
{
    switch (format)
    {
        case GPU::BUFFER_FORMAT_UNKNOWN: size =  0; return DXGI_FORMAT_UNKNOWN;
        case GPU::BUFFER_FORMAT_BOOL:    size =  1; return DXGI_FORMAT_R8_UINT;
        case GPU::BUFFER_FORMAT_BOOLx2:  size =  2; return DXGI_FORMAT_R8G8_UINT;
//      case GPU::BUFFER_FORMAT_BOOLx3:  size =  3; return DXGI_FORMAT_R8G8B8_UINT;
        case GPU::BUFFER_FORMAT_BOOLx4:  size =  4; return DXGI_FORMAT_R8G8B8A8_UINT;
        case GPU::BUFFER_FORMAT_F16:     size =  2; return DXGI_FORMAT_R16_FLOAT;
        case GPU::BUFFER_FORMAT_F16x2:   size =  4; return DXGI_FORMAT_R16G16_FLOAT;
//      case GPU::BUFFER_FORMAT_F16x3:   size =  6; return DXGI_FORMAT_R16G16B16_FLOAT;
        case GPU::BUFFER_FORMAT_F16x4:   size =  8; return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case GPU::BUFFER_FORMAT_F32:     size =  4; return DXGI_FORMAT_R32_FLOAT;
        case GPU::BUFFER_FORMAT_F32x2:   size =  8; return DXGI_FORMAT_R32G32_FLOAT;
        case GPU::BUFFER_FORMAT_F32x3:   size = 12; return DXGI_FORMAT_R32G32B32_FLOAT;
        case GPU::BUFFER_FORMAT_F32x4:   size = 16; return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case GPU::BUFFER_FORMAT_U32:     size =  4; return DXGI_FORMAT_R32_UINT;
        case GPU::BUFFER_FORMAT_U32x2:   size =  8; return DXGI_FORMAT_R32G32_UINT;
        case GPU::BUFFER_FORMAT_U32x3:   size = 12; return DXGI_FORMAT_R32G32B32_UINT;
        case GPU::BUFFER_FORMAT_U32x4:   size = 16; return DXGI_FORMAT_R32G32B32A32_UINT;
        case GPU::BUFFER_FORMAT_S32:     size =  4; return DXGI_FORMAT_R32_SINT;
        case GPU::BUFFER_FORMAT_S32x2:   size =  8; return DXGI_FORMAT_R32G32_SINT;
        case GPU::BUFFER_FORMAT_S32x3:   size = 12; return DXGI_FORMAT_R32G32B32_SINT;
        case GPU::BUFFER_FORMAT_S32x4:   size = 16; return DXGI_FORMAT_R32G32B32A32_SINT;
    }

    REV_ERROR_M("Invalid GPU::BUFFER_FORMAT: %I32u", format);
    return DXGI_FORMAT_UNKNOWN;
}

u64 MemoryManager::AllocateVertexBuffer(u32 count, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK(count);

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(count * sizeof(Vertex),
                                    DXGI_FORMAT_UNKNOWN,
                                    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                    flags,
                                    index,
                                    name);

    buffer->stride = sizeof(Vertex);

    return index;
}

u64 MemoryManager::AllocateIndexBuffer(u32 count, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK(count);

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(count * sizeof(Index),
                                    DXGI_FORMAT_UNKNOWN,
                                    D3D12_RESOURCE_STATE_INDEX_BUFFER,
                                    flags,
                                    index,
                                    name);

    buffer->format = DXGI_FORMAT_R32_UINT;
    buffer->stride = sizeof(Index);

    return index;
}

u64 MemoryManager::AllocateConstantBuffer(u32 bytes, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK(bytes);

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(bytes,
                                    DXGI_FORMAT_UNKNOWN,
                                    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                    flags,
                                    index,
                                    name);

    return index;
}

u64 MemoryManager::AllocateStructuredBuffer(u32 count, u32 stride, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK(count);
    REV_CHECK(stride);

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                                | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(count * stride, DXGI_FORMAT_UNKNOWN, state, flags, index, name);

    buffer->ro_rw_type = RO_RW_BUFFER_TYPE_STRUCTURED;
    buffer->stride     = stride;

    return index;
}

u64 MemoryManager::AllocateByteAddressBuffer(u32 bytes, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK(bytes);

    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                                | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(bytes, DXGI_FORMAT_UNKNOWN, state, flags, index, name);

    buffer->ro_rw_type = RO_RW_BUFFER_TYPE_BYTE_ADDRESS;
    buffer->stride     = 1;

    return index;
}

u64 MemoryManager::AllocateBuffer(u32 bytes, GPU::BUFFER_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK(bytes);

    RO_RW_BUFFER_TYPE     ro_rw_type = RO_RW_BUFFER_TYPE_NONE;
    D3D12_RESOURCE_STATES state      = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                                     | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    if (flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
    {
        state = D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    else if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        ro_rw_type = RO_RW_BUFFER_TYPE_BUFFER;
        state      = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    u32         stride      = 0;
    DXGI_FORMAT dxgi_format = REVToDXGIBufferFormat(format, stride);

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(bytes, dxgi_format, state, flags, index, name);

    buffer->ro_rw_type = ro_rw_type;
    buffer->format     = dxgi_format;
    buffer->stride     = stride;

    return index;
}

REV_INTERNAL REV_INLINE u16 GetMaxMipLevels(u16 width, u16 height = 0, u16 depth = 0)
{
    u32 count = 0;
    _BitScanReverse(&count, width | height | depth);
    return cast(u16, count + 1);
}

u64 MemoryManager::AllocateTexture1D(u16 width, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK_M(1 <= width && width <= D3D12_REQ_TEXTURE1D_U_DIMENSION, "Width gotta be = [1, %hu].", D3D12_REQ_TEXTURE1D_U_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width);
    if (mip_levels > max_mip_levels)
    {
        REV_WARNING_M("Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
        if (mip_levels > D3D12_REQ_MIP_LEVELS)
        {
            REV_WARNING_M("DirectX 12 does not support that many MIP levels. Only %I32u of them will be used", D3D12_REQ_MIP_LEVELS);
            mip_levels = D3D12_REQ_MIP_LEVELS;
        }
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = 1;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    if (flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    else if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(desc.Format);
    REV_CHECK_M(planes_count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    u64      index   = REV_INVALID_U64_INDEX;
    Texture *texture = AllocateTexture(TEXTURE_DIMENSION_1D, desc, flags, planes_count, index, name);

    return index;
}

u64 MemoryManager::AllocateTexture2D(u16 width, u16 height, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Width gotta be = [1, %hu].",  D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        REV_WARNING_M("Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
        if (mip_levels > D3D12_REQ_MIP_LEVELS)
        {
            REV_WARNING_M("DirectX 12 does not support that many MIP levels. Only %I32u of them will be used", D3D12_REQ_MIP_LEVELS);
            mip_levels = D3D12_REQ_MIP_LEVELS;
        }
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = 1;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    if (flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    else if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(desc.Format);
    REV_CHECK_M(planes_count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    u64      index   = REV_INVALID_U64_INDEX;
    Texture *texture = AllocateTexture(TEXTURE_DIMENSION_2D, desc, flags, planes_count, index, name);

    return index;
}

u64 MemoryManager::AllocateTexture3D(u16 width, u16 height, u16 depth, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Width gotta be = [1, %hu].",  D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
    REV_CHECK_M(1 <= depth  && depth  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Depth gotta be = [1, %hu].",  D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height, depth);
    if (mip_levels > max_mip_levels)
    {
        REV_WARNING_M("Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
        if (mip_levels > D3D12_REQ_MIP_LEVELS)
        {
            REV_WARNING_M("DirectX 12 does not support that many MIP levels. Only %I32u of them will be used", D3D12_REQ_MIP_LEVELS);
            mip_levels = D3D12_REQ_MIP_LEVELS;
        }
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = depth;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    if (flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    else if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(desc.Format);
    REV_CHECK_M(planes_count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    u64      index   = REV_INVALID_U64_INDEX;
    Texture *texture = AllocateTexture(TEXTURE_DIMENSION_3D, desc, flags, planes_count, index, name);

    texture->depth = depth;

    return index;
}

u64 MemoryManager::AllocateTextureCube(u16 width, u16 height, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Width gotta be = [1, %hu].",  D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        REV_WARNING_M("Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
        if (mip_levels > D3D12_REQ_MIP_LEVELS)
        {
            REV_WARNING_M("DirectX 12 does not support that many MIP levels. Only %I32u of them will be used", D3D12_REQ_MIP_LEVELS);
            mip_levels = D3D12_REQ_MIP_LEVELS;
        }
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = 6;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(desc.Format);
    REV_CHECK_M(planes_count * 6 * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    u64      index   = REV_INVALID_U64_INDEX;
    Texture *texture = AllocateTexture(TEXTURE_DIMENSION_CUBE, desc, flags, planes_count, index, name);

    return index;
}

u64 MemoryManager::AllocateTexture1DArray(u16 width, u16 count, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK_M(1 <= width && width <= D3D12_REQ_TEXTURE1D_U_DIMENSION,          "Width gotta be = [1, %hu].", D3D12_REQ_TEXTURE1D_U_DIMENSION);
    REV_CHECK_M(1 <= count && count <= D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].", D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width);
    if (mip_levels > max_mip_levels)
    {
        REV_WARNING_M("Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
        if (mip_levels > D3D12_REQ_MIP_LEVELS)
        {
            REV_WARNING_M("DirectX 12 does not support that many MIP levels. Only %I32u of them will be used", D3D12_REQ_MIP_LEVELS);
            mip_levels = D3D12_REQ_MIP_LEVELS;
        }
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = 1;
    desc.DepthOrArraySize   = count;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    if (flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    else if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(desc.Format);
    REV_CHECK_M(planes_count * count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    u64      index   = REV_INVALID_U64_INDEX;
    Texture *texture = AllocateTexture(TEXTURE_DIMENSION_1D_ARRAY, desc, flags, planes_count, index, name);

    texture->subtextures_count = count;

    return index;
}

u64 MemoryManager::AllocateTexture2DArray(u16 width, u16 height, u16 count, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,     "Width gotta be = [1, %hu].",  D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,     "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    REV_CHECK_M(1 <= count  && count  <= D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].",  D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        REV_WARNING_M("Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
        if (mip_levels > D3D12_REQ_MIP_LEVELS)
        {
            REV_WARNING_M("DirectX 12 does not support that many MIP levels. Only %I32u of them will be used", D3D12_REQ_MIP_LEVELS);
            mip_levels = D3D12_REQ_MIP_LEVELS;
        }
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = count;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    if (flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    else if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(desc.Format);
    REV_CHECK_M(planes_count * count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    u64      index   = REV_INVALID_U64_INDEX;
    Texture *texture = AllocateTexture(TEXTURE_DIMENSION_2D_ARRAY, desc, flags, planes_count, index, name);

    texture->subtextures_count = count;

    return index;
}

u64 MemoryManager::AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, GPU::TEXTURE_FORMAT format, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURECUBE_DIMENSION,            "Width gotta be = [1, %hu].",  D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= height && height <= D3D12_REQ_TEXTURECUBE_DIMENSION,            "Height gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);
    REV_CHECK_M(1 <= count  && count  <= D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].",  D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION);

    u16 max_mip_levels = GetMaxMipLevels(width, height);
    if (mip_levels > max_mip_levels)
    {
        REV_WARNING_M("Number of MIP levels is to high: %hu. It has been changed to %hu.", mip_levels, max_mip_levels);
        mip_levels = max_mip_levels;
        if (mip_levels > D3D12_REQ_MIP_LEVELS)
        {
            REV_WARNING_M("DirectX 12 does not support that many MIP levels. Only %I32u of them will be used", D3D12_REQ_MIP_LEVELS);
            mip_levels = D3D12_REQ_MIP_LEVELS;
        }
    }

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = 6 * count;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(desc.Format);
    REV_CHECK_M(planes_count * count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    u64      index   = REV_INVALID_U64_INDEX;
    Texture *texture = AllocateTexture(TEXTURE_DIMENSION_CUBE_ARRAY, desc, flags, planes_count, index, name);

    texture->subtextures_count = count;

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

void MemoryManager::LoadResources(const ConstArray<GPU::ResourceHandle>& resources)
{
    UploadResources(m_DeviceContext->CurrentGraphicsList(), resources);
}

void MemoryManager::StoreResources(const ConstArray<GPU::ResourceHandle>& resources)
{
    ReadbackResources(m_DeviceContext->CurrentGraphicsList(), resources);
}

void MemoryManager::ResizeRenderTarget(GPU::ResourceHandle resource, u16 width, u16 height, u16 depth)
{
    REV_CHECK_M(resource.kind == GPU::RESOURCE_KIND_TEXTURE, "We do not allow to resize render target buffers for now");

    Texture *texture = resource.flags & GPU::RESOURCE_FLAG_STATIC
                     ? m_StaticMemory.texture_memory.textures.GetPointer(resource.index)
                     : m_SceneMemory.texture_memory.textures.GetPointer(resource.index);
    REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_RENDER_TARGET, "Texture \"%.*s\" is not a render target", texture->name.Length(), texture->name.Data());

    ID3D12Device *device = m_DeviceContext->Device();

    D3D12_RESOURCE_DESC default_desc = texture->default_gpu_mem->GetDesc();
    {
        switch (default_desc.Dimension)
        {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            {
                default_desc.Width = width;
            } break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            {
                default_desc.Width  = width;
                default_desc.Height = height;
            } break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            {
                default_desc.Width            = width;
                default_desc.Height           = height;
                default_desc.DepthOrArraySize = depth;
            } break;

            default:
            {
                REV_ERROR_M("Invalid D3D12_RESOURCE_DIMENSION: 0x%I32X", default_desc.Dimension);
            } break;
        }

        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask     = 0;
        heap_properties.VisibleNodeMask      = 0;

        SafeRelease(texture->default_gpu_mem);
        HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &default_desc,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                        null,
                                                        IID_PPV_ARGS(&texture->default_gpu_mem));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    ID3D12Device4 *device_4 = null;
    HRESULT        error    = device->QueryInterface(&device_4);
    REV_CHECK(CheckResultAndPrintMessages(error));

    D3D12_RESOURCE_ALLOCATION_INFO1 info1{};
    device_4->GetResourceAllocationInfo1(0, 1, &default_desc, &info1);

    SafeRelease(device_4);

    texture->first_subresource_offset = info1.Offset;

    if (resource.flags & (GPU::RESOURCE_FLAG_CPU_WRITE | GPU::RESOURCE_FLAG_CPU_READ))
    {
        u64 subresources_count = texture->planes_count * texture->subtextures_count * texture->mip_levels;
        device->GetCopyableFootprints(&default_desc, 0, cast(u32, subresources_count), texture->first_subresource_offset, null, null, null, &texture->buffer_total_bytes);

        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
        heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask     = 0;
        heap_properties.VisibleNodeMask      = 0;

        D3D12_RESOURCE_DESC resource_desc;
        resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Width              = texture->buffer_total_bytes;
        resource_desc.Height             = 1;
        resource_desc.DepthOrArraySize   = 1;
        resource_desc.MipLevels          = 1;
        resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count   = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        if (resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            SafeRelease(texture->upload_gpu_mem);
            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            null,
                                                            IID_PPV_ARGS(&texture->upload_gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));

            D3D12_RANGE read_range = { 0, 0 };
            error = texture->upload_gpu_mem->Map(0, &read_range, cast(void **, &texture->upload_cpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }

        if (resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            heap_properties.Type = D3D12_HEAP_TYPE_READBACK;

            SafeRelease(texture->readback_gpu_mem);
            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            D3D12_RESOURCE_STATE_COPY_DEST,
                                                            null,
                                                            IID_PPV_ARGS(&texture->readback_gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));

            D3D12_RANGE read_range = { 0, texture->buffer_total_bytes };
            error = texture->readback_gpu_mem->Map(0, &read_range, cast(void **, &texture->readback_cpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }
}

const void *MemoryManager::GetBufferData(const GPU::ResourceHandle& resource)
{
    REV_CHECK_M(GPU::MemoryManager::IsBuffer(resource), "Resource is not a buffer");

    if (GPU::MemoryManager::IsStatic(resource))
    {
        const Buffer *buffer = m_StaticMemory.buffer_memory.buffers.GetPointer(resource.index);
        REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_READ, "You are not able to read data from \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());

        const ReadBackBufferMemoryPage *readback_page = m_StaticMemory.buffer_memory.readback_pages.GetPointer(buffer->readback_page_index);
        return readback_page->cpu_mem + buffer->readback_page_offset;
    }
    else
    {
        const Buffer *buffer = m_SceneMemory.buffer_memory.buffers.GetPointer(resource.index);
        REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_READ, "You are not able to read data from \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());

        const ReadBackBufferMemoryPage *readback_page = m_SceneMemory.buffer_memory.readback_pages.GetPointer(buffer->readback_page_index);
        return readback_page->cpu_mem + buffer->readback_page_offset;
    }
}

void MemoryManager::SetBufferData(const GPU::ResourceHandle& resource, const void *data)
{
    BufferMemory *buffer_memory = resource.flags & GPU::RESOURCE_FLAG_STATIC
                                ? &m_StaticMemory.buffer_memory
                                : &m_SceneMemory.buffer_memory;

    Buffer *buffer = buffer_memory->buffers.GetPointer(resource.index);
    REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE, "You are not able to upload data to \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());

    UploadBufferMemoryPage *upload_page = buffer_memory->upload_pages.GetPointer(buffer->upload_page_index);

    if (data) CopyMemory(upload_page->cpu_mem + buffer->upload_page_offset, data, buffer->actual_size);
    else      ZeroMemory(upload_page->cpu_mem + buffer->upload_page_offset,       buffer->actual_size);
}

GPU::TextureData *MemoryManager::GetTextureData(const GPU::ResourceHandle& resource)
{
    REV_CHECK_M(GPU::MemoryManager::IsTexture(resource), "Resource is not a texture");

    const Texture *texture = GPU::MemoryManager::IsStatic(resource)
                           ? m_StaticMemory.texture_memory.textures.GetPointer(resource.index)
                           : m_SceneMemory.texture_memory.textures.GetPointer(resource.index);
    REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_READ, "You are not able to read data from \"%.*s\" texture", texture->name.Length(), texture->name.Data());

    ID3D12Device *device = m_DeviceContext->Device();
    Memory       *memory = Memory::Get();

    u64 surfaces_count = texture->planes_count * texture->subtextures_count * texture->mip_levels;

    byte *footprints_memory = memory->PushToFA<byte>(surfaces_count * (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(u32) + sizeof(u64)));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints         = cast(D3D12_PLACED_SUBRESOURCE_FOOTPRINT *, footprints_memory);
    u32                                *unpadded_num_rows  = cast(u32 *, footprints        + surfaces_count);
    u64                                *unpadded_row_bytes = cast(u64 *, unpadded_num_rows + surfaces_count);
    device->GetCopyableFootprints(&texture->default_gpu_mem->GetDesc(), 0, cast(u32, surfaces_count), texture->first_subresource_offset, footprints, cast(UINT *, unpadded_num_rows), unpadded_row_bytes, null);

    GPU::TextureData *texture_data = cast(GPU::TextureData *, memory->PushToFrameArena(REV_StructFieldOffset(GPU::TextureData, surfaces) + surfaces_count * sizeof(GPU::TextureSurface)));

    texture_data->planes_count      = texture->planes_count;
    texture_data->subtextures_count = texture->subtextures_count;
    texture_data->mip_levels_count  = texture->mip_levels;
    texture_data->surfaces_count    = surfaces_count;

    for (GPU::TextureSurface *surface = texture_data->surfaces; surfaces_count--; ++surface)
    {
        surface->data          = texture->readback_cpu_mem + footprints->Offset;
        surface->row_bytes     = (*unpadded_row_bytes);
        surface->size_in_bytes = (*unpadded_row_bytes) * (*unpadded_num_rows);

        ++footprints;
        ++unpadded_num_rows;
        ++unpadded_row_bytes;
    }

    return texture_data;
}

void MemoryManager::SetTextureData(const GPU::ResourceHandle& resource, const GPU::TextureData *texture_data)
{
    Texture *texture = &GetTexture(resource);
    
    REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE, "You are not able to upload data to \"%.*s\" texture", texture->name.Length(), texture->name.Data());
    REV_CHECK(texture->planes_count == texture_data->planes_count);
    
    ID3D12Device *device = m_DeviceContext->Device();
    
    if (texture_data->surfaces_count)
    {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints = Memory::Get()->PushToFA<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(texture_data->surfaces_count);
        device->GetCopyableFootprints(&texture->default_gpu_mem->GetDesc(), 0, cast(u32, texture_data->surfaces_count), texture->first_subresource_offset, footprints, null, null, null);
        
        const GPU::TextureSurface *surfaces_end = texture_data->surfaces + texture_data->surfaces_count;
        for (const GPU::TextureSurface *surface = texture_data->surfaces; surface < surfaces_end; ++surface)
        {
            byte *dest_surface = texture->upload_cpu_mem + footprints->Offset;
            
            u64 num_rows = surface->size_in_bytes / surface->row_bytes;
            for (u64 row = 0; row < num_rows; ++row)
            {
                byte *dest_row = dest_surface  + footprints->Footprint.RowPitch * row;
                byte *src_row  = surface->data + surface->row_bytes             * row;
                
                CopyMemory(dest_row, src_row, surface->row_bytes);
            }
            
            ++footprints;
        }
    }
    else
    {
        ZeroMemory(texture->upload_cpu_mem, texture->buffer_total_bytes);
    }
}

void MemoryManager::FreeSceneMemory()
{
    FreeMemory(&m_SceneMemory);
}

void MemoryManager::FreeStaticMemory()
{
    FreeMemory(&m_StaticMemory);
}

ConstString MemoryManager::GetResourceName(const GPU::ResourceHandle& resource)
{
    ConstString name = null;

    if (GPU::MemoryManager::IsBuffer(resource))
    {
        Buffer *buffer = GPU::MemoryManager::IsStatic(resource)
                       ? m_StaticMemory.buffer_memory.buffers.GetPointer(resource.index)
                       : m_SceneMemory.buffer_memory.buffers.GetPointer(resource.index);
    
        name = buffer->name.ToConstString();
    }
    else if (GPU::MemoryManager::IsTexture(resource))
    {
        Texture *texture = GPU::MemoryManager::IsStatic(resource)
                         ? m_StaticMemory.texture_memory.textures.GetPointer(resource.index)
                         : m_SceneMemory.texture_memory.textures.GetPointer(resource.index);
    
        name = texture->name.ToConstString();
    }
    else
    {
        REV_WARNING_M("Resource #%I64u is not a buffer nor a texture so it doesn't have a name");
    }

    return name;
}

ConstString MemoryManager::GetBufferName(const ResourceHandle& resource)
{
    return GetBuffer(resource).name.ToConstString();
}

ConstString MemoryManager::GetTextureName(const ResourceHandle& resource)
{
    return GetTexture(resource).name.ToConstString();
}

Buffer *MemoryManager::AllocateBuffer(u64 size, DXGI_FORMAT format, D3D12_RESOURCE_STATES initial_state, GPU::RESOURCE_FLAG flags, u64& index, const ConstString& name)
{
    ID3D12Device *device = m_DeviceContext->Device();

    BufferMemory *buffer_memory = (flags & GPU::RESOURCE_FLAG_STATIC)
                                ? &m_StaticMemory.buffer_memory
                                : &m_SceneMemory.buffer_memory;

    index = buffer_memory->buffers.Count();

    Buffer *buffer       = new (buffer_memory->buffers.PushBack()) Buffer();
    buffer->format       = format;
    buffer->flags        = flags;
    buffer->actual_size  = size;
    buffer->aligned_size = AlignUp(size, 256);
    buffer->name         = name;

    {
        u64 page_index = 0;
        for (DefaultBufferMemoryPage& page : buffer_memory->default_pages)
        {
            if (page.occupied_bytes <= DEFAULT_BUFFER_MEMORY_PAGE_SIZE - buffer->aligned_size
            &&  page.initial_state  == initial_state
            &&  page.format         == format)
            {
                buffer->default_page_index  = page_index;
                buffer->default_page_offset = page.occupied_bytes;

                page.occupied_bytes += buffer->aligned_size;
                break;
            }
            ++page_index;
        }

        if (buffer->default_page_index == REV_INVALID_U64_INDEX)
        {
            buffer->default_page_index  = buffer_memory->default_pages.Count();
            buffer->default_page_offset = 0;

            D3D12_HEAP_PROPERTIES heap_properties;
            heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
            heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask     = 0;
            heap_properties.VisibleNodeMask      = 0;

            D3D12_RESOURCE_DESC resource_desc;
            resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            resource_desc.Width              = DEFAULT_BUFFER_MEMORY_PAGE_SIZE;
            resource_desc.Height             = 1;
            resource_desc.DepthOrArraySize   = 1;
            resource_desc.MipLevels          = 1;
            resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc.Count   = 1;
            resource_desc.SampleDesc.Quality = 0;
            resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            if (flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
            {
                resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }
            else if (flags & GPU::RESOURCE_FLAG_CPU_READ)
            {
                resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            DefaultBufferMemoryPage *page = buffer_memory->default_pages.PushBack();
            page->occupied_bytes          = buffer->aligned_size;
            page->initial_state           = initial_state;
            page->format                  = format;

            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            page->initial_state,
                                                            null,
                                                            IID_PPV_ARGS(&page->gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    if (flags & GPU::RESOURCE_FLAG_CPU_WRITE)
    {
        u64 page_index = 0;
        for (UploadBufferMemoryPage& page : buffer_memory->upload_pages)
        {
            if (page.occupied_bytes <= DEFAULT_BUFFER_MEMORY_PAGE_SIZE - buffer->aligned_size)
            {
                buffer->upload_page_index  = page_index;
                buffer->upload_page_offset = page.occupied_bytes;

                page.occupied_bytes += buffer->aligned_size;
                break;
            }
            ++page_index;
        }

        if (buffer->upload_page_index == REV_INVALID_U64_INDEX)
        {
            buffer->upload_page_index  = buffer_memory->upload_pages.Count();
            buffer->upload_page_offset = 0;

            D3D12_HEAP_PROPERTIES heap_properties;
            heap_properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
            heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask     = 0;
            heap_properties.VisibleNodeMask      = 0;

            D3D12_RESOURCE_DESC resource_desc;
            resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            resource_desc.Width              = UPLOAD_BUFFER_MEMORY_PAGE_SIZE;
            resource_desc.Height             = 1;
            resource_desc.DepthOrArraySize   = 1;
            resource_desc.MipLevels          = 1;
            resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc.Count   = 1;
            resource_desc.SampleDesc.Quality = 0;
            resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            UploadBufferMemoryPage *page = buffer_memory->upload_pages.PushBack();
            page->occupied_bytes         = buffer->aligned_size;
            
            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            null,
                                                            IID_PPV_ARGS(&page->gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));

            D3D12_RANGE read_range = { 0, 0 };
            error = page->gpu_mem->Map(0, &read_range, cast(void **, &page->cpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        u64 page_index = 0;
        for (ReadBackBufferMemoryPage& page : buffer_memory->readback_pages)
        {
            if (page.occupied_bytes <= DEFAULT_BUFFER_MEMORY_PAGE_SIZE - buffer->aligned_size)
            {
                buffer->readback_page_index  = page_index;
                buffer->readback_page_offset = page.occupied_bytes;

                page.occupied_bytes += buffer->aligned_size;
                break;
            }
            ++page_index;
        }

        if (buffer->readback_page_index == REV_INVALID_U64_INDEX)
        {
            buffer->readback_page_index  = buffer_memory->readback_pages.Count();
            buffer->readback_page_offset = 0;

            D3D12_HEAP_PROPERTIES heap_properties;
            heap_properties.Type                 = D3D12_HEAP_TYPE_READBACK;
            heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask     = 0;
            heap_properties.VisibleNodeMask      = 0;

            D3D12_RESOURCE_DESC resource_desc;
            resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            resource_desc.Width              = READBACK_BUFFER_MEMORY_PAGE_SIZE;
            resource_desc.Height             = 1;
            resource_desc.DepthOrArraySize   = 1;
            resource_desc.MipLevels          = 1;
            resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc.Count   = 1;
            resource_desc.SampleDesc.Quality = 0;
            resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            ReadBackBufferMemoryPage *page = buffer_memory->readback_pages.PushBack();
            page->occupied_bytes           = buffer->aligned_size;

            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            D3D12_RESOURCE_STATE_COPY_DEST,
                                                            null,
                                                            IID_PPV_ARGS(&page->gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));

            D3D12_RANGE read_range = { 0, READBACK_BUFFER_MEMORY_PAGE_SIZE };
            error = page->gpu_mem->Map(0, &read_range, cast(void **, &page->cpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    SetBufferName(buffer_memory, buffer, flags, name);
    return buffer;
}

Texture *MemoryManager::AllocateTexture(TEXTURE_DIMENSION dimension, const D3D12_RESOURCE_DESC& desc, GPU::RESOURCE_FLAG flags, u8 planes_count, u64& index, const ConstString& name)
{
    ID3D12Device *device = m_DeviceContext->Device();

    TextureMemory *texture_memory = (flags & GPU::RESOURCE_FLAG_STATIC)
                                  ? &m_StaticMemory.texture_memory
                                  : &m_SceneMemory.texture_memory;

    index = texture_memory->textures.Count();

    Texture *texture           = texture_memory->textures.PushBack();
    texture->dimension         = dimension;
    texture->format            = desc.Format;
    texture->flags             = flags;
    texture->depth             = 1;
    texture->mip_levels        = desc.MipLevels;
    texture->subtextures_count = 1;
    texture->planes_count      = planes_count;
    texture->name              = name;

    ID3D12Device4 *device_4 = null;
    HRESULT        error    = device->QueryInterface(&device_4);
    REV_CHECK(CheckResultAndPrintMessages(error));

    D3D12_RESOURCE_ALLOCATION_INFO1 info1{};
    device_4->GetResourceAllocationInfo1(0, 1, &desc, &info1);

    SafeRelease(device_4);

    texture->first_subresource_offset = info1.Offset;

    {
        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask     = 0;
        heap_properties.VisibleNodeMask      = 0;

        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                                    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &desc,
                                                        state,
                                                        null,
                                                        IID_PPV_ARGS(&texture->default_gpu_mem));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    if (flags & (GPU::RESOURCE_FLAG_CPU_READ | GPU::RESOURCE_FLAG_CPU_WRITE))
    {
        u64 subresources_count = texture->planes_count * texture->subtextures_count * texture->mip_levels;
        device->GetCopyableFootprints(&desc, 0, cast(u32, subresources_count), texture->first_subresource_offset, null, null, null, &texture->buffer_total_bytes);

        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
        heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask     = 0;
        heap_properties.VisibleNodeMask      = 0;

        D3D12_RESOURCE_DESC resource_desc;
        resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Width              = texture->buffer_total_bytes;
        resource_desc.Height             = 1;
        resource_desc.DepthOrArraySize   = 1;
        resource_desc.MipLevels          = 1;
        resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count   = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        if (flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            null,
                                                            IID_PPV_ARGS(&texture->upload_gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));

            D3D12_RANGE read_range = { 0, 0 };
            error = texture->upload_gpu_mem->Map(0, &read_range, cast(void **, &texture->upload_cpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }

        if (flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            heap_properties.Type = D3D12_HEAP_TYPE_READBACK;

            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            D3D12_RESOURCE_STATE_COPY_DEST,
                                                            null,
                                                            IID_PPV_ARGS(&texture->readback_gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));

            D3D12_RANGE read_range = { 0, texture->buffer_total_bytes };
            error = texture->readback_gpu_mem->Map(0, &read_range, cast(void **, &texture->readback_cpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    SetTextureName(texture_memory, texture, flags, name);
    return texture;
}

void MemoryManager::UploadResources(ID3D12GraphicsCommandList *command_list, const ConstArray<GPU::ResourceHandle>& resources)
{
    if (resources.Empty()) return;

    D3D12_RESOURCE_BARRIER *barriers    = Memory::Get()->PushToFA<D3D12_RESOURCE_BARRIER>(resources.Count());
    D3D12_RESOURCE_BARRIER *barriers_it = barriers;

    for (const GPU::ResourceHandle& resource : resources)
    {
        if (GPU::MemoryManager::IsBuffer(resource))
        {
            BufferMemory *buffer_memory = (resource.flags & GPU::RESOURCE_FLAG_STATIC)
                                        ? &m_StaticMemory.buffer_memory
                                        : &m_SceneMemory.buffer_memory;

            Buffer *buffer = buffer_memory->buffers.GetPointer(resource.index);
            REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE, "You are not able to upload data to \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());
        
            DefaultBufferMemoryPage *default_page = buffer_memory->default_pages.GetPointer(buffer->default_page_index);
            UploadBufferMemoryPage  *upload_page  = buffer_memory->upload_pages.GetPointer(buffer->upload_page_index);
        
            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.pResource   = default_page->gpu_mem;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.StateBefore = default_page->initial_state;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

            ++barriers_it;
        }
    }

    command_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);

    barriers_it = barriers;
    for (const GPU::ResourceHandle& resource : resources)
    {
        if (GPU::MemoryManager::IsBuffer(resource))
        {
            BufferMemory *buffer_memory = (resource.flags & GPU::RESOURCE_FLAG_STATIC)
                                        ? &m_StaticMemory.buffer_memory
                                        : &m_SceneMemory.buffer_memory;

            Buffer                  *buffer       = buffer_memory->buffers.GetPointer(resource.index);
            DefaultBufferMemoryPage *default_page = buffer_memory->default_pages.GetPointer(buffer->default_page_index);
            UploadBufferMemoryPage  *upload_page  = buffer_memory->upload_pages.GetPointer(buffer->upload_page_index);

            command_list->CopyBufferRegion(default_page->gpu_mem, buffer->default_page_offset, upload_page->gpu_mem, buffer->upload_page_offset, buffer->actual_size);
        
            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers_it->Transition.StateAfter  = default_page->initial_state;
        
            ++barriers_it;
        }
        else
        {
            Texture *texture = &GetTexture(resource);
            REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE, "You are not able to upload data to \"%.*s\" texture", texture->name.Length(), texture->name.Data());
            
            ID3D12Device *device = m_DeviceContext->Device();
            
            u64 subresources_count = texture->planes_count * texture->subtextures_count * texture->mip_levels;
            
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints = Memory::Get()->PushToFA<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(subresources_count);
            device->GetCopyableFootprints(&texture->default_gpu_mem->GetDesc(), 0, cast(u32, subresources_count), texture->first_subresource_offset, footprints, null, null, null);
            
            D3D12_TEXTURE_COPY_LOCATION source_location;
            source_location.pResource = texture->upload_gpu_mem;
            source_location.Type      = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            
            D3D12_TEXTURE_COPY_LOCATION dest_location;
            dest_location.pResource = texture->default_gpu_mem;
            dest_location.Type      = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            
            for (u32 i = 0; i < subresources_count; ++i)
            {
                source_location.PlacedFootprint = footprints[i];
                dest_location.SubresourceIndex  = i;

                command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, null);
            }
        }
    }

    command_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);
}

void MemoryManager::ReadbackResources(ID3D12GraphicsCommandList *command_list, const ConstArray<GPU::ResourceHandle>& resources)
{
    if (resources.Empty()) return;

    D3D12_RESOURCE_BARRIER *barriers    = Memory::Get()->PushToFA<D3D12_RESOURCE_BARRIER>(resources.Count());
    D3D12_RESOURCE_BARRIER *barriers_it = barriers;

    for (const GPU::ResourceHandle& resource : resources)
    {
        if (GPU::MemoryManager::IsBuffer(resource))
        {
            BufferMemory *buffer_memory = resource.flags & GPU::RESOURCE_FLAG_STATIC
                                        ? &m_StaticMemory.buffer_memory
                                        : &m_SceneMemory.buffer_memory;

            Buffer *buffer = buffer_memory->buffers.GetPointer(resource.index);
            REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_READ, "You are not able to read data from \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());
            
            DefaultBufferMemoryPage *default_page = buffer_memory->default_pages.GetPointer(buffer->default_page_index);
            
            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.pResource   = default_page->gpu_mem;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.StateBefore = default_page->initial_state;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

            ++barriers_it;
        }
    }

    command_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);

    barriers_it = barriers;
    for (const GPU::ResourceHandle& resource : resources)
    {
        if (GPU::MemoryManager::IsBuffer(resource))
        {
            BufferMemory *buffer_memory = resource.flags & GPU::RESOURCE_FLAG_STATIC
                                        ? &m_StaticMemory.buffer_memory
                                        : &m_SceneMemory.buffer_memory;

            Buffer                   *buffer        = buffer_memory->buffers.GetPointer(resource.index);
            DefaultBufferMemoryPage  *default_page  = buffer_memory->default_pages.GetPointer(buffer->default_page_index);
            ReadBackBufferMemoryPage *readback_page = buffer_memory->readback_pages.GetPointer(buffer->readback_page_index);

            command_list->CopyBufferRegion(readback_page->gpu_mem, buffer->readback_page_offset, default_page->gpu_mem, buffer->default_page_offset, buffer->actual_size);

            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers_it->Transition.StateAfter  = default_page->initial_state;

            ++barriers_it;
        }
        else
        {
            Texture *texture = &GetTexture(resource);
            REV_CHECK_M(resource.flags & GPU::RESOURCE_FLAG_CPU_READ, "You are not able to read data from \"%.*s\" texture", texture->name.Length(), texture->name.Data());
            
            ID3D12Device *device = m_DeviceContext->Device();
            
            u64 subresources_count = texture->planes_count * texture->subtextures_count * texture->mip_levels;
            
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints = Memory::Get()->PushToFA<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(subresources_count);
            device->GetCopyableFootprints(&texture->default_gpu_mem->GetDesc(), 0, cast(u32, subresources_count), texture->first_subresource_offset, footprints, null, null, null);
            
            D3D12_TEXTURE_COPY_LOCATION source_location;
            source_location.pResource = texture->default_gpu_mem;
            source_location.Type      = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            
            D3D12_TEXTURE_COPY_LOCATION dest_location;
            dest_location.pResource = texture->readback_gpu_mem;
            dest_location.Type      = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            
            for (u32 i = 0; i < subresources_count; ++i)
            {
                source_location.SubresourceIndex = i;
                dest_location.PlacedFootprint    = footprints[i];

                command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, null);
            }
        }
    }

    command_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);
}

void MemoryManager::SetBufferName(BufferMemory *buffer_memory, Buffer *buffer, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    wchar_t *wname = Memory::Get()->PushToFA<wchar_t>(name.Length() + 1);
    MultiByteToWideChar(CP_ACP, 0, name.Data(), cast(u32, name.Length()), wname, cast(u32, name.Length()));

    {
        DefaultBufferMemoryPage *page  = buffer_memory->default_pages.GetPointer(buffer->default_page_index);

        UINT    gotten_wname_length = 0;
        HRESULT error               = page->gpu_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &gotten_wname_length, null);

        if (gotten_wname_length)
        {
            gotten_wname_length = gotten_wname_length / sizeof(wchar_t) - 1;

            u32      new_wname_length = cast(u32, gotten_wname_length + 1 + name.Length());
            wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

            UINT data_size = (new_wname_length + 1) * sizeof(wchar_t);
            error = page->gpu_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
            REV_CHECK(gotten_wname_length == data_size / sizeof(wchar_t) - 1);

            _snwprintf(new_wname + gotten_wname_length, new_wname_length - gotten_wname_length, L" %.*s", cast(u32, name.Length()), wname);

            error = page->gpu_mem->SetName(new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
        else
        {
            u32      new_wname_length = cast(u32, REV_CSTRLEN("DEFAULT ") + name.Length());
            wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

            _snwprintf(new_wname, new_wname_length, L"DEFAULT %.*s", cast(u32, name.Length()), wname);

            error = page->gpu_mem->SetName(new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    if (flags & GPU::RESOURCE_FLAG_CPU_WRITE)
    {
        UploadBufferMemoryPage *page = buffer_memory->upload_pages.GetPointer(buffer->upload_page_index);

        UINT    gotten_wname_length = 0;
        HRESULT error               = page->gpu_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &gotten_wname_length, null);

        if (gotten_wname_length)
        {
            gotten_wname_length = gotten_wname_length / sizeof(wchar_t) - 1;

            u32      new_wname_length = cast(u32, gotten_wname_length + 1 + name.Length());
            wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

            UINT data_size = (new_wname_length + 1) * sizeof(wchar_t);
            error = page->gpu_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
            REV_CHECK(gotten_wname_length == data_size / sizeof(wchar_t) - 1);

            _snwprintf(new_wname + gotten_wname_length, new_wname_length - gotten_wname_length, L" %.*s", cast(u32, name.Length()), wname);

            error = page->gpu_mem->SetName(new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
        else
        {
            u32      new_wname_length = cast(u32, REV_CSTRLEN("UPLOAD ") + name.Length());
            wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

            _snwprintf(new_wname, new_wname_length, L"UPLOAD %.*s", cast(u32, name.Length()), wname);

            error = page->gpu_mem->SetName(new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        ReadBackBufferMemoryPage *page = buffer_memory->readback_pages.GetPointer(buffer->readback_page_index);

        UINT    gotten_wname_length = 0;
        HRESULT error               = page->gpu_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &gotten_wname_length, null);

        if (gotten_wname_length)
        {
            gotten_wname_length = gotten_wname_length / sizeof(wchar_t) - 1;

            u32      new_wname_length = cast(u32, gotten_wname_length + 1 + name.Length());
            wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

            UINT data_size = (new_wname_length + 1) * sizeof(wchar_t);
            error = page->gpu_mem->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
            REV_CHECK(gotten_wname_length == data_size / sizeof(wchar_t) - 1);

            _snwprintf(new_wname + gotten_wname_length, new_wname_length - gotten_wname_length, L" %.*s", cast(u32, name.Length()), wname);

            error = page->gpu_mem->SetName(new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
        else
        {
            u32      new_wname_length = cast(u32, REV_CSTRLEN("READBACK ") + name.Length());
            wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

            _snwprintf(new_wname, new_wname_length, L"READBACK %.*s", cast(u32, name.Length()), wname);

            error = page->gpu_mem->SetName(new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }
}

void MemoryManager::SetTextureName(TextureMemory *texture_memory, Texture *texture, GPU::RESOURCE_FLAG flags, const ConstString& name)
{
    wchar_t *wname = Memory::Get()->PushToFA<wchar_t>(name.Length() + 1);
    MultiByteToWideChar(CP_ACP, 0, name.Data(), cast(u32, name.Length()), wname, cast(u32, name.Length()));

    u32      new_wname_length = cast(u32, 11 + name.Length());
    wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

    {
        CopyMemory(new_wname, L"DEFAULT ", sizeof(L"DEFAULT ") - sizeof(wchar_t));
        CopyMemory(new_wname + REV_CSTRLEN(L"DEFAULT "), wname, cast(u32, name.Length() * sizeof(wchar_t)));

        HRESULT error = texture->default_gpu_mem->SetName(new_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    if (flags & GPU::RESOURCE_FLAG_CPU_WRITE)
    {
        _snwprintf(new_wname, new_wname_length, L"UPLOAD %.*s", cast(u32, name.Length()), wname);

        HRESULT error = texture->upload_gpu_mem->SetName(new_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    if (flags & GPU::RESOURCE_FLAG_CPU_READ)
    {
        _snwprintf(new_wname, new_wname_length, L"READBACK %.*s", cast(u32, name.Length()), wname);

        HRESULT error = texture->readback_gpu_mem->SetName(new_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }
}

void MemoryManager::FreeMemory(ResourceMemory *memory)
{
    memory->buffer_memory.buffers.Clear();

    for (DefaultBufferMemoryPage& page : memory->buffer_memory.default_pages)
    {
        SafeRelease(page.gpu_mem);
    }
    memory->buffer_memory.default_pages.Clear();

    for (UploadBufferMemoryPage& page : memory->buffer_memory.upload_pages)
    {
        SafeRelease(page.gpu_mem);
    }
    memory->buffer_memory.upload_pages.Clear();

    for (ReadBackBufferMemoryPage& page : memory->buffer_memory.readback_pages)
    {
        SafeRelease(page.gpu_mem);
    }
    memory->buffer_memory.readback_pages.Clear();

    for (Texture& texture : memory->texture_memory.textures)
    {
        SafeRelease(texture.default_gpu_mem);
        SafeRelease(texture.upload_gpu_mem);
        SafeRelease(texture.readback_gpu_mem);
    }
    memory->texture_memory.textures.Clear();

    memory->sampler_memory.samplers.Clear();
}

};
