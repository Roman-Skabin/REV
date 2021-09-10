//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/graphics_api.h"
#include "memory/memory.h"
#include "asset_manager/asset_manager.h"
#include "tools/file.h"

#include "platform/d3d12/d3d12_shader_manager.h"
#include "platform/d3d12/d3d12_common.h"

#include <d3dcompiler.h>

namespace REV::D3D12
{

//
// ShaderManager
//

ShaderManager::ShaderManager(Allocator *allocator, const Logger& logger)
    : m_Allocator(allocator),
      m_StaticGraphicsShaders(allocator),
      m_StaticComputeShaders(allocator),
      m_SceneGraphicsShaders(allocator),
      m_SceneComputeShaders(allocator),
      m_Logger(logger, ConstString(REV_CSTR_ARGS("ShaderManager logger")), Logger::TARGET_FILE | Logger::TARGET_CONSOLE)
{
}

ShaderManager::~ShaderManager()
{
    if (m_Allocator)
    {
        FreeSceneShaders();

        for (GraphicsShader& graphics_shader : m_StaticGraphicsShaders)
        {
            SafeRelease(graphics_shader.signature);
            SafeRelease(graphics_shader.root_signature);
            SafeRelease(graphics_shader.pipeline_state);
            SafeRelease(graphics_shader.vertex_shader);
            SafeRelease(graphics_shader.pixel_shader);
            SafeRelease(graphics_shader.hull_shader);
            SafeRelease(graphics_shader.domain_shader);
            SafeRelease(graphics_shader.geometry_shader);
            SafeRelease(graphics_shader.cbv_srv_uav_desc_heap);
            SafeRelease(graphics_shader.sampler_desc_heap);

            graphics_shader.bound_resources.Clear();
        }

        for (ComputeShader& compute_shader : m_StaticComputeShaders)
        {
            SafeRelease(compute_shader.signature);
            SafeRelease(compute_shader.root_signature);
            SafeRelease(compute_shader.pipeline_state);
            SafeRelease(compute_shader.shader);
            SafeRelease(compute_shader.cbv_srv_uav_desc_heap);
            SafeRelease(compute_shader.sampler_desc_heap);

            compute_shader.bound_resources.Clear();
        }

        m_Logger.LogInfo("ShaderManager has been destroyed");
    }
}

void ShaderManager::FreeSceneShaders()
{
    for (GraphicsShader& graphics_shader : m_SceneGraphicsShaders)
    {
        SafeRelease(graphics_shader.signature);
        SafeRelease(graphics_shader.root_signature);
        SafeRelease(graphics_shader.pipeline_state);
        SafeRelease(graphics_shader.vertex_shader);
        SafeRelease(graphics_shader.pixel_shader);
        SafeRelease(graphics_shader.hull_shader);
        SafeRelease(graphics_shader.domain_shader);
        SafeRelease(graphics_shader.geometry_shader);
        SafeRelease(graphics_shader.cbv_srv_uav_desc_heap);
        SafeRelease(graphics_shader.sampler_desc_heap);

        graphics_shader.bound_resources.Clear();
    }

    for (ComputeShader& compute_shader : m_SceneComputeShaders)
    {
        SafeRelease(compute_shader.signature);
        SafeRelease(compute_shader.root_signature);
        SafeRelease(compute_shader.pipeline_state);
        SafeRelease(compute_shader.shader);
        SafeRelease(compute_shader.cbv_srv_uav_desc_heap);
        SafeRelease(compute_shader.sampler_desc_heap);

        compute_shader.bound_resources.Clear();
    }
}

u64 ShaderManager::CreateGraphicsShader(
    const ConstString&                  shader_cache_filename,
    const ConstArray<AssetHandle>&      textures,
    const ConstArray<GPU::CBufferDesc>& cbuffers,
    const ConstArray<GPU::SamplerDesc>& samplers,
    bool                                _static)
{
    Memory                *memory           = Memory::Get();
    Array<GraphicsShader> *graphics_shaders = _static ? &m_StaticGraphicsShaders : &m_SceneGraphicsShaders;
    AssetManager          *asset_manager    = AssetManager::Get();

    GraphicsShader *graphics_shader = new (graphics_shaders->PushBack()) GraphicsShader(m_Allocator);
    LoadShaderCache(graphics_shader, shader_cache_filename);
    CreateDescHeapsAndViews(graphics_shader, textures, cbuffers, samplers);

    u64 resources_count = cbuffers.Count() + textures.Count() + samplers.Count();

    GPU::ResourceHandle  *shader_resoucres     = new (graphics_shader->bound_resources.PushBack(resources_count)) GPU::ResourceHandle();
    D3D12_ROOT_PARAMETER *root_parameters      = memory->PushToFA<D3D12_ROOT_PARAMETER>(resources_count);
    u64                   root_parameter_index = 0;

    for (u64 i = 0; i < cbuffers.Count(); ++i)
    {
        const GPU::CBufferDesc *cbuffer = cbuffers.GetPointer(i);

        *shader_resoucres++ = cbuffer->resource;

        D3D12_ROOT_PARAMETER *it      = root_parameters + root_parameter_index++;
        it->ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
        it->Descriptor.ShaderRegister = cbuffer->shader_register;
        it->Descriptor.RegisterSpace  = cbuffer->register_space;
        it->ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;
    }

    for (u64 i = 0; i < textures.Count(); ++i)
    {
        Asset *asset = asset_manager->GetAsset(textures[i]);

        *shader_resoucres++ = asset->texture.resource;

        D3D12_DESCRIPTOR_RANGE desc_range;
        desc_range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        desc_range.NumDescriptors                    = 1;
        desc_range.BaseShaderRegister                = asset->texture.shader_register;
        desc_range.RegisterSpace                     = asset->texture.register_space;
        desc_range.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER *it                = root_parameters + root_parameter_index++;
        it->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        it->DescriptorTable.NumDescriptorRanges = 1;
        it->DescriptorTable.pDescriptorRanges   = &desc_range;
        it->ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
    }

    for (u64 i = 0; i < samplers.Count(); ++i)
    {
        const GPU::SamplerDesc *sampler = samplers.GetPointer(i);

        *shader_resoucres++ = sampler->resource;

        D3D12_DESCRIPTOR_RANGE desc_range;
        desc_range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        desc_range.NumDescriptors                    = 1;
        desc_range.BaseShaderRegister                = sampler->shader_register;
        desc_range.RegisterSpace                     = sampler->register_space;
        desc_range.OffsetInDescriptorsFromTableStart = 0;

        D3D12_ROOT_PARAMETER *it                = root_parameters + root_parameter_index++;
        it->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        it->DescriptorTable.NumDescriptorRanges = 1;
        it->DescriptorTable.pDescriptorRanges   = &desc_range;
        it->ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
    }

    // @TODO(Roman): Static samplers
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.NumParameters     = cast(u32, resources_count);
    root_signature_desc.pParameters       = root_parameters;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers   = null;
    root_signature_desc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // @TODO(Roman): D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT

    CreateRootSignature(graphics_shader, root_signature_desc);

#if 1
    D3D12_INPUT_ELEMENT_DESC elements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_INPUT_LAYOUT_DESC input_layout;
    input_layout.pInputElementDescs = elements;
    input_layout.NumElements        = cast(u32, ArrayCount(elements));
#endif

    CreatePipelineState(graphics_shader, input_layout, false, D3D12_CULL_MODE_NONE, true);

    return graphics_shaders->Count() - 1;
}

void ShaderManager::SetCurrentGraphicsShader(const GraphicsShader& graphics_shader)
{
    DeviceContext *device_context = cast(DeviceContext *, GraphicsAPI::GetDeviceContext());
    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());
    AssetManager  *asset_manager = AssetManager::Get();

