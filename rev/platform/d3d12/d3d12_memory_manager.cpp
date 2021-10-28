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
        FreeMemory(&m_SceneMemory);
        FreeMemory(&m_StaticMemory);
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

u64 MemoryManager::AllocateVertexBuffer(u32 count, bool _static, const ConstString& name)
{
    REV_CHECK(count);

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(_static ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory,
                                    CPU_RESOURCE_ACCESS_WRITE,
                                    DXGI_FORMAT_UNKNOWN,
                                    count * sizeof(Vertex),
                                    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                    index,
                                    name);

    buffer->stride = sizeof(Vertex);

    return index;
}

u64 MemoryManager::AllocateIndexBuffer(u32 count, bool _static, const ConstString& name)
{
    REV_CHECK(count);

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(_static ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory,
                                    CPU_RESOURCE_ACCESS_WRITE,
                                    DXGI_FORMAT_UNKNOWN,
                                    count * sizeof(Index),
                                    D3D12_RESOURCE_STATE_INDEX_BUFFER,
                                    index,
                                    name);

    buffer->format = DXGI_FORMAT_R32_UINT;
    buffer->stride = sizeof(Index);

    return index;
}

u64 MemoryManager::AllocateConstantBuffer(u32 bytes, bool _static, const ConstString& name)
{
    REV_CHECK(bytes);

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(_static ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory,
                                    CPU_RESOURCE_ACCESS_WRITE,
                                    DXGI_FORMAT_UNKNOWN,
                                    bytes,
                                    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                    index,
                                    name);

    return index;
}

