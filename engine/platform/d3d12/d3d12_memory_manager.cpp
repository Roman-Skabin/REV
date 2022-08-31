// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/graphics_api.h"
#include "memory/memory.h"
#include "core/settings.h"
#include "graphics/vertex_index_instance_buffers.h"

#include "platform/d3d12/d3d12_memory_manager.h"
#include "platform/d3d12/d3d12_common.h"

namespace REV::D3D12
{

//
// Internal
//

REV_INTERNAL REV_INLINE u16 GetMaxMipLevels(u16 width, u16 height = 0, u16 depth = 0)
{
    u32 count = 0;
    _BitScanReverse(&count, width | height | depth);
    return cast(u16, count + 1);
}

template<u64 buffer_page_prefix_size>
REV_INTERNAL REV_INLINE void AddBufferNameToBufferPage(
    ID3D12Resource *buffer_page,
    const wchar_t  (&buffer_page_prefix)[buffer_page_prefix_size],
    const wchar_t  *buffer_name,
    UINT            buffer_name_length)
{
    UINT    buffer_page_name_size = 0;
    HRESULT error                 = buffer_page->GetPrivateData(WKPDID_D3DDebugObjectNameW, &buffer_page_name_size, null);

    if (buffer_page_name_size)
    {
        UINT buffer_page_name_length = buffer_page_name_size / sizeof(wchar_t) - 1;

        UINT     new_buffer_page_name_length = buffer_page_name_length + 1 + buffer_name_length;
        wchar_t *new_buffer_page_name        = Memory::Get()->PushToFA<wchar_t>(new_buffer_page_name_length + 1);

        UINT data_size = (new_buffer_page_name_length + 1) * sizeof(wchar_t);
        error = buffer_page->GetPrivateData(WKPDID_D3DDebugObjectNameW, &data_size, new_buffer_page_name);
        REV_CHECK(CheckResultAndPrintMessages(error));
        REV_CHECK(buffer_page_name_size == data_size / sizeof(wchar_t) - 1);

        new_buffer_page_name[buffer_page_name_length] = L' ';
        CopyMemory(new_buffer_page_name + buffer_page_name_length + 1, buffer_name, buffer_name_length * sizeof(wchar_t));

        error = buffer_page->SetName(new_buffer_page_name);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }
    else
    {
        constexpr u64 buffer_page_prefix_length = buffer_page_prefix_size / sizeof(wchar_t) - 1;

        u32      new_buffer_page_name_length = cast(u32, buffer_page_prefix_length + buffer_name_length);
        wchar_t *new_buffer_page_name        = Memory::Get()->PushToFA<wchar_t>(new_buffer_page_name_length + 1);

        CopyMemory(new_buffer_page_name, buffer_page_prefix, buffer_page_prefix_length);
        CopyMemory(new_buffer_page_name + buffer_page_prefix_length, buffer_name, buffer_name_length * sizeof(wchar_t));

        error = buffer_page->SetName(new_buffer_page_name);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }
}

//
// MemoryManager
//

MemoryManager::MemoryManager(Allocator *allocator)
    : m_Allocator(allocator),
      m_DeviceContext(cast(DeviceContext *, GraphicsAPI::GetDeviceContext())),
      m_PerFrameMemory(allocator),
      m_PerSceneMemory(allocator),
      m_PermanentMemory(allocator)
{
}

MemoryManager::~MemoryManager()
{
    FreeMemory(&m_PerFrameMemory);
    FreeMemory(&m_PerSceneMemory);
    FreeMemory(&m_PermanentMemory);
}

const ResourceHandle& MemoryManager::AllocateBuffer(const BufferDesc& desc, const ConstString& name)
{
    ID3D12Device4 *device = m_DeviceContext->Device();

    ResourceMemory *resource_memory = GetResourceMemory(desc.flags, name);
    REV_CHECK(resource_memory);

    Buffer *buffer = new (resource_memory->buffer_memory.buffers.PushBack()) Buffer();
    buffer->flags       = desc.flags;
    buffer->handle.ptr  = buffer;
    buffer->handle.kind = RESOURCE_KIND_BUFFER;
    buffer->name        = name;

    if (buffer->flags & (RESOURCE_FLAG_VERTEX_BUFFER | RESOURCE_FLAG_INDEX_BUFFER))
    {
        buffer->size   = desc.count * desc.stride;
        buffer->stride = desc.stride;
    }
    else
    {
        buffer->size = desc.size;
    }

    // @TODO(Roman): Not all buffers require 256 bytes alignment
    u64                   aligned_size  = AlignUp(buffer->size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    D3D12_RESOURCE_STATES default_state = GetResourceDefaultState(RESOURCE_KIND_BUFFER, buffer->flags);

    {
        for (DefaultBufferMemoryPage& page : resource_memory->buffer_memory.default_pages)
        {
            if (page.occupied_bytes <= DEFAULT_BUFFER_MEMORY_PAGE_SIZE - aligned_size
            &&  page.default_state  == default_state)
            {
                buffer->default_page        = &page;
                buffer->default_page_offset = page.occupied_bytes;

                page.occupied_bytes += aligned_size;
                break;
            }
        }

        if (!buffer->default_page)
        {
            DefaultBufferMemoryPage *page = new (resource_memory->buffer_memory.default_pages.PushBack()) DefaultBufferMemoryPage();
            page->occupied_bytes = aligned_size;
            page->default_state  = default_state;

            buffer->default_page        = page;
            buffer->default_page_offset = 0;

            D3D12_HEAP_PROPERTIES heap_properties;
            heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
            heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask     = 0;
            heap_properties.VisibleNodeMask      = 0;

            // @TODO(Roman): Small resources alignment: D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT
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
            resource_desc.Flags              = GetResourceFlags(RESOURCE_KIND_BUFFER, buffer->flags);

            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            page->default_state,
                                                            null,
                                                            IID_PPV_ARGS(&page->gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    if (buffer->flags & (RESOURCE_FLAG_CPU_WRITE | RESOURCE_FLAG_CPU_WRITE_ONCE))
    {
        bool upload_once_page = flags & RESOURCE_FLAG_CPU_WRITE_ONCE;

        for (UploadBufferMemoryPage& page : resource_memory->buffer_memory.upload_pages)
        {
            if (page.occupied_bytes <= DEFAULT_BUFFER_MEMORY_PAGE_SIZE - aligned_size
            &&  page.upload_once    == upload_once_page)
            {
                buffer->upload_page        = &page;
                buffer->upload_page_offset = page.occupied_bytes;

                page.occupied_bytes += buffer->aligned_size;
                break;
            }
        }

        if (!buffer->upload_page)
        {
            UploadBufferMemoryPage *page = new (resource_memory->buffer_memory.upload_pages.PushBack()) UploadBufferMemoryPage();
            page->occupied_bytes         = buffer->aligned_size;
            page->upload_once            = upload_once_page;

            buffer->upload_page        = page;
            buffer->upload_page_offset = 0;

            D3D12_HEAP_PROPERTIES heap_properties;
            heap_properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
            heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask     = 0;
            heap_properties.VisibleNodeMask      = 0;

            // @TODO(Roman): Small resources alignment: D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT
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
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

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

    if (buffer->flags & (RESOURCE_FLAG_CPU_READ | RESOURCE_FLAG_CPU_READ_ONCE))
    {
        bool readback_once_page = flags & RESOURCE_FLAG_CPU_READ_ONCE;

        for (ReadBackBufferMemoryPage& page : resource_memory->buffer_memory.readback_pages)
        {
            if (page.occupied_bytes <= DEFAULT_BUFFER_MEMORY_PAGE_SIZE - buffer->aligned_size
            &&  page.readback_once  == readback_once_page)
            {
                buffer->readback_page        = &page;
                buffer->readback_page_offset = page.occupied_bytes;

                page.occupied_bytes += buffer->aligned_size;
                break;
            }
        }

        if (!buffer->readback_page)
        {
            ReadBackBufferMemoryPage *page = new (resource_memory->buffer_memory.readback_pages.PushBack()) ReadBackBufferMemoryPage();
            page->occupied_bytes           = buffer->aligned_size;
            page->readback_once            = readback_once_page;

            buffer->readback_page        = page;
            buffer->readback_page_offset = 0;

            D3D12_HEAP_PROPERTIES heap_properties;
            heap_properties.Type                 = D3D12_HEAP_TYPE_READBACK;
            heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_properties.CreationNodeMask     = 0;
            heap_properties.VisibleNodeMask      = 0;

            // @TODO(Roman): Small resources alignment: D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT
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
            resource_desc.Flags              = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

            HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &resource_desc,
                                                            D3D12_RESOURCE_STATE_COPY_DEST,
                                                            null,
                                                            IID_PPV_ARGS(&page->gpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));

            D3D12_RANGE read_range = { 0, 0 };
            error = page->gpu_mem->Map(0, &read_range, cast(void **, &page->cpu_mem));
            REV_CHECK(CheckResultAndPrintMessages(error));
        }
    }

    SetBufferName(buffer);
    return buffer->handle;
}

const ResourceHandle& MemoryManager::AllocateTexture(TextureDesc& desc, const ConstString& name)
{
    {
        desc.width      = Math::max(1, desc.width);
        desc.height     = Math::max(1, desc.height);
        desc.depth      = Math::max(1, desc.depth);
        desc.array_size = Math::max(1, desc.array_size);
        desc.mip_levels = Math::max(1, desc.mip_levels);
    }

    D3D12_RESOURCE_DIMENSION d3d12_resource_dimension  = D3D12_RESOURCE_DIMENSION_UNKNOWN;
    u16                      d3d12_depth_or_array_size = 1;

    switch (desc.dimension)
    {
        case TEXTURE_DIMENSION_1D:
        {
            REV_CHECK_M(desc.width <= D3D12_REQ_TEXTURE1D_U_DIMENSION, "Width should be = [1, %hu].", D3D12_REQ_TEXTURE1D_U_DIMENSION);

            d3d12_resource_dimension  = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            d3d12_depth_or_array_size = 1;
        } break;

        case TEXTURE_DIMENSION_2D:
        {
            REV_CHECK_M(desc.width  <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Width should be = [1, %hu].",  D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            REV_CHECK_M(desc.height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, "Height should be = [1, %hu].", D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

            d3d12_resource_dimension  = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d3d12_depth_or_array_size = 1;
        } break;

        case TEXTURE_DIMENSION_3D:
        {
            REV_CHECK_M(desc.width  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Width should be = [1, %hu].",  D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            REV_CHECK_M(desc.height <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Height should be = [1, %hu].", D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
            REV_CHECK_M(desc.depth  <= D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION, "Depth should be = [1, %hu].",  D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);

            d3d12_resource_dimension  = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            d3d12_depth_or_array_size = desc.depth;
        } break;

        case TEXTURE_DIMENSION_CUBE:
        {
            REV_CHECK_M(desc.width  <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Width should be = [1, %hu].",  D3D12_REQ_TEXTURECUBE_DIMENSION);
            REV_CHECK_M(desc.height <= D3D12_REQ_TEXTURECUBE_DIMENSION, "Height should be = [1, %hu].", D3D12_REQ_TEXTURECUBE_DIMENSION);

            d3d12_resource_dimension  = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d3d12_depth_or_array_size = 6;
        } break;

        case TEXTURE_DIMENSION_1D_ARRAY:
        {
            REV_CHECK_M(desc.width      <= D3D12_REQ_TEXTURE1D_U_DIMENSION,          "Width should be = [1, %hu].",      D3D12_REQ_TEXTURE1D_U_DIMENSION);
            REV_CHECK_M(desc.array_size <= D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION, "Array size should be = [1, %hu].", D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION);

            d3d12_resource_dimension  = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            d3d12_depth_or_array_size = desc.array_size;
        } break;

        case TEXTURE_DIMENSION_2D_ARRAY:
        {
            REV_CHECK_M(desc.width      <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,     "Width should be = [1, %hu].",      D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            REV_CHECK_M(desc.height     <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,     "Height should be = [1, %hu].",     D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
            REV_CHECK_M(desc.array_size <= D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION, "Array size should be = [1, %hu].", D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);

            d3d12_resource_dimension  = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d3d12_depth_or_array_size = desc.array_size;
        } break;

        case TEXTURE_DIMENSION_CUBE_ARRAY:
        {
            REV_CHECK_M(desc.width      <= D3D12_REQ_TEXTURECUBE_DIMENSION,            "Width should be = [1, %hu].",      D3D12_REQ_TEXTURECUBE_DIMENSION);
            REV_CHECK_M(desc.height     <= D3D12_REQ_TEXTURECUBE_DIMENSION,            "Height should be = [1, %hu].",     D3D12_REQ_TEXTURECUBE_DIMENSION);
            REV_CHECK_M(desc.array_size <= D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION, "Array size should be = [1, %hu].", D3D12_REQ_TEXTURECUBE_ARRAY_AXIS_DIMENSION);

            d3d12_resource_dimension  = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d3d12_depth_or_array_size = 6 * desc.array_size;
        } break;

        default:
        {
            REV_ERROR_M("Invalid or unhandled texture dimension: %hhu", desc.dimension);
        } break;
    }

    {
        u16 max_mip_levels = GetMaxMipLevels(desc.width, desc.height, desc.depth);
        if (desc.mip_levels > max_mip_levels)
        {
            REV_WARNING_M("Number of MIP levels is to high: %hu. It has been changed to %hu.", desc.mip_levels, max_mip_levels);
            desc.mip_levels = max_mip_levels;
        }
        if (desc.mip_levels > D3D12_REQ_MIP_LEVELS)
        {
            REV_WARNING_M("DirectX 12 does not support that many MIP levels (%hu). Only %hu of them will be used", desc.mip_levels, D3D12_REQ_MIP_LEVELS);
            desc.mip_levels = D3D12_REQ_MIP_LEVELS;
        }
    }

    DXGI_FORMAT dxgi_format  = REVToDXGITextureFormat(desc.format);
    u8          planes_count = m_DeviceContext->GetFormatPlanesCount(dxgi_format);

    u32 d3d12_array_size = desc.array_size;
    if (desc.dimension == TEXTURE_DIMENSION_CUBE || desc.dimension == TEXTURE_DIMENSION_CUBE_ARRAY)
        d3d12_array_size *= 6;

    u32 surfaces_count = planes_count * d3d12_array_size * desc.mip_levels;
    REV_CHECK_M(surfaces_count <= D3D12_REQ_SUBRESOURCES, "Texture \"%.*s\" is too big for DirectX 12", name.Length(), name.Data());

    ID3D12Device4 *device = m_DeviceContext->Device();

    ResourceMemory *resource_memory = GetResourceMemory(desc.flags, name);
    REV_CHECK(resource_memory);

    Texture *texture = new (resource_memory->texture_memory.textures.PushBack()) Texture();
    texture->format        = dxgi_format;
    texture->default_state = GetResourceDefaultState(RESOURCE_KIND_TEXTURE, desc.flags);
    texture->flags         = desc.flags;
    texture->depth         = desc.depth;
    texture->mip_levels    = desc.mip_levels;
    texture->array_size    = desc.array_size;
    texture->planes_count  = planes_count;
    texture->dimension     = desc.dimension;
    texture->handle.ptr    = texture;
    texture->handle.kind   = RESOURCE_KIND_TEXTURE;
    texture->name          = name;

    // @TODO(Roman): MSAA alignment: D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
    // @TODO(Roman): Small resources alignment: D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
    D3D12_RESOURCE_DESC default_resource_desc;
    default_resource_desc.Dimension          = d3d12_resource_dimension;
    default_resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    default_resource_desc.Width              = desc.width;
    default_resource_desc.Height             = desc.height;
    default_resource_desc.DepthOrArraySize   = d3d12_depth_or_array_size;
    default_resource_desc.MipLevels          = desc.mip_levels;
    default_resource_desc.Format             = dxgi_format;
    default_resource_desc.SampleDesc.Count   = 1;
    default_resource_desc.SampleDesc.Quality = 0;
    default_resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    default_resource_desc.Flags              = GetResourceFlags(RESOURCE_KIND_TEXTURE, desc.flags);

    {
        D3D12_RESOURCE_ALLOCATION_INFO1 info1{};
        device->GetResourceAllocationInfo1(0, 1, &default_resource_desc, &info1);

        texture->first_surface_offset = info1.Offset;

        device->GetCopyableFootprints(&default_resource_desc, 0, 1, surfaces_count, texture->first_surface_offset, null, null null, &texture->buffer_total_bytes);
    }

    // @NOTE(Roman): Default
    {
        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask     = 0;
        heap_properties.VisibleNodeMask      = 0;

        D3D12_CLEAR_VALUE  optimized_clear_value     = GetOptimizedClearValue(texture->flags, desc.format);
        D3D12_CLEAR_VALUE *optimized_clear_value_ptr = optimized_clear_value.Format == DXGI_FORMAT_UNKNOWN ? null : &optimized_clear_value;

        HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &default_resource_desc,
                                                        texture->default_state,
                                                        optimized_clear_value_ptr,
                                                        IID_PPV_ARGS(&texture->default_gpu_mem));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    // @NOTE(Roman): Upload
    if (texture->flags & (RESOURCE_FLAG_CPU_WRITE | RESOURCE_FLAG_CPU_WRITE_ONCE))
    {
        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
        heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask     = 0;
        heap_properties.VisibleNodeMask      = 0;

        // @TODO(Roman): MSAA alignment: D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
        // @TODO(Roman): Small resources alignment: D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
        D3D12_RESOURCE_DESC upload_resource_desc;
        upload_resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        upload_resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        upload_resource_desc.Width              = texture->buffer_total_bytes;
        upload_resource_desc.Height             = 1;
        upload_resource_desc.DepthOrArraySize   = 1;
        upload_resource_desc.MipLevels          = 1;
        upload_resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
        upload_resource_desc.SampleDesc.Count   = 1;
        upload_resource_desc.SampleDesc.Quality = 0;
        upload_resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        upload_resource_desc.Flags              = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &upload_resource_desc,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        null,
                                                        IID_PPV_ARGS(&texture->upload_gpu_mem));
        REV_CHECK(CheckResultAndPrintMessages(error));

        D3D12_RANGE read_range = { 0, 0 };
        error = texture->upload_gpu_mem->Map(0, &read_range, cast(void **, &texture->upload_cpu_mem));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    // @NOTE(Roman): Readback
    if (texture->flags & (RESOURCE_FLAG_CPU_READ | RESOURCE_FLAG_CPU_READ_ONCE))
    {
        D3D12_HEAP_PROPERTIES heap_properties;
        heap_properties.Type                 = D3D12_HEAP_TYPE_READBACK;
        heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_properties.CreationNodeMask     = 0;
        heap_properties.VisibleNodeMask      = 0;

        // @TODO(Roman): MSAA alignment: D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
        // @TODO(Roman): Small resources alignment: D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT, D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
        D3D12_RESOURCE_DESC readback_resource_desc;
        readback_resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        readback_resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        readback_resource_desc.Width              = texture->buffer_total_bytes;
        readback_resource_desc.Height             = 1;
        readback_resource_desc.DepthOrArraySize   = 1;
        readback_resource_desc.MipLevels          = 1;
        readback_resource_desc.Format             = DXGI_FORMAT_UNKNOWN;
        readback_resource_desc.SampleDesc.Count   = 1;
        readback_resource_desc.SampleDesc.Quality = 0;
        readback_resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        readback_resource_desc.Flags              = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        HRESULT error = device->CreateCommittedResource(&heap_properties,
                                                        D3D12_HEAP_FLAG_NONE,
                                                        &readback_resource_desc,
                                                        D3D12_RESOURCE_STATE_COPY_DEST,
                                                        null,
                                                        IID_PPV_ARGS(&texture->readback_gpu_mem));
        REV_CHECK(CheckResultAndPrintMessages(error));

        D3D12_RANGE read_range = { 0, 0 };
        error = texture->readback_gpu_mem->Map(0, &read_range, cast(void **, &texture->readback_cpu_mem));
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    SetTextureName(texture);
    return texture->handle;
}

const ResourceHandle& MemoryManager::AllocateSampler(const SamplerDesc& desc, const ConstString& name = null)
{
    ResourceMemory *resource_memory = GetResourceMemory(desc.flags, name);
    REV_CHECK(resource_memory);

    D3D12_TEXTURE_ADDRESS_MODE d3d12_address_mode = RevToD3D12SamplerAddressMode(desc.adress_mode);
    u32                        d3d12_anisotrophy  = 0;
    D3D12_FILTER               d3d12_filter       = RevToD3D12SamplerFilter(desc.filter, anisotrophy);

    if (d3d12_anisotrophy > D3D12_REQ_MAXANISOTROPY)
    {
        REV_WARNING_M("DirectX 12 does not support that large anisotropy (%I32u). Max available will be used (%I32u)", d3d12_anisotrophy, D3D12_REQ_MAXANISOTROPY);
        d3d12_anisotrophy = D3D12_REQ_MAXANISOTROPY;
    }

    // @TODO(Roman): Find sampler by hash. We do not want to create sampler each time.
    //               So we can associate it with a hash and reuse one.
    //               We have to hash some flags as well because we do not want to reuse
    //               per-frame sampler for per-scene sampler for example.
    Sampler *sampler = new (resource_memory->sampler_memory.samplers.PushBack()) Sampler();
    sampler->desc.Filter         = d3d12_filter;
    sampler->desc.AddressU       = d3d12_address_mode;
    sampler->desc.AddressV       = d3d12_address_mode;
    sampler->desc.AddressW       = d3d12_address_mode;
    sampler->desc.MipLODBias     = D3D12_DEFAULT_MIP_LOD_BIAS;
    sampler->desc.MaxAnisotropy  = d3d12_anisotrophy;
    sampler->desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler->desc.BorderColor[0] = 0.0f;
    sampler->desc.BorderColor[1] = 0.0f;
    sampler->desc.BorderColor[2] = 0.0f;
    sampler->desc.BorderColor[3] = 0.0f;
    sampler->desc.MinLOD         = desc.min_max_lod.x;
    sampler->desc.MaxLOD         = desc.min_max_lod.y;
    sampler->flags               = desc.flags;
    sampler->handle.ptr          = sampler;
    sampler->handle.kind         = RESOURCE_KIND_SAMPLER;
    sampler->name                = name;

    return sampler->handle;
}

void MemoryManager::UpdateBuffer(const ResourceHandle& resource, const void *data)
{
    Buffer *buffer = GetBuffer(resource);

    REV_CHECK_M(buffer->flags & (RESOURCE_FLAG_CPU_WRITE | RESOURCE_FLAG_CPU_WRITE_ONCE),
                "You are not able to write data to \"%.*s\" buffer. "
                "Use RESOURCE_FLAG_CPU_WRITE or RESOURCE_FLAG_CPU_WRITE_ONCE for updating this buffer",
                buffer->name.Length(), buffer->name.Data());

    REV_CHECK_M(buffer->upload_page && buffer->upload_page->cpu_mem && buffer->upload_page_offset != REV_INVALID_U64_OFFSET,
                "It seems like buffer \"%.*s\" has flag RESOURCE_FLAG_CPU_WRITE_ONCE and is already been updated, "
                "so you are not able to update it anymore",
                buffer->name.Length(), buffer->name.Data());

    if (data) CopyMemory(buffer->upload_page->cpu_mem + buffer->upload_page_offset, data, buffer->size);
    else      ZeroMemory(buffer->upload_page->cpu_mem + buffer->upload_page_offset,       buffer->size);
}

void MemoryManager::UpdateTexture(const ResourceHandle& resource, const TextureData& data)
{
    Texture *texture = GetTexture(resource);

    REV_CHECK_M(texture->flags & (RESOURCE_FLAG_CPU_WRITE | RESOURCE_FLAG_CPU_WRITE_ONCE),
                "You are not able to write data to \"%.*s\" texture. "
                "Use RESOURCE_FLAG_CPU_WRITE or RESOURCE_FLAG_CPU_WRITE_ONCE for updating this texture",
                texture->name.Length(), texture->name.Data());

    REV_CHECK_M(texture->upload_gpu_mem && texture->upload_cpu_mem,
                "It seems like texture \"%.*s\" has flag RESOURCE_FLAG_CPU_WRITE_ONCE and is already been updated, "
                "so you are not able to update it anymore",
                texture->name.Length(), texture->name.Data());

    u16 data_surfaces_count = data.SurfacesCount();
    if (data_surfaces_count)
    {
        u32 d3d12_array_size = texture.array_size;
        if (texture.dimension == TEXTURE_DIMENSION_CUBE || texture.dimension == TEXTURE_DIMENSION_CUBE_ARRAY)
            d3d12_array_size *= 6;

        u16 resource_surfaces_count = texture.planes_count * d3d12_array_size * texture.mip_levels;
        REV_CHECK_M(data_surfaces_count == resource_surfaces_count, "Only entrie textures updates are currently supported");

        ID3D12Device4 *device = m_DeviceContext->Device();

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints = Memory::Get()->PushToFA<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(data_surfaces_count);
        device->GetCopyableFootprints(&texture->default_gpu_mem->GetDesc(), 0, data_surfaces_count, texture.first_surface_offset, footprints, null, null, null);

        data.ForEachSurface([&texture, &footprints](const TextureSurface *surface, u16 mip_level, u16 subtexture, u16 plane) -> bool
        {
            byte *dest_surface = texture.upload_cpu_mem + footprints->Offset;

            u64 num_rows = surface->size_in_bytes / surface->row_bytes;
            for (u64 row = 0; row < num_rows; ++row)
            {
                byte *dest_row = dest_surface  + footprints->Footprint.RowPitch * row;
                byte *src_row  = surface->data + surface->row_bytes             * row;

                CopyMemory(dest_row, src_row, surface->row_bytes);
            }

            ++footprints;

            return false;
        });
    }
    else
    {
        ZeroMemory(texture->upload_cpu_mem, texture->buffer_total_bytes);
    }
}

void *MemoryManager::ReadBuffer(const ResourceHandle& resource)
{
    Buffer *buffer = GetBuffer(resource);

    REV_CHECK_M(buffer->flags & (RESOURCE_FLAG_CPU_READ | RESOURCE_FLAG_CPU_READ_ONCE),
                "You are not able to read data from \"%.*s\" buffer. "
                "Use RESOURCE_FLAG_CPU_READ or RESOURCE_FLAG_CPU_READ_ONCE to read data from this buffer",
                buffer->name.Length(), buffer->name.Data());

    REV_CHECK_M(buffer->readback_page && buffer->readback_page->cpu_mem && buffer->readback_page_offset != REV_INVALID_U64_OFFSET,
                "It seems like buffer \"%.*s\" has flag RESOURCE_FLAG_CPU_READ_ONCE and is already been read, "
                "so you are not able to read it anymore",
                buffer->name.Length(), buffer->name.Data());

    byte *buffer_data = buffer->readback_page->cpu_mem + buffer->readback_page_offset;

    if (buffer->flags & RESOURCE_FLAG_CPU_READ_ONCE)
    {
        REV_CHECK_M(buffer->readback_page->readback_once,
                    "Internal error: buffer \"%.*s\" has flag RESOURCE_FLAG_CPU_READ_ONCE, but its readback page has not",
                    buffer->name.Length(), buffer->name.Data());

        REV_CHECK(buffer->readback_page->readback_once_resources_left_count > 0);
        --buffer->readback_page->readback_once_resources_left_count;

        if (!buffer->readback_page->readback_once_resources_left_count)
        {
            SafeRelease(buffer->readback_page->gpu_mem);
            buffer->readback_page->cpu_mem = null;
        }

        buffer->readback_page        = null;
        buffer->readback_page_offset = REV_INVALID_U64_OFFSET;
    }

    return buffer_data;
}

TextureData MemoryManager::ReadTexture(const ResourceHandle& resource)
{
    Texture *texture = GetTexture(resource);

    REV_CHECK_M(texture->flags & (RESOURCE_FLAG_CPU_READ | RESOURCE_FLAG_CPU_READ_ONCE),
                "You are not able to read data from \"%.*s\" texture. "
                "Use RESOURCE_FLAG_CPU_READ or RESOURCE_FLAG_CPU_READ_ONCE to reade data from this texture",
                texture->name.Length(), texture->name.Data());

    REV_CHECK_M(texture->readback_gpu_mem && texture->readback_cpu_mem,
                "It seems like texture \"%.*s\" has flag RESOURCE_FLAG_CPU_READ_ONCE and is already been read, "
                "so you are not able to read it anymore",
                texture->name.Length(), texture->name.Data());

    ID3D12Device4 *device = m_DeviceContext->Device();
    Memory        *memory = Memory::Get();

    u64 surfaces_count = texture->planes_count * texture->array_size * texture->mip_levels;
    if (texture->dimension == TEXTURE_DIMENSION_CUBE || texture->dimension == TEXTURE_DIMENSION_CUBE_ARRAY)
        surfaces_count *= 6;

    byte *footprints_memory = memory->PushToFA<byte>(surfaces_count * (sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(u32) + sizeof(u64)));

    D3D12_RESOURCE_DESC texture_desc = texture->default_gpu_mem->GetDesc();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints         = cast(D3D12_PLACED_SUBRESOURCE_FOOTPRINT *, footprints_memory);
    u32                                *unpadded_num_rows  = cast(u32 *, footprints        + surfaces_count);
    u64                                *unpadded_row_bytes = cast(u64 *, unpadded_num_rows + surfaces_count);
    device->GetCopyableFootprints(&texture_desc, 0, cast(u32, surfaces_count), texture->first_subresource_offset, footprints, cast(UINT *, unpadded_num_rows), unpadded_row_bytes, null);

    TextureData texture_data;
    texture_data.InitFromNonLinearData(surfaces_count, [texture, &texture_desc, &footprints, &unpadded_num_rows, &unpadded_row_bytes](TextureDataHeader *header)
    {
        header->dimension    = texture->dimension;
        header->format       = DXGIToREVTextureFormat(texture->format);
        header->width        = texture_desc.Width;
        header->height       = texture_desc.Height;
        header->depth        = texture->depth;
        header->mip_levels   = texture->mip_levels;
        header->array_size   = texture->array_size;
        header->planes_count = texture->planes_count;

        TextureSurface *surfaces_end = header->surfaces + header->surfaces_count;
        for (TextureSurface *surface = header->surfaces; surface < surfaces_end; ++surface)
        {
            surface->data          = texture->readback_cpu_mem + footprints->Offset;
            surface->row_bytes     = (*unpadded_row_bytes);
            surface->size_in_bytes = (*unpadded_row_bytes) * (*unpadded_num_rows);

            ++footprints;
            ++unpadded_num_rows;
            ++unpadded_row_bytes;
        }
    });

    if (texture->flags & RESOURCE_FLAG_CPU_READ_ONCE)
    {
        SafeRelease(texture->readback_gpu_mem);
        texture->readback_cpu_mem = null;
    }

    return texture_data;
}

void MemoryManager::UploadResources(const ConstArray<ResourceHandle>& resources)
{
    if (resources.Empty())
    {
        return;
    }

    D3D12_RESOURCE_BARRIER *barriers    = Memory::Get()->PushToFA<D3D12_RESOURCE_BARRIER>(2 * resources.Count());
    D3D12_RESOURCE_BARRIER *barriers_it = barriers;

    for (const ResourceHandle& resource : resources)
    {
        if (resource.kind == RESOURCE_KIND_BUFFER)
        {
            Buffer *buffer = GetBuffer(resource);
            REV_CHECK_M(buffer->flags & (RESOURCE_FLAG_CPU_WRITE | RESOURCE_FLAG_CPU_WRITE_ONCE),
                        "You are not able to upload data to \"%.*s\" buffer",
                        buffer->name.Length(), buffer->name.Data());

            REV_CHECK_M(buffer->flags & (RESOURCE_FLAG_CPU_WRITE | RESOURCE_FLAG_CPU_WRITE_ONCE),
                        "You are not able to write data to \"%.*s\" buffer. "
                        "Use RESOURCE_FLAG_CPU_WRITE or RESOURCE_FLAG_CPU_WRITE_ONCE for updating this buffer",
                        buffer->name.Length(), buffer->name.Data());

            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.pResource   = buffer->default_page->gpu_mem;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.StateBefore = buffer->default_page->default_state;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

            ++barriers_it;

            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.pResource   = buffer->upload_page->gpu_mem;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

            ++barriers_it;
        }
        else
        {
            Texture *texture = GetTexture(resource);
            REV_CHECK_M(texture->flags & (RESOURCE_FLAG_CPU_WRITE | RESOURCE_FLAG_CPU_WRITE_ONCE),
                        "You are not able to write data to \"%.*s\" texture. "
                        "Use RESOURCE_FLAG_CPU_WRITE or RESOURCE_FLAG_CPU_WRITE_ONCE for updating this texture",
                        texture->name.Length(), texture->name.Data());

            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.pResource   = texture->default_gpu_mem;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.StateBefore = texture->default_state;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

            ++barriers_it;

            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.pResource   = texture->upload_gpu_mem;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

            ++barriers_it;
        }
    }

    ID3D12GraphicsCommandList *command_list = m_DeviceContext->CurrentGraphicsList();
    command_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);

    barriers_it = barriers;
    for (const ResourceHandle& resource : resources)
    {
        if (resource.kind == RESOURCE_KIND_BUFFER)
        {
            Buffer *buffer = GetBuffer(resource);

            command_list->CopyBufferRegion(buffer->default_page->gpu_mem, buffer->default_page_offset,
                                           buffer->upload_page->gpu_mem, buffer->upload_page_offset,
                                           buffer->actual_size);

            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers_it->Transition.StateAfter  = buffer->default_page->default_state;

            ++barriers_it;

            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_GENERIC_READ;

            ++barriers_it;

            if (buffer->flags & RESOURCE_FLAG_CPU_WRITE_ONCE)
            {
                REV_CHECK_M(buffer->upload_page->upload_once,
                            "Internal error: buffer \"%.*s\" has flag RESOURCE_FLAG_CPU_WRITE_ONCE, but its upload page has not",
                            buffer->name.Length(), buffer->name.Data());

                REV_CHECK(buffer->upload_page->upload_once_resources_left_count > 0);
                --buffer->upload_page->upload_once_resources_left_count;

                if (!buffer->upload_page->upload_once_resources_left_count)
                {
                    SafeRelease(buffer->upload_page->gpu_mem);
                    buffer->upload_page->cpu_mem = null;
                }

                buffer->upload_page        = null;
                buffer->upload_page_offset = REV_INVALID_U64_OFFSET;
            }
        }
        else
        {
            Texture *texture = GetTexture(resource);

            ID3D12Device4 *device = m_DeviceContext->Device();

            u32 surfaces_count = texture->planes_count * texture->array_size * texture->mip_levels;
            if (texture->dimension == TEXTURE_DIMENSION_CUBE || texture->dimension == TEXTURE_DIMENSION_CUBE_ARRAY)
                surfaces_count *= 6;

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints = Memory::Get()->PushToFA<D3D12_PLACED_SUBRESOURCE_FOOTPRINT>(subresources_count);
            device->GetCopyableFootprints(&texture->default_gpu_mem->GetDesc(), 0, surfaces_count, texture->first_surface_offset, footprints, null, null, null);

            D3D12_TEXTURE_COPY_LOCATION source_location;
            source_location.pResource = texture->upload_gpu_mem;
            source_location.Type      = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

            D3D12_TEXTURE_COPY_LOCATION dest_location;
            dest_location.pResource = texture->default_gpu_mem;
            dest_location.Type      = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

            for (u32 i = 0; i < surfaces_count; ++i)
            {
                source_location.PlacedFootprint = footprints[i];
                dest_location.SubresourceIndex  = i;

                command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, null);
            }

            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers_it->Transition.StateAfter  = texture->default_state;

            ++barriers_it;

            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_GENERIC_READ;

            ++barriers_it;

            if (texture->flags & RESOURCE_FLAG_CPU_WRITE_ONCE)
            {
                SafeRelease(texture->upload_gpu_mem);
                texture->upload_cpu_mem = null;
            }
        }
    }

    command_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);
}

void MemoryManager::ReadbackResources(const ConstArray<ResourceHandle>& resources)
{
    if (resources.Empty())
    {
        return;
    }

    D3D12_RESOURCE_BARRIER *barriers    = Memory::Get()->PushToFA<D3D12_RESOURCE_BARRIER>(resources.Count());
    D3D12_RESOURCE_BARRIER *barriers_it = barriers;

    for (const ResourceHandle& resource : resources)
    {
        if (resource.kind == RESOURCE_KIND_BUFFER)
        {
            BufferMemory *buffer_memory = resource.flags & RESOURCE_FLAG_STATIC
                                        ? &m_StaticMemory.buffer_memory
                                        : &m_SceneMemory.buffer_memory;

            Buffer *buffer = buffer_memory->buffers.GetPointer(resource.index);
            REV_CHECK_M(resource.flags & RESOURCE_FLAG_CPU_READ, "You are not able to read data from \"%.*s\" buffer", buffer->name.Length(), buffer->name.Data());
            
            DefaultBufferMemoryPage *default_page = buffer_memory->default_pages.GetPointer(buffer->default_page_index);

            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.pResource   = default_page->gpu_mem;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.StateBefore = default_page->initial_state;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

            ++barriers_it;
        }
        else
        {
            Texture *texture = &GetTexture(resource);
            REV_CHECK_M(resource.flags & RESOURCE_FLAG_CPU_READ, "You are not able to read data from \"%.*s\" texture", texture->name.Length(), texture->name.Data());

            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.pResource   = texture->default_gpu_mem;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.StateBefore = texture->initial_state;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

            ++barriers_it;
        }
    }

    command_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);

    barriers_it = barriers;
    for (const ResourceHandle& resource : resources)
    {
        if (MemoryManager::IsBuffer(resource))
        {
            BufferMemory *buffer_memory = resource.flags & RESOURCE_FLAG_STATIC
                                        ? &m_StaticMemory.buffer_memory
                                        : &m_SceneMemory.buffer_memory;

            Buffer                   *buffer        = buffer_memory->buffers.GetPointer(resource.index);
            DefaultBufferMemoryPage  *default_page  = buffer_memory->default_pages.GetPointer(buffer->default_page_index);
            ReadBackBufferMemoryPage *readback_page = buffer_memory->readback_pages.GetPointer(buffer->readback_page_index);

            command_list->CopyBufferRegion(readback_page->gpu_mem, buffer->readback_page_offset, default_page->gpu_mem, buffer->default_page_offset, buffer->actual_size);

            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers_it->Transition.StateAfter  = default_page->initial_state;

            ++barriers_it;

#if 0 // @Incomplete(Roman)
            if ((resource.flags & RESOURCE_FLAG_READBACK_ONCE) && readback_page->cpu_mem)
            {
                D3D12_RANGE write_range = { 0, 0 };
                readback_page->gpu_mem->Unmap(0, &write_range);
                readback_page->cpu_mem = null;
            }
#endif
        }
        else
        {
            Texture *texture = &GetTexture(resource);

            ID3D12Device4 *device = m_DeviceContext->Device();

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

            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers_it->Transition.StateAfter  = texture->initial_state;

            ++barriers_it;

            if ((resource.flags & RESOURCE_FLAG_READBACK_ONCE) && texture->readback_cpu_mem)
            {
                D3D12_RANGE write_range = { 0, 0 };
                texture->readback_gpu_mem->Unmap(0, &write_range);
                texture->readback_cpu_mem = null;
            }
        }
    }

    command_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);
}

void MemoryManager::ResizeRenderTarget(ResourceHandle resource, u16 width, u16 height, u16 depth)
{
    REV_CHECK_M(resource.kind == RESOURCE_KIND_TEXTURE, "We do not allow to resize render target buffers for now");

    Texture *texture = resource.flags & RESOURCE_FLAG_STATIC
                     ? m_StaticMemory.texture_memory.textures.GetPointer(resource.index)
                     : m_SceneMemory.texture_memory.textures.GetPointer(resource.index);
    REV_CHECK_M(resource.flags & RESOURCE_FLAG_RENDER_TARGET, "Texture \"%.*s\" is not a render target", texture->name.Length(), texture->name.Data());

    ID3D12Device4 *device = m_DeviceContext->Device();

    D3D12_RESOURCE_DESC default_desc = texture->default_gpu_mem->GetDesc();
    default_desc.Width            = Math::max(1, width);
    default_desc.Height           = Math::max(1, height);
    default_desc.DepthOrArraySize = Math::max(1, depth);

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

    D3D12_RESOURCE_ALLOCATION_INFO1 info1{};
    device->GetResourceAllocationInfo1(0, 1, &default_desc, &info1);

    texture->first_subresource_offset = info1.Offset;

    if (resource.flags & (RESOURCE_FLAG_CPU_WRITE | RESOURCE_FLAG_CPU_READ))
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

        if (resource.flags & RESOURCE_FLAG_CPU_WRITE)
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

        if (resource.flags & RESOURCE_FLAG_CPU_READ)
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

void MemoryManager::SetBufferName(Buffer *buffer)
{
    UINT     buffer_name_length = cast(UINT, buffer->name.Length());
    wchar_t *buffer_name        = Memory::Get()->PushToFA<wchar_t>(buffer_name_length + 1);
    MultiByteToWideChar(CP_ACP, 0, buffer->name.Data(), buffer_wname_length, buffer_name, buffer_wname_length);

    {
        AddBufferNameToBufferPage(buffer->default_page->gpu_mem, L"DEFAULT ", buffer_name, buffer_name_length);
    }

    if (buffer->flags & RESOURCE_FLAG_CPU_WRITE_ONCE)
    {
        AddBufferNameToBufferPage(buffer->upload_page->gpu_mem, L"UPLOAD_ONCE ", buffer_name, buffer_name_length);
    }
    else if (buffer->flags & RESOURCE_FLAG_CPU_WRITE)
    {
        AddBufferNameToBufferPage(buffer->upload_page->gpu_mem, L"UPLOAD ", buffer_name, buffer_name_length);
    }

    if (buffer->flags & RESOURCE_FLAG_CPU_READ_ONCE)
    {
        AddBufferNameToBufferPage(buffer->readback_page->gpu_mem, L"READBACK_ONCE ", buffer_name, buffer_name_length);
    }
    else if (buffer->flags & RESOURCE_FLAG_CPU_READ)
    {
        AddBufferNameToBufferPage(buffer->readback_page->gpu_mem, L"READBACK ", buffer_name, buffer_name_length);
    }
}

void MemoryManager::SetTextureName(Texture *texture)
{
    wchar_t *wname = Memory::Get()->PushToFA<wchar_t>(texture->name.Length() + 1);
    MultiByteToWideChar(CP_ACP, 0, texture->name.Data(), cast(int, texture->name.Length()), wname, cast(int, texture->name.Length()));

    u32      new_wname_length = cast(u32, 16 + texture->name.Length());
    wchar_t *new_wname        = Memory::Get()->PushToFA<wchar_t>(new_wname_length + 1);

    {
        CopyMemory(new_wname, REV_CSTR_ARGS(L"DEFAULT "));
        CopyMemory(new_wname + REV_CSTRLEN(L"DEFAULT "), wname, texture->name.Length() * sizeof(wchar_t));

        HRESULT error = texture->default_gpu_mem->SetName(new_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    if (texture->flags & RESOURCE_FLAG_CPU_WRITE_ONCE)
    {
        CopyMemory(new_wname, REV_CSTR_ARGS(L"UPLOAD_ONCE "));
        CopyMemory(new_wname + REV_CSTRLEN(L"UPLOAD_ONCE "), wname, texture->name.Length() * sizeof(wchar_t));

        HRESULT error = texture->upload_gpu_mem->SetName(new_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }
    else if (texture->flags & RESOURCE_FLAG_CPU_WRITE)
    {
        CopyMemory(new_wname, REV_CSTR_ARGS(L"UPLOAD "));
        CopyMemory(new_wname + REV_CSTRLEN(L"UPLOAD "), wname, texture->name.Length() * sizeof(wchar_t));

        HRESULT error = texture->upload_gpu_mem->SetName(new_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }

    if (texture->flags & RESOURCE_FLAG_CPU_READ_ONCE)
    {
        CopyMemory(new_wname, REV_CSTR_ARGS(L"READBACK_ONCE "));
        CopyMemory(new_wname + REV_CSTRLEN(L"READBACK_ONCE "), wname, texture->name.Length() * sizeof(wchar_t));

        HRESULT error = texture->readback_gpu_mem->SetName(new_wname);
        REV_CHECK(CheckResultAndPrintMessages(error));
    }
    else if (texture->flags & RESOURCE_FLAG_CPU_READ)
    {
        CopyMemory(new_wname, REV_CSTR_ARGS(L"READBACK "));
        CopyMemory(new_wname + REV_CSTRLEN(L"READBACK "), wname, texture->name.Length() * sizeof(wchar_t));

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