    ID3D12GraphicsCommandList *graphics_list = device_context->CurrentGraphicsList();

    graphics_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphics_list->SetGraphicsRootSignature(graphics_shader.root_signature);
    graphics_list->SetPipelineState(graphics_shader.pipeline_state);

    if (graphics_shader.bound_resources.Count())
    {
        if (graphics_shader.cbv_srv_uav_desc_heap && graphics_shader.sampler_desc_heap)
        {
            ID3D12DescriptorHeap *desc_heaps[] =
            {
                graphics_shader.cbv_srv_uav_desc_heap,
                graphics_shader.sampler_desc_heap
            };
            graphics_list->SetDescriptorHeaps(cast(u32, ArrayCount(desc_heaps)), desc_heaps);
        }
        else if (graphics_shader.cbv_srv_uav_desc_heap)
        {
            graphics_list->SetDescriptorHeaps(1, &graphics_shader.cbv_srv_uav_desc_heap);
        }
        else
        {
            graphics_list->SetDescriptorHeaps(1, &graphics_shader.sampler_desc_heap);
        }

        u32 cbv_srv_uav_descriptor_size = device_context->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        u32 sampler_descriptor_size     = device_context->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        D3D12_GPU_DESCRIPTOR_HANDLE cbv_srv_uav_gpu_descriptor_handle = graphics_shader.cbv_srv_uav_desc_heap->GetGPUDescriptorHandleForHeapStart();
        D3D12_GPU_DESCRIPTOR_HANDLE sampler_gpu_descriptor_handle     = graphics_shader.sampler_desc_heap->GetGPUDescriptorHandleForHeapStart();

        u32 resource_index = 0;
        for (const GPU::ResourceHandle& resource : graphics_shader.bound_resources)
        {
            switch (resource.kind)
            {
                case GPU::RESOURCE_KIND_CB:
                case GPU::RESOURCE_KIND_STATIC_CB:
                {
                    graphics_list->SetGraphicsRootConstantBufferView(resource_index, memory_manager->GetBufferGPUVirtualAddress(resource));
                    cbv_srv_uav_gpu_descriptor_handle.ptr += cbv_srv_uav_descriptor_size;
                } break;

                case GPU::RESOURCE_KIND_SR:
                case GPU::RESOURCE_KIND_STATIC_SR:
                {
                    Texture& texture = memory_manager->GetTexture(resource);

                    graphics_list->SetGraphicsRootDescriptorTable(resource_index, cbv_srv_uav_gpu_descriptor_handle);
                    cbv_srv_uav_gpu_descriptor_handle.ptr += cbv_srv_uav_descriptor_size;
                } break;

                case GPU::RESOURCE_KIND_SAMPLER:
                case GPU::RESOURCE_KIND_STATIC_SAMPLER:
                {
                    Sampler& sampler = memory_manager->GetSampler(resource);

                    graphics_list->SetGraphicsRootDescriptorTable(resource_index, sampler_gpu_descriptor_handle);
                    sampler_gpu_descriptor_handle.ptr += sampler_descriptor_size;
                } break;

                default:
                {
                    REV_ERROR_M("Invalid or unsupported resource kind: %I64u", resource.kind);
                } break;
            }

            ++resource_index;
        }
    }
}