u64 MemoryManager::AllocateBuffer(u32 bytes, GPU::BUFFER_FORMAT format, bool cpu_read_access, bool _static, const ConstString& name)
{
    REV_CHECK(bytes);

    CPU_RESOURCE_ACCESS   access = CPU_RESOURCE_ACCESS_WRITE;
    D3D12_RESOURCE_STATES state  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // @TODO(Roman): But what about compute shaders? Should we use D3D12_RESOURCE_STATE_COMMON then?
    if (cpu_read_access)
    {
        access |= CPU_RESOURCE_ACCESS_READ;
        state   = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    u32         stride      = 0;
    DXGI_FORMAT dxgi_format = REVToDXGIBufferFormat(format, stride);

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(_static ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory, access, dxgi_format, bytes, state, index, name);

    buffer->ro_rw_type = RO_RW_BUFFER_TYPE_BUFFER;
    buffer->format     = dxgi_format;
    buffer->stride     = stride;

    return index;
}

u64 MemoryManager::AllocateStructuredBuffer(u32 count, u32 stride, bool cpu_read_access, bool _static, const ConstString& name)
{
    REV_CHECK(count);
    REV_CHECK(stride);

    CPU_RESOURCE_ACCESS   access = CPU_RESOURCE_ACCESS_WRITE;
    D3D12_RESOURCE_STATES state  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // @TODO(Roman): But what about compute shaders? Should we use D3D12_RESOURCE_STATE_COMMON then?
    if (cpu_read_access)
    {
        access |= CPU_RESOURCE_ACCESS_READ;
        state   = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(_static ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory, access, DXGI_FORMAT_UNKNOWN, count * stride, state, index, name);

    buffer->ro_rw_type = RO_RW_BUFFER_TYPE_STRUCTURED;
    buffer->stride     = stride;
    
    return index;
}

u64 MemoryManager::AllocateByteAddressBuffer(u32 bytes, bool cpu_read_access, bool _static, const ConstString& name)
{
    REV_CHECK(bytes);

    CPU_RESOURCE_ACCESS   access = CPU_RESOURCE_ACCESS_WRITE;
    D3D12_RESOURCE_STATES state  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // @TODO(Roman): But what about compute shaders? Should we use D3D12_RESOURCE_STATE_COMMON then?
    if (cpu_read_access)
    {
        access |= CPU_RESOURCE_ACCESS_READ;
        state   = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    u64     index  = REV_INVALID_U64_INDEX;
    Buffer *buffer = AllocateBuffer(_static ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory, access, DXGI_FORMAT_UNKNOWN, bytes, state, index, name);

    buffer->ro_rw_type = RO_RW_BUFFER_TYPE_BYTE_ADDRESS;
    buffer->stride     = 1;

    return index;
}

REV_INTERNAL REV_INLINE u16 GetMaxMipLevels(u16 width, u16 height = REV_U16_MAX, u16 depth = REV_U16_MAX)
{
    u32 count = 0;
    _BitScanReverse(&count, width & height & depth);
    return cast(u16, count + 1);
}

u64 MemoryManager::AllocateTexture(u16 width, u16 height, u16 depth, u16 mip_levels, GPU::RESOURCE_KIND resource_kind, GPU::TEXTURE_FORMAT texture_format, const ConstString& name)
{
    D3D12_RESOURCE_DESC desc{};
    TEXTURE_DIMENSION   dimension = TEXTURE_DIMENSION_UNKNOWN;

    if (depth > 0)
    {
        REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Width gotta be = [1, %hu].", D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
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

        desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        desc.Height           = height;
        desc.DepthOrArraySize = depth;

        dimension = TEXTURE_DIMENSION_3D;
    }
    else if (height > 0)
    {
        REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Width gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
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

        desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Height           = height;
        desc.DepthOrArraySize = 1;

        dimension = TEXTURE_DIMENSION_2D;
    }
    else if (width > 0)
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

        desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        desc.Height           = 1;
        desc.DepthOrArraySize = 1;

        dimension = TEXTURE_DIMENSION_1D;
    }
    REV_CHECK_M(dimension != TEXTURE_DIMENSION_UNKNOWN, "You can not allocate a texture with width = 0, height = 0 and depth = 0");

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(texture_format);
    REV_CHECK_M(planes_count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(texture_format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    TextureMemory       *texture_memory = &m_SceneMemory.texture_memory;
    CPU_RESOURCE_ACCESS  access         = CPU_RESOURCE_ACCESS_WRITE;

    if ((resource_kind & ~GPU::RESOURCE_KIND_STATIC) == GPU::RESOURCE_KIND_READ_WRITE_TEXTURE)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        access     |= CPU_RESOURCE_ACCESS_READ;
    }
    if (resource_kind & GPU::RESOURCE_KIND_STATIC)
    {
        texture_memory = &m_StaticMemory.texture_memory;
    }

    return AllocateTexture(texture_memory, dimension, access, desc, planes_count, name);
}

u64 MemoryManager::AllocateTextureArray(u16 width, u16 height, u16 count, u16 mip_levels, GPU::RESOURCE_KIND resource_kind, GPU::TEXTURE_FORMAT texture_format, const ConstString& name)
{
    REV_CHECK_M(1 <= count && count <= D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION, "Count gotta be = [1, %hu].",  D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);

    D3D12_RESOURCE_DESC desc{};
    TEXTURE_DIMENSION   dimension = TEXTURE_DIMENSION_UNKNOWN;

    if (height > 0)
    {
        REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Width gotta be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
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

        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Height    = height;

        dimension = TEXTURE_DIMENSION_2D_ARRAY;
    }
    else if (width > 0)
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

        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        desc.Height    = 1;

        dimension = TEXTURE_DIMENSION_1D_ARRAY;
    }
    REV_CHECK_M(dimension != TEXTURE_DIMENSION_UNKNOWN, "You can not allocate a texture with width = 0, height = 0 and depth = 0");

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(texture_format);
    REV_CHECK_M(planes_count * count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.DepthOrArraySize   = count;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(texture_format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    TextureMemory       *texture_memory = &m_SceneMemory.texture_memory;
    CPU_RESOURCE_ACCESS  access         = CPU_RESOURCE_ACCESS_WRITE;

    if ((resource_kind & ~GPU::RESOURCE_KIND_STATIC) == GPU::RESOURCE_KIND_READ_WRITE_TEXTURE)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        access     |= CPU_RESOURCE_ACCESS_READ;
    }
    if (resource_kind & GPU::RESOURCE_KIND_STATIC)
    {
        texture_memory = &m_StaticMemory.texture_memory;
    }

    return AllocateTexture(texture_memory, dimension, access, desc, planes_count, name);
}

u64 MemoryManager::AllocateTextureCube(u16 width, u16 height, u16 mip_levels, GPU::RESOURCE_KIND resource_kind, GPU::TEXTURE_FORMAT texture_format, const ConstString& name)
{
    REV_CHECK_M(1 <= width  && width  <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Width gotta be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);
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

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(texture_format);
    REV_CHECK_M(planes_count * 6 * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = 6;
    desc.MipLevels          = mip_levels;
    desc.Format             = REVToDXGITextureFormat(texture_format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    TextureMemory       *texture_memory = &m_SceneMemory.texture_memory;
    CPU_RESOURCE_ACCESS  access         = CPU_RESOURCE_ACCESS_WRITE;

    if ((resource_kind & ~GPU::RESOURCE_KIND_STATIC) == GPU::RESOURCE_KIND_READ_WRITE_TEXTURE)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        access     |= CPU_RESOURCE_ACCESS_READ;
    }
    if (resource_kind & GPU::RESOURCE_KIND_STATIC)
    {
        texture_memory  = &m_StaticMemory.texture_memory;
    }

    return AllocateTexture(texture_memory, TEXTURE_DIMENSION_CUBE, access, desc, planes_count, name);
}

u64 MemoryManager::AllocateTextureCubeArray(u16 width, u16 height, u16 count, u16 mip_levels, GPU::RESOURCE_KIND resource_kind, GPU::TEXTURE_FORMAT texture_format, const ConstString& name)
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

    u8 planes_count = m_DeviceContext->GetFormatPlanesCount(texture_format);
    REV_CHECK_M(planes_count * 6 * count * mip_levels <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    D3D12_RESOURCE_DESC desc;
    desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Width              = width;
    desc.Height             = height;
    desc.DepthOrArraySize   = 6 * count;
    desc.MipLevels          = 1;
    desc.Format             = REVToDXGITextureFormat(texture_format);
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

    TextureMemory       *texture_memory = &m_SceneMemory.texture_memory;
    CPU_RESOURCE_ACCESS  access         = CPU_RESOURCE_ACCESS_WRITE;

    if ((resource_kind & ~GPU::RESOURCE_KIND_STATIC) == GPU::RESOURCE_KIND_READ_WRITE_TEXTURE)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        access     |= CPU_RESOURCE_ACCESS_READ;
    }
    if (resource_kind & GPU::RESOURCE_KIND_STATIC)
    {
        texture_memory = &m_StaticMemory.texture_memory;
    }

    return AllocateTexture(texture_memory, TEXTURE_DIMENSION_CUBE_ARRAY, access, desc, planes_count, name);
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

void MemoryManager::SetBufferData(const GPU::ResourceHandle& resource, const void *data)
{
    UploadBufferData(m_DeviceContext->CurrentGraphicsList(),
                     &GetBuffer(resource),
                     GPU::MemoryManager::IsStatic(resource),
                     data);
}

void MemoryManager::SetBufferDataImmediately(const GPU::ResourceHandle& resource, const void *data)
{
    UploadBufferData(m_CommandList,
                     &GetBuffer(resource),
                     GPU::MemoryManager::IsStatic(resource),
                     data);
}

void MemoryManager::SetTextureData(const GPU::ResourceHandle& resource, const GPU::TextureData *data)
{
    UploadTextureData(m_DeviceContext->CurrentGraphicsList(), &GetTexture(resource), data);
}

void MemoryManager::SetTextureDataImmediately(const GPU::ResourceHandle& resource, const GPU::TextureData *data)
{
    UploadTextureData(m_CommandList, &GetTexture(resource), data);
}

void MemoryManager::CopyDefaultResourcesToReadBackResources(const ConstArray<GPU::ResourceHandle>& read_write_resources)
{
    for (const GPU::ResourceHandle& resource : read_write_resources)
    {
        if (GPU::MemoryManager::IsBuffer(resource))
        {
            CopyDefaultBufferResourceToReadBackResource(m_DeviceContext->CurrentGraphicsList(), &GetBuffer(resource), GPU::MemoryManager::IsStatic(resource));
        }
        else if (GPU::MemoryManager::IsTexture(resource))
        {
            CopyDefaultTextureResourceToReadBackResource(m_DeviceContext->CurrentGraphicsList(), &GetTexture(resource));
        }
    }
}

const void *MemoryManager::GetBufferData(const GPU::ResourceHandle& resource) const
{
    REV_CHECK_M(GPU::MemoryManager::IsBuffer(resource), "Resource is not a buffer");

    if (GPU::MemoryManager::IsStatic(resource))
    {
        const Buffer *buffer = m_StaticMemory.buffer_memory.buffers.GetPointer(resource.index);
        REV_CHECK_M(buffer->cpu_access & CPU_RESOURCE_ACCESS_READ, "You are not able to read data from \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());

        const ReadBackBufferMemoryPage *readback_page = m_StaticMemory.buffer_memory.readback_pages.GetPointer(buffer->readback_page_index);
        return readback_page->cpu_mem[m_DeviceContext->CurrentBuffer()] + buffer->readback_page_offset;
    }
    else
    {
        const Buffer *buffer = m_SceneMemory.buffer_memory.buffers.GetPointer(resource.index);
        REV_CHECK_M(buffer->cpu_access & CPU_RESOURCE_ACCESS_READ, "You are not able to read data from \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());

        const ReadBackBufferMemoryPage *readback_page = m_SceneMemory.buffer_memory.readback_pages.GetPointer(buffer->readback_page_index);
        return readback_page->cpu_mem[m_DeviceContext->CurrentBuffer()] + buffer->readback_page_offset;
    }
}

GPU::TextureData *MemoryManager::GetTextureData(const GPU::ResourceHandle& resource) const
{
    const Texture *texture = m_StaticMemory.texture_memory.textures.GetPointer(resource.index);
    REV_CHECK_M(texture->cpu_access & CPU_RESOURCE_ACCESS_READ, "You are not able to read data from \"%.*s\" texture", texture->name.Length(), texture->name.Data());

    byte *readback_memory = texture->readback_cpu_mem[m_DeviceContext->CurrentBuffer()];

    ID3D12Device *device = m_DeviceContext->Device();
    Memory       *memory = Memory::Get();

    u64 surfaces_count    = texture->planes_count * texture->desc.MipLevels;
    u16 subtextures_count = 1;

    if (texture->dimension >= TEXTURE_DIMENSION_CUBE)
    {
        surfaces_count    *= texture->desc.DepthOrArraySize;
        subtextures_count  = texture->desc.DepthOrArraySize;
    }

    byte *footprints_memory = memory->PushToFA<byte>(surfaces_count * (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(u32) + sizeof(u64)));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints         = cast(D3D12_PLACED_SUBRESOURCE_FOOTPRINT *, footprints_memory);
    u32                                *unpadded_num_rows  = cast(u32 *, footprints        + surfaces_count);
    u64                                *unpadded_row_bytes = cast(u64 *, unpadded_num_rows + surfaces_count);
    device->GetCopyableFootprints(&texture->desc, 0, cast(u32, surfaces_count), texture->first_subresource_offset, footprints, cast(UINT *, unpadded_num_rows), unpadded_row_bytes, null);

    GPU::TextureData *texture_data = cast(GPU::TextureData *, memory->PushToFrameArena(REV_StructFieldOffset(GPU::TextureData, surfaces) + surfaces_count * sizeof(GPU::TextureSurface)));

    texture_data->planes_count      = texture->planes_count;
    texture_data->subtextures_count = subtextures_count;
    texture_data->mip_levels_count  = texture->desc.MipLevels;
    texture_data->surfaces_count    = surfaces_count;

    for (GPU::TextureSurface *surface = texture_data->surfaces; surfaces_count--; ++surface)
    {
        surface->data          = readback_memory + footprints->Offset;
        surface->row_bytes     = (*unpadded_row_bytes);
        surface->size_in_bytes = (*unpadded_row_bytes) * (*unpadded_num_rows);

        ++footprints;
        ++unpadded_num_rows;
        ++unpadded_row_bytes;
    }

    return texture_data;
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
    FreeMemory(&m_SceneMemory);
}

void MemoryManager::FreeStaticMemory()
{
    FreeMemory(&m_StaticMemory);
}

Buffer *MemoryManager::AllocateBuffer(
    BufferMemory          *buffer_memory,
    CPU_RESOURCE_ACCESS    cpu_access,
    DXGI_FORMAT            format,
    u64                    size,
    D3D12_RESOURCE_STATES  initial_state,
    u64&                   index,
    const ConstString&     name)
{
    ID3D12Device *device = m_DeviceContext->Device();

    index = buffer_memory->buffers.Count();

    Buffer *buffer              = new (buffer_memory->buffers.PushBack()) Buffer();
    buffer->cpu_access          = cpu_access;
    buffer->format              = format;
    buffer->actual_size         = size;
    buffer->aligned_size        = AlignUp(size, 256);
    buffer->name                = name;

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

            if (buffer->cpu_access & CPU_RESOURCE_ACCESS_READ)
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

    if (buffer->cpu_access & CPU_RESOURCE_ACCESS_WRITE)
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

            for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
            {
                HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                                D3D12_HEAP_FLAG_NONE,
                                                                &resource_desc,
                                                                D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                null,
                                                                IID_PPV_ARGS(page->gpu_mem + i));
                REV_CHECK(CheckResultAndPrintMessages(error));

                D3D12_RANGE read_range = { 0, 0 };
                error = page->gpu_mem[i]->Map(0, &read_range, cast(void **, page->cpu_mem + i));
                REV_CHECK(CheckResultAndPrintMessages(error));
            }
        }
    }

    if (buffer->cpu_access & CPU_RESOURCE_ACCESS_READ)
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

            for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
            {
                HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                                D3D12_HEAP_FLAG_NONE,
                                                                &resource_desc,
                                                                D3D12_RESOURCE_STATE_COPY_DEST,
                                                                null,
                                                                IID_PPV_ARGS(page->gpu_mem + i));
                REV_CHECK(CheckResultAndPrintMessages(error));

                D3D12_RANGE read_range = { 0, READBACK_BUFFER_MEMORY_PAGE_SIZE };
                error = page->gpu_mem[i]->Map(0, &read_range, cast(void **, page->cpu_mem + i));
                REV_CHECK(CheckResultAndPrintMessages(error));
            }
        }
    }

    SetBufferName(buffer_memory, buffer, name);

    return buffer;
}