void ShaderManager::BindVertexBuffer(GraphicsShader& graphics_shader, const GPU::ResourceHandle& resource_handle)
{
    REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_VB, "Resource #%I64u is not a vertex buffer", resource_handle.index);

    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());
    Buffer&        buffer         = memory_manager->GetBuffer(resource_handle);

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = memory_manager->GetBufferGPUVirtualAddress(resource_handle);
    vbv.SizeInBytes    = cast(u32, buffer.actual_size);
    vbv.StrideInBytes  = buffer.vstride;

    cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->CurrentGraphicsList()->IASetVertexBuffers(0, 1, &vbv);
}

void ShaderManager::BindIndexBuffer(GraphicsShader& graphics_shader, const GPU::ResourceHandle& resource_handle)
{
    REV_CHECK_M(resource_handle.kind & GPU::RESOURCE_KIND_IB, "Resource #%I64u is not an index buffer", resource_handle.index);

    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());
    Buffer&        buffer         = memory_manager->GetBuffer(resource_handle);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = memory_manager->GetBufferGPUVirtualAddress(resource_handle);
    ibv.SizeInBytes    = cast(u32, buffer.actual_size);
    ibv.Format         = DXGI_FORMAT_R32_UINT;

    cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->CurrentGraphicsList()->IASetIndexBuffer(&ibv);

    graphics_shader.index_buffer = resource_handle;
}