u64 MemoryManager::AllocateTexture(
    TextureMemory              *texture_memory,
    TEXTURE_DIMENSION           dimension,
    CPU_RESOURCE_ACCESS         cpu_access,
    const D3D12_RESOURCE_DESC&  desc,
    u8                          planes_count,
    const ConstString&          name)
{
    ID3D12Device *device = m_DeviceContext->Device();

    Texture *texture      = texture_memory->textures.PushBack();
    texture->dimension    = dimension;
    texture->cpu_access   = cpu_access;
    texture->desc         = desc;
    texture->planes_count = planes_count;
    texture->name         = name;

    ID3D12Device4 *device_4 = null;
    HRESULT        error    = device->QueryInterface(&device_4);
    REV_CHECK(CheckResultAndPrintMessages(error));

    D3D12_RESOURCE_ALLOCATION_INFO1 info1{};
    device_4->GetResourceAllocationInfo1(0, 1, &texture->desc, &info1);

    SafeRelease(device_4);

    texture->first_subresource_offset = info1.Offset;

    {
        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask     = 0;
        heap_properties.VisibleNodeMask      = 0;

        D3D12_RESOURCE_STATES state = (texture->cpu_access & CPU_RESOURCE_ACCESS_READ)
                                    ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                                    : D3D12_RESOURCE_STATE_COMMON;
        
        HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &texture->desc,
                                                        state,
                                                        null,
                                                        IID_PPV_ARGS(&texture->default_gpu_mem));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    if (texture->cpu_access & (CPU_RESOURCE_ACCESS_WRITE | CPU_RESOURCE_ACCESS_READ))
    {
        device->GetCopyableFootprints(&texture->desc, 0, texture->desc.MipLevels, texture->first_subresource_offset, null, null, null, &texture->upload_and_readback_total_bytes);

        if (texture->cpu_access & CPU_RESOURCE_ACCESS_WRITE)
        {
            D3D12_HEAP_PROPERTIES heap_properties;
            heap_properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
            heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask     = 0;
            heap_properties.VisibleNodeMask      = 0;

            D3D12_RESOURCE_DESC resource_desc;
            resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            resource_desc.Width              = texture->upload_and_readback_total_bytes;
            resource_desc.Height             = 1;
            resource_desc.DepthOrArraySize   = 1;
            resource_desc.MipLevels          = 1;
            resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc.Count   = 1;
            resource_desc.SampleDesc.Quality = 0;
            resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
            {
                HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                                D3D12_HEAP_FLAG_NONE,
                                                                &resource_desc,
                                                                D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                null,
                                                                IID_PPV_ARGS(texture->upload_gpu_mem + i));
                REV_CHECK(CheckResultAndPrintMessages(error));

                D3D12_RANGE read_range = { 0, 0 };
                error = texture->upload_gpu_mem[i]->Map(0, &read_range, cast(void **, texture->upload_cpu_mem + i));
                REV_CHECK(CheckResultAndPrintMessages(error));
            }
        }

        if (texture->cpu_access & CPU_RESOURCE_ACCESS_READ)
        {
            D3D12_HEAP_PROPERTIES heap_properties;
            heap_properties.Type                 = D3D12_HEAP_TYPE_READBACK;
            heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask     = 0;
            heap_properties.VisibleNodeMask      = 0;

            D3D12_RESOURCE_DESC resource_desc;
            resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            resource_desc.Width              = texture->upload_and_readback_total_bytes;
            resource_desc.Height             = 1;
            resource_desc.DepthOrArraySize   = 1;
            resource_desc.MipLevels          = 1;
            resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
            resource_desc.SampleDesc.Count   = 1;
            resource_desc.SampleDesc.Quality = 0;
            resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
            {
                HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                                D3D12_HEAP_FLAG_NONE,
                                                                &resource_desc,
                                                                D3D12_RESOURCE_STATE_COPY_DEST,
                                                                null,
                                                                IID_PPV_ARGS(texture->readback_gpu_mem + i));
                REV_CHECK(CheckResultAndPrintMessages(error));

                D3D12_RANGE read_range = { 0, texture->upload_and_readback_total_bytes };
                error = texture->readback_gpu_mem[i]->Map(0, &read_range, cast(void **, texture->readback_cpu_mem + i));
                REV_CHECK(CheckResultAndPrintMessages(error));
            }
        }
    }

    SetTextureName(texture_memory, texture, name);

    return texture_memory->textures.Count() - 1;
}