void ShaderManager::Draw(const GraphicsShader& graphics_shader)
{
    Buffer& buffer = cast(MemoryManager *, GraphicsAPI::GetMemoryManager())->GetBuffer(graphics_shader.index_buffer);
    cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->CurrentGraphicsList()->DrawIndexedInstanced(buffer.icount, 1, 0, 0, 0);
}

ID3DBlob *ShaderManager::CompileShader(const ConstString& hlsl_code, const char *name, const char *entry_point, const char *target)
{
    u32 compile_flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#if REV_DEBUG
    compile_flags |= D3DCOMPILE_DEBUG
                  |  D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3
                  |  D3DCOMPILE_SKIP_VALIDATION;
#endif
    if (cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->HalfPrecisionSupported())
    {
        compile_flags |= D3DCOMPILE_PARTIAL_PRECISION;
    }

    ID3DBlob *code   = null;
    ID3DBlob *errors = null;
    HRESULT   error  = D3DCompile(hlsl_code.Data(),
                                  hlsl_code.Length(),
                                  name,
                                  null,
                                  D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                  entry_point,
                                  target,
                                  compile_flags,
                                  0,
                                  &code,
                                  &errors);

    if (Failed(error))
    {
        REV_ERROR_M("Shader compilation failure:\n%s", errors->GetBufferPointer());
    }

    SafeRelease(errors);

    return code;
}

void ShaderManager::LoadShaderCache(GraphicsShader *graphics_shader, const ConstString& shader_cache_filename)
{
    File file(shader_cache_filename, File::FLAG_READ | File::FLAG_EXISTS | File::FLAG_SEQ);
    
    GPU::SHADER_KIND shader_kind = GPU::SHADER_KIND_UNKNOWN;
    file.Read(&shader_kind, sizeof(GPU::SHADER_KIND));

    REV_CHECK(shader_kind & GPU::SHADER_KIND_VERTEX)
    {
        u32 size = 0;
        file.Read(&size, sizeof(size));

        HRESULT error = D3DCreateBlob(size, &graphics_shader->vertex_shader);
        REV_CHECK(CheckResultAndPrintMessages(error));

        file.Read(graphics_shader->vertex_shader->GetBufferPointer(), size);
    }
    if (shader_kind & GPU::SHADER_KIND_HULL)
    {
        u32 size = 0;
        file.Read(&size, sizeof(size));

        HRESULT error = D3DCreateBlob(size, &graphics_shader->hull_shader);
        REV_CHECK(CheckResultAndPrintMessages(error));

        file.Read(graphics_shader->hull_shader->GetBufferPointer(), size);
    }
    if (shader_kind & GPU::SHADER_KIND_DOMAIN)
    {
        u32 size = 0;
        file.Read(&size, sizeof(size));

        HRESULT error = D3DCreateBlob(size, &graphics_shader->domain_shader);
        REV_CHECK(CheckResultAndPrintMessages(error));

        file.Read(graphics_shader->domain_shader->GetBufferPointer(), size);
    }
    if (shader_kind & GPU::SHADER_KIND_GEOMETRY)
    {
        u32 size = 0;
        file.Read(&size, sizeof(size));

        HRESULT error = D3DCreateBlob(size, &graphics_shader->geometry_shader);
        REV_CHECK(CheckResultAndPrintMessages(error));

        file.Read(graphics_shader->geometry_shader->GetBufferPointer(), size);
    }
    REV_CHECK(shader_kind & GPU::SHADER_KIND_PIXEL)
    {
        u32 size = 0;
        file.Read(&size, sizeof(size));

        HRESULT error = D3DCreateBlob(size, &graphics_shader->pixel_shader);
        REV_CHECK(CheckResultAndPrintMessages(error));

        file.Read(graphics_shader->pixel_shader->GetBufferPointer(), size);
    }
}