void MemoryManager::UploadBufferData(ID3D12GraphicsCommandList *command_list, Buffer *buffer, bool _static, const void *data)
{
    REV_CHECK_M(buffer->cpu_access & CPU_RESOURCE_ACCESS_WRITE, "You are not able to upload data to \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());

    BufferMemory *buffer_memory = _static ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory;

    DefaultBufferMemoryPage *default_page = buffer_memory->default_pages.GetPointer(buffer->default_page_index);
    UploadBufferMemoryPage  *upload_page  = buffer_memory->upload_pages.GetPointer(buffer->upload_page_index);

    ID3D12Resource *upload_gpu_mem = upload_page->gpu_mem[m_DeviceContext->CurrentBuffer()];
    byte           *upload_cpu_mem = upload_page->cpu_mem[m_DeviceContext->CurrentBuffer()] + buffer->upload_page_offset;

    if (data)
    {
        CopyMemory(upload_cpu_mem, data, buffer->actual_size);

        D3D12_RESOURCE_BARRIER resource_barrier;
        resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        resource_barrier.Transition.pResource   = default_page->gpu_mem;
        resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        resource_barrier.Transition.StateBefore = default_page->initial_state;
        resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

        command_list->ResourceBarrier(1, &resource_barrier);

        command_list->CopyBufferRegion(default_page->gpu_mem, buffer->default_page_offset, upload_gpu_mem, buffer->upload_page_offset, buffer->actual_size);

        resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        resource_barrier.Transition.StateAfter  = default_page->initial_state;

        command_list->ResourceBarrier(1, &resource_barrier);
    }
    else
    {
        ZeroMemory(upload_cpu_mem, buffer->actual_size);
        command_list->DiscardResource(upload_gpu_mem, null);
        command_list->DiscardResource(default_page->gpu_mem, null);
    }
}

void MemoryManager::CopyDefaultBufferResourceToReadBackResource(ID3D12GraphicsCommandList *command_list, Buffer *buffer, bool _static)
{
    REV_CHECK_M(buffer->cpu_access & CPU_RESOURCE_ACCESS_READ, "You are not able to read data from \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());

    BufferMemory *buffer_memory = _static ? &m_StaticMemory.buffer_memory : &m_SceneMemory.buffer_memory;

    DefaultBufferMemoryPage  *default_page  = buffer_memory->default_pages.GetPointer(buffer->default_page_index);
    ReadBackBufferMemoryPage *readback_page = buffer_memory->readback_pages.GetPointer(buffer->readback_page_index);

    ID3D12Resource *readback_gpu_mem = readback_page->gpu_mem[m_DeviceContext->CurrentBuffer()];
    byte           *readback_cpu_mem = readback_page->cpu_mem[m_DeviceContext->CurrentBuffer()] + buffer->readback_page_offset;

    D3D12_RESOURCE_BARRIER resource_barrier;
    resource_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    resource_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    resource_barrier.Transition.pResource   = default_page->gpu_mem;
    resource_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    resource_barrier.Transition.StateBefore = default_page->initial_state;
    resource_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

    command_list->ResourceBarrier(1, &resource_barrier);

    command_list->CopyBufferRegion(readback_gpu_mem, buffer->readback_page_offset, default_page->gpu_mem, buffer->default_page_offset, buffer->actual_size);

    resource_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    resource_barrier.Transition.StateAfter  = default_page->initial_state;

    command_list->ResourceBarrier(1, &resource_barrier);
}

void MemoryManager::UploadTextureData(ID3D12GraphicsCommandList *command_list, Texture *texture, const GPU::TextureData *texture_data)
{
    REV_CHECK_M(texture->cpu_access & CPU_RESOURCE_ACCESS_WRITE, "You are not able to upload data to \"%.*s\" texture", texture->name.Length(), texture->name.Data());
    REV_CHECK(texture->planes_count == texture_data->planes_count);

    ID3D12Device *device = m_DeviceContext->Device();

    ID3D12Resource *upload_gpu_mem = texture->upload_gpu_mem[m_DeviceContext->CurrentBuffer()];
    byte           *upload_cpu_mem = texture->upload_cpu_mem[m_DeviceContext->CurrentBuffer()];

    if (texture_data->surfaces_count)
    {
        D3D12_TEXTURE_COPY_LOCATION dest_location;
        dest_location.pResource = texture->default_gpu_mem;
        dest_location.Type      = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        D3D12_TEXTURE_COPY_LOCATION source_location;
        source_location.pResource = upload_gpu_mem;
        source_location.Type      = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints = Memory::Get()->PushToFA<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(texture_data->surfaces_count);
        device->GetCopyableFootprints(&texture->desc, 0, cast(u32, texture_data->surfaces_count), texture->first_subresource_offset, footprints, null, null, null);

        const GPU::TextureSurface *surfaces_end = texture_data->surfaces + texture_data->surfaces_count;
        for (const GPU::TextureSurface *surface = texture_data->surfaces; surface < surfaces_end; ++surface)
        {
            byte *dest_surface = upload_cpu_mem + footprints->Offset;

            u64 num_rows = surface->size_in_bytes / surface->row_bytes;
            for (u64 row = 0; row < num_rows; ++row)
            {
                byte *dest_row = dest_surface  + footprints->Footprint.RowPitch * row;
                byte *src_row  = surface->data + surface->row_bytes             * row;

                CopyMemory(dest_row, src_row, surface->row_bytes);
            }

            dest_location.SubresourceIndex  = cast(u32, surface - texture_data->surfaces);
            source_location.PlacedFootprint = *footprints;

            command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, null);

            ++footprints;
        }
    }
    else
    {
        ZeroMemory(upload_cpu_mem, texture->upload_and_readback_total_bytes);
        command_list->DiscardResource(upload_gpu_mem, null);
        command_list->DiscardResource(texture->default_gpu_mem, null);
    }
}

void MemoryManager::CopyDefaultTextureResourceToReadBackResource(ID3D12GraphicsCommandList *command_list, Texture *texture)
{
    REV_CHECK_M(texture->cpu_access & CPU_RESOURCE_ACCESS_READ, "You are not able to read data from \"%.*s\" texture", texture->name.Length(), texture->name.Data());

    ID3D12Device *device = m_DeviceContext->Device();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints = Memory::Get()->PushToFA<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(texture->desc.MipLevels);
    device->GetCopyableFootprints(&texture->desc, 0, texture->desc.MipLevels, texture->first_subresource_offset, footprints, null, null, null);

    ID3D12Resource *readback_gpu_mem = texture->readback_gpu_mem[m_DeviceContext->CurrentBuffer()];
    byte           *readback_cpu_mem = texture->readback_cpu_mem[m_DeviceContext->CurrentBuffer()];

    D3D12_TEXTURE_COPY_LOCATION source_location;
    source_location.pResource = texture->default_gpu_mem;
    source_location.Type      = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    D3D12_TEXTURE_COPY_LOCATION dest_location;
    dest_location.pResource = readback_gpu_mem;
    dest_location.Type      = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    for (u32 i = 0; i < texture->desc.MipLevels; ++i)
    {
        source_location.SubresourceIndex = i;
        dest_location.PlacedFootprint    = footprints[i];

        command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, null);
    }
}

void MemoryManager::SetBufferName(BufferMemory *buffer_memory, Buffer *buffer, const ConstString& name)
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

    if (buffer->cpu_access & CPU_RESOURCE_ACCESS_WRITE)
    {
        UploadBufferMemoryPage *page = buffer_memory->upload_pages.GetPointer(buffer->upload_page_index);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            ID3D12Resource *resource = page->gpu_mem[i];

            UINT    gotten_wname_length = 0;
            HRESULT error               = resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &gotten_wname_length, null);

            if (gotten_wname_length)
            {
                gotten_wname_length = gotten_wname_length / sizeof(wchar_t) - 1;

                u32      new_wname_length = cast(u32, gotten_wname_length + 1 + name.Length());
                wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

                UINT data_size = (new_wname_length + 1) * sizeof(wchar_t);
                error = resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, new_wname);
                REV_CHECK(CheckResultAndPrintMessages(error));
                REV_CHECK(gotten_wname_length == data_size / sizeof(wchar_t) - 1);

                _snwprintf(new_wname + gotten_wname_length, new_wname_length - gotten_wname_length, L" %.*s", cast(u32, name.Length()), wname);

                error = resource->SetName(new_wname);
                REV_CHECK(CheckResultAndPrintMessages(error));
            }
            else
            {
                u32      new_wname_length = cast(u32, REV_CSTRLEN("UPLOAD ") + 2 + name.Length());
                wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

                _snwprintf(new_wname, new_wname_length, L"UPLOAD #%I32u %.*s", i, cast(u32, name.Length()), wname);

                error = resource->SetName(new_wname);
                REV_CHECK(CheckResultAndPrintMessages(error));
            }
        }
    }

    if (buffer->cpu_access & CPU_RESOURCE_ACCESS_READ)
    {
        ReadBackBufferMemoryPage *page = buffer_memory->readback_pages.GetPointer(buffer->readback_page_index);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            ID3D12Resource *resource = page->gpu_mem[i];

            UINT    gotten_wname_length = 0;
            HRESULT error               = resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &gotten_wname_length, null);

            if (gotten_wname_length)
            {
                gotten_wname_length = gotten_wname_length / sizeof(wchar_t) - 1;

                u32      new_wname_length = cast(u32, gotten_wname_length + 1 + name.Length());
                wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

                UINT data_size = (new_wname_length + 1) * sizeof(wchar_t);
                error = resource->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, new_wname);
                REV_CHECK(CheckResultAndPrintMessages(error));
                REV_CHECK(gotten_wname_length == data_size / sizeof(wchar_t) - 1);

                _snwprintf(new_wname + gotten_wname_length, new_wname_length - gotten_wname_length, L" %.*s", cast(u32, name.Length()), wname);

                error = resource->SetName(new_wname);
                REV_CHECK(CheckResultAndPrintMessages(error));
            }
            else
            {
                u32      new_wname_length = cast(u32, REV_CSTRLEN("READBACK ") + 2 + name.Length());
                wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

                _snwprintf(new_wname, new_wname_length, L"READBACK #%I32u %.*s", i, cast(u32, name.Length()), wname);

                error = resource->SetName(new_wname);
                REV_CHECK(CheckResultAndPrintMessages(error));
            }
        }
    }
}