void ShaderManager::CreateDescHeapsAndViews(
    GraphicsShader                      *graphics_shader,
    const ConstArray<AssetHandle>&       textures,
    const ConstArray<GPU::CBufferDesc>&  cbuffers,
    const ConstArray<GPU::SamplerDesc>&  samplers)
{
    ID3D12Device  *device         = cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->Device();
    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());
    AssetManager  *asset_manager  = AssetManager::Get();

    if (cbuffers.Count() || textures.Count())
    {
        D3D12_DESCRIPTOR_HEAP_DESC cbv_srv_uav_desc_heap_desc;
        cbv_srv_uav_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbv_srv_uav_desc_heap_desc.NumDescriptors = cast(u32, textures.Count() + cbuffers.Count());
        cbv_srv_uav_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        cbv_srv_uav_desc_heap_desc.NodeMask       = 0;

        HRESULT error = device->CreateDescriptorHeap(&cbv_srv_uav_desc_heap_desc, IID_PPV_ARGS(&graphics_shader->cbv_srv_uav_desc_heap));
        REV_CHECK(CheckResultAndPrintMessages(error));

        u32                         cbv_srv_uav_descriptor_size       = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE cbv_srv_uav_cpu_descriptor_handle = graphics_shader->cbv_srv_uav_desc_heap->GetCPUDescriptorHandleForHeapStart();

        for (const GPU::CBufferDesc& cbuffer_desc : cbuffers)
        {
            Buffer& cbuffer = memory_manager->GetBuffer(cbuffer_desc.resource);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
            cbv_desc.BufferLocation = memory_manager->GetBufferGPUVirtualAddress(cbuffer_desc.resource);
            cbv_desc.SizeInBytes    = cast(u32, cbuffer.aligned_size);

            device->CreateConstantBufferView(&cbv_desc, cbv_srv_uav_cpu_descriptor_handle);
            cbv_srv_uav_cpu_descriptor_handle.ptr += cbv_srv_uav_descriptor_size;
        }

        for (const AssetHandle& texture_asset_handle : textures)
        {
            Asset   *texture_asset = asset_manager->GetAsset(texture_asset_handle);
            Texture& texture       = memory_manager->GetTexture(texture_asset->texture.resource);

            D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
            srv_desc.Format                  = texture.desc.Format;
            srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            switch (texture.kind)
            {
                case TEXTURE_KIND_1D:
                {
                    srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE1D;
                    srv_desc.Texture1D.MostDetailedMip     = 0;    // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                    srv_desc.Texture1D.MipLevels           = 1;    // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                    srv_desc.Texture1D.ResourceMinLODClamp = 0.5f; // scale bound
                } break;

                case TEXTURE_KIND_2D:
                {
                    srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Texture2D.MostDetailedMip     = 0;                      // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                    srv_desc.Texture2D.MipLevels           = texture.desc.MipLevels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                    srv_desc.Texture2D.PlaneSlice          = 0;                      // The index (plane slice number) of the plane to use in the texture.
                    srv_desc.Texture2D.ResourceMinLODClamp = 0.5f;                   // scale bound
                } break;

                case TEXTURE_KIND_3D:
                {
                    srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
                    srv_desc.Texture3D.MostDetailedMip     = 0;                      // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                    srv_desc.Texture3D.MipLevels           = texture.desc.MipLevels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                    srv_desc.Texture3D.ResourceMinLODClamp = 0.5f;                   // scale bound
                } break;

                case TEXTURE_KIND_CUBE:
                {
                    srv_desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    srv_desc.TextureCube.MostDetailedMip     = 0;                      // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                    srv_desc.TextureCube.MipLevels           = texture.desc.MipLevels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                    srv_desc.TextureCube.ResourceMinLODClamp = 0.5f;                   // scale bound
                } break;

                case TEXTURE_KIND_1D_ARRAY:
                {
                    srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                    srv_desc.Texture1DArray.MostDetailedMip     = 0;                             // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                    srv_desc.Texture1DArray.MipLevels           = 1;                             // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                    srv_desc.Texture1DArray.FirstArraySlice     = 0;                             // First texture in an array to access [0, desc.DepthOrArraySize)
                    srv_desc.Texture1DArray.ArraySize           = texture.desc.DepthOrArraySize; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
                    srv_desc.Texture1DArray.ResourceMinLODClamp = 0.5f;                          // scale bound
                } break;

                case TEXTURE_KIND_2D_ARRAY:
                {
                    srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    srv_desc.Texture2DArray.MostDetailedMip     = 0;                             // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                    srv_desc.Texture2DArray.MipLevels           = texture.desc.MipLevels;        // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                    srv_desc.Texture2DArray.FirstArraySlice     = 0;                             // First texture in an array to access [0, desc.DepthOrArraySize)
                    srv_desc.Texture2DArray.ArraySize           = texture.desc.DepthOrArraySize; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
                    srv_desc.Texture2DArray.PlaneSlice          = 0;                             // The index (plane slice number) of the plane to use in the texture.
                    srv_desc.Texture2DArray.ResourceMinLODClamp = 0.5f;                          // scale bound
                } break;

                case TEXTURE_KIND_CUBE_ARRAY:
                {
                    srv_desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                    srv_desc.TextureCubeArray.MostDetailedMip     = 0;                             // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                    srv_desc.TextureCubeArray.MipLevels           = texture.desc.MipLevels;        // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                    srv_desc.TextureCubeArray.First2DArrayFace    = 0;                             // First texture in an array to access [0, desc.DepthOrArraySize)
                    srv_desc.TextureCubeArray.NumCubes            = texture.desc.DepthOrArraySize; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
                    srv_desc.TextureCubeArray.ResourceMinLODClamp = 0.5f;                          // scale bound
                } break;

                default:
                {
                    REV_ERROR_M("Invalid TEXTURE_KIND: %I32u", texture.kind);
                } break;
            }

            device->CreateShaderResourceView(texture.def_resource, &srv_desc, cbv_srv_uav_cpu_descriptor_handle);
            cbv_srv_uav_cpu_descriptor_handle.ptr += cbv_srv_uav_descriptor_size;
        }
    }

    if (samplers.Count())
    {
        D3D12_DESCRIPTOR_HEAP_DESC sampler_desc_heap_desc = {};
        sampler_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        sampler_desc_heap_desc.NumDescriptors = cast(u32, samplers.Count());
        sampler_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        sampler_desc_heap_desc.NodeMask       = 0;

        HRESULT error = device->CreateDescriptorHeap(&sampler_desc_heap_desc, IID_PPV_ARGS(&graphics_shader->sampler_desc_heap));
        REV_CHECK(CheckResultAndPrintMessages(error));

        u32                         sampler_descriptor_size       = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpu_descriptor_handle = graphics_shader->sampler_desc_heap->GetCPUDescriptorHandleForHeapStart();

        for (const GPU::SamplerDesc& sampler_desc : samplers)
        {
            Sampler& sampler = memory_manager->GetSampler(sampler_desc.resource);

            device->CreateSampler(&sampler.desc, sampler_cpu_descriptor_handle);
            sampler_cpu_descriptor_handle.ptr += sampler_descriptor_size;
        }
    }
}

void ShaderManager::CreateRootSignature(GraphicsShader *graphics_shader, const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc)
{
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC vrsd = {};
    vrsd.Version = D3D_ROOT_SIGNATURE_VERSION_1_0; // device_context->HighestRootSignatureVersion();
    if (vrsd.Version == D3D_ROOT_SIGNATURE_VERSION_1_0)
    {
        vrsd.Desc_1_0       = root_signature_desc;
        vrsd.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                          // @TODO(Roman): support stream output
                          /*| D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT*/;
    }
    else if (vrsd.Version == D3D_ROOT_SIGNATURE_VERSION_1_1)
    {
        // @TODO(Roman): Root Signature v1.1 support
        REV_ERROR_M("Root Signature v1.1 is not supported yet");
    }
    else
    {
        // @NOTE(Roman): This is the first and the last time
        //               we're asserting Root Signature version.
        //               It's not needed to do this another places.
        REV_ERROR_M("Unsuppored or unknown Root Signature version");
    }

    ID3DBlob *error_blob = null;
    HRESULT   error      = D3D12SerializeVersionedRootSignature(&vrsd, &graphics_shader->signature, &error_blob);
    if (Failed(error))
    {
        REV_ERROR_M("Versioned Root Signature creation failure: %s", error_blob->GetBufferPointer());
    }
    SafeRelease(error_blob);

    error = cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->Device()->CreateRootSignature(0,
                                                                                                  graphics_shader->signature->GetBufferPointer(),
                                                                                                  graphics_shader->signature->GetBufferSize(),
                                                                                                  IID_PPV_ARGS(&graphics_shader->root_signature));
    REV_CHECK(CheckResultAndPrintMessages(error));
}