void MemoryManager::SetTextureName(TextureMemory *texture_memory, Texture *texture, const ConstString& name)
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

    if (texture->cpu_access & CPU_RESOURCE_ACCESS_WRITE)
    {
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            _snwprintf(new_wname, new_wname_length, L"UPLOAD #%I32u %.*s", i, cast(u32, name.Length()), wname);

            HRESULT error = texture->upload_gpu_mem[i]->SetName(new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    if (texture->cpu_access & CPU_RESOURCE_ACCESS_READ)
    {
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            _snwprintf(new_wname, new_wname_length, L"READBACK #%I32u %.*s", i, cast(u32, name.Length()), wname);

            HRESULT error = texture->readback_gpu_mem[i]->SetName(new_wname);
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
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
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(page.gpu_mem[i]);
        }
    }
    memory->buffer_memory.upload_pages.Clear();

    for (ReadBackBufferMemoryPage& page : memory->buffer_memory.readback_pages)
    {
        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(page.gpu_mem[i]);
        }
    }
    memory->buffer_memory.readback_pages.Clear();

    for (Texture& texture : memory->texture_memory.textures)
    {
        SafeRelease(texture.default_gpu_mem);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(texture.upload_gpu_mem[i]);
            SafeRelease(texture.readback_gpu_mem[i]);
        }
    }
    memory->texture_memory.textures.Clear();

    memory->sampler_memory.samplers.Clear();
}