void ShaderManager::CreatePipelineState(GraphicsShader *graphics_shader, const D3D12_INPUT_LAYOUT_DESC& input_layout, bool blending_enabled, D3D12_CULL_MODE cull_mode, bool depth_test_enabled)
{
    // @TODO(Roman): support stream output
    D3D12_STREAM_OUTPUT_DESC sod;
    sod.pSODeclaration   = null;
    sod.NumEntries       = 0;
    sod.pBufferStrides   = null;
    sod.NumStrides       = 0;
    sod.RasterizedStream = 0;

    D3D12_BLEND_DESC blend_desc = {0};
    if (blending_enabled)
    {
        D3D12_RENDER_TARGET_BLEND_DESC rtbd_main;
        rtbd_main.BlendEnable           = true;
        rtbd_main.LogicOpEnable         = false;
        rtbd_main.SrcBlend              = D3D12_BLEND_ONE;
        rtbd_main.DestBlend             = D3D12_BLEND_ONE;
        rtbd_main.BlendOp               = D3D12_BLEND_OP_ADD;
        rtbd_main.SrcBlendAlpha         = D3D12_BLEND_ONE;
        rtbd_main.DestBlendAlpha        = D3D12_BLEND_ONE;
        rtbd_main.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        rtbd_main.LogicOp               = D3D12_LOGIC_OP_NOOP;
        rtbd_main.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_RENDER_TARGET_BLEND_DESC rtbd_sum;
        rtbd_sum.BlendEnable           = true;
        rtbd_sum.LogicOpEnable         = false;
        rtbd_sum.SrcBlend              = D3D12_BLEND_ONE;
        rtbd_sum.DestBlend             = D3D12_BLEND_ONE;
        rtbd_sum.BlendOp               = D3D12_BLEND_OP_ADD;
        rtbd_sum.SrcBlendAlpha         = D3D12_BLEND_ONE;
        rtbd_sum.DestBlendAlpha        = D3D12_BLEND_ONE;
        rtbd_sum.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        rtbd_sum.LogicOp               = D3D12_LOGIC_OP_NOOP;
        rtbd_sum.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_RENDER_TARGET_BLEND_DESC rtbd_mul;
        rtbd_mul.BlendEnable           = true;
        rtbd_mul.LogicOpEnable         = false;
        rtbd_mul.SrcBlend              = D3D12_BLEND_ZERO;
        rtbd_mul.DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
        rtbd_mul.BlendOp               = D3D12_BLEND_OP_ADD;
        rtbd_mul.SrcBlendAlpha         = D3D12_BLEND_ZERO;
        rtbd_mul.DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
        rtbd_mul.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        rtbd_mul.LogicOp               = D3D12_LOGIC_OP_NOOP;
        rtbd_mul.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        blend_desc.RenderTarget[0] = rtbd_main;
        blend_desc.RenderTarget[1] = rtbd_sum;
        blend_desc.RenderTarget[2] = rtbd_mul;
    }
    else
    {
        D3D12_RENDER_TARGET_BLEND_DESC rtbd;
        rtbd.BlendEnable           = false;
        rtbd.LogicOpEnable         = false;
        rtbd.SrcBlend              = D3D12_BLEND_ONE;
        rtbd.DestBlend             = D3D12_BLEND_ONE;
        rtbd.BlendOp               = D3D12_BLEND_OP_ADD;
        rtbd.SrcBlendAlpha         = D3D12_BLEND_ONE;
        rtbd.DestBlendAlpha        = D3D12_BLEND_ONE;
        rtbd.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        rtbd.LogicOp               = D3D12_LOGIC_OP_NOOP;
        rtbd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        blend_desc.RenderTarget[0] = rtbd;
    }

    D3D12_RASTERIZER_DESC rasterizer_desc;
    rasterizer_desc.FillMode              = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode              = cull_mode;
    rasterizer_desc.FrontCounterClockwise = false;
    rasterizer_desc.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_desc.DepthClipEnable       = true;
    rasterizer_desc.MultisampleEnable     = false;
    rasterizer_desc.AntialiasedLineEnable = false;
    rasterizer_desc.ForcedSampleCount     = 0;
    rasterizer_desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_DEPTH_STENCIL_DESC depth_desc;
    depth_desc.DepthEnable                  = depth_test_enabled;
    depth_desc.DepthWriteMask               = blending_enabled ? D3D12_DEPTH_WRITE_MASK_ZERO : D3D12_DEPTH_WRITE_MASK_ALL;
    depth_desc.DepthFunc                    = D3D12_COMPARISON_FUNC_LESS;
    depth_desc.StencilEnable                = false;
    depth_desc.StencilReadMask              = REV_U8_MAX;
    depth_desc.StencilWriteMask             = REV_U8_MAX;
    depth_desc.FrontFace.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
    depth_desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depth_desc.FrontFace.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
    depth_desc.FrontFace.StencilFunc        = D3D12_COMPARISON_FUNC_NEVER;
    depth_desc.BackFace.StencilFailOp       = D3D12_STENCIL_OP_KEEP;
    depth_desc.BackFace.StencilDepthFailOp  = D3D12_STENCIL_OP_KEEP;
    depth_desc.BackFace.StencilPassOp       = D3D12_STENCIL_OP_KEEP;
    depth_desc.BackFace.StencilFunc         = D3D12_COMPARISON_FUNC_NEVER;

    // @TODO(Roman): Cached pipeline state
    D3D12_CACHED_PIPELINE_STATE cached_pipeline_state;
    cached_pipeline_state.pCachedBlob           = 0;
    cached_pipeline_state.CachedBlobSizeInBytes = 0;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {0};
    gpsd.pRootSignature        = graphics_shader->root_signature;
    gpsd.VS.pShaderBytecode    = graphics_shader->vertex_shader->GetBufferPointer();
    gpsd.VS.BytecodeLength     = graphics_shader->vertex_shader->GetBufferSize();
    gpsd.PS.pShaderBytecode    = graphics_shader->pixel_shader->GetBufferPointer();
    gpsd.PS.BytecodeLength     = graphics_shader->pixel_shader->GetBufferSize();
    if (graphics_shader->domain_shader)
    {
        gpsd.DS.pShaderBytecode = graphics_shader->domain_shader->GetBufferPointer();
        gpsd.DS.BytecodeLength  = graphics_shader->domain_shader->GetBufferSize();
    }
    if (graphics_shader->hull_shader)
    {
        gpsd.HS.pShaderBytecode = graphics_shader->hull_shader->GetBufferPointer();
        gpsd.HS.BytecodeLength  = graphics_shader->hull_shader->GetBufferSize();
    }
    if (graphics_shader->geometry_shader)
    {
        gpsd.GS.pShaderBytecode = graphics_shader->geometry_shader->GetBufferPointer();
        gpsd.GS.BytecodeLength  = graphics_shader->geometry_shader->GetBufferSize();
    }
    gpsd.StreamOutput          = sod;
    gpsd.BlendState            = blend_desc;
    gpsd.SampleMask            = UINT_MAX;
    gpsd.RasterizerState       = rasterizer_desc;
    gpsd.DepthStencilState     = depth_desc;
    gpsd.InputLayout           = input_layout;
    gpsd.IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // @TODO(Roman): support triangle strips
    gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    if (blending_enabled)
    {
        gpsd.NumRenderTargets = 3;
        gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        gpsd.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        gpsd.RTVFormats[2] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        for (u32 i = gpsd.NumRenderTargets; i < ArrayCount(gpsd.RTVFormats); ++i)
        {
            gpsd.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
        }
    }
    else
    {
        gpsd.NumRenderTargets = 1;
        gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        for (u32 i = gpsd.NumRenderTargets; i < ArrayCount(gpsd.RTVFormats); ++i)
        {
            gpsd.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
        }
    }
    gpsd.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
    gpsd.SampleDesc.Count      = 1;
    gpsd.SampleDesc.Quality    = 0;
    gpsd.NodeMask              = 0;
    gpsd.CachedPSO             = cached_pipeline_state;
    gpsd.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

    HRESULT error = cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->Device()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&graphics_shader->pipeline_state));
    REV_CHECK(CheckResultAndPrintMessages(error));
}

}