ConstArray<GPU::ResourceHandle> MemoryManager::GetReadWriteResources() const
{
    u64 read_write_resources_count = 0;
    for (const Buffer&  buffer  : m_SceneMemory.buffer_memory.buffers)    if (buffer.cpu_access  & CPU_RESOURCE_ACCESS_READ) ++read_write_resources_count;
    for (const Buffer&  buffer  : m_StaticMemory.buffer_memory.buffers)   if (buffer.cpu_access  & CPU_RESOURCE_ACCESS_READ) ++read_write_resources_count;
    for (const Texture& texture : m_SceneMemory.texture_memory.textures)  if (texture.cpu_access & CPU_RESOURCE_ACCESS_READ) ++read_write_resources_count;
    for (const Texture& texture : m_StaticMemory.texture_memory.textures) if (texture.cpu_access & CPU_RESOURCE_ACCESS_READ) ++read_write_resources_count;

    if (read_write_resources_count)
    {
        GPU::ResourceHandle *read_write_resources    = Memory::Get()->PushToFA<GPU::ResourceHandle>(read_write_resources_count);
        GPU::ResourceHandle *read_write_resources_it = read_write_resources;

        u64 index = 0;
        for (const Buffer& buffer : m_SceneMemory.buffer_memory.buffers)
        {
            if (buffer.cpu_access & CPU_RESOURCE_ACCESS_READ)
            {
                read_write_resources_it->index = index;
                read_write_resources_it->kind  = GPU::RESOURCE_KIND_READ_WRITE_BUFFER;
                read_write_resources_it++;
            }
            ++index;
        }

        index = 0;
        for (const Buffer& buffer : m_StaticMemory.buffer_memory.buffers)
        {
            if (buffer.cpu_access & CPU_RESOURCE_ACCESS_READ)
            {
                read_write_resources_it->index = index;
                read_write_resources_it->kind  = GPU::RESOURCE_KIND_READ_WRITE_BUFFER | GPU::RESOURCE_KIND_STATIC;
                read_write_resources_it++;
            }
            ++index;
        }

        index = 0;
        for (const Texture& texture : m_SceneMemory.texture_memory.textures)
        {
            if (texture.cpu_access & CPU_RESOURCE_ACCESS_READ)
            {
                read_write_resources_it->index = index;
                read_write_resources_it->kind  = GPU::RESOURCE_KIND_READ_WRITE_TEXTURE;
                read_write_resources_it++;
            }
            ++index;
        }

        index = 0;
        for (const Texture& texture : m_StaticMemory.texture_memory.textures)
        {
            if (texture.cpu_access & CPU_RESOURCE_ACCESS_READ)
            {
                read_write_resources_it->index = index;
                read_write_resources_it->kind  = GPU::RESOURCE_KIND_READ_WRITE_TEXTURE | GPU::RESOURCE_KIND_STATIC;
                read_write_resources_it++;
            }
            ++index;
        }

        return ConstArray(read_write_resources, read_write_resources_count);
    }

    return null;
}

};
