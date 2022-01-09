// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/graphics_api.h"
#include "memory/memory.h"
#include "tools/file.h"

#include "platform/d3d12/d3d12_shader_manager.h"
#include "platform/d3d12/d3d12_common.h"

namespace REV::D3D12
{

//
// ShaderManager
//

ShaderManager::ShaderManager(Allocator *allocator, const Logger& logger)
    : m_StaticGraphicsShaders(allocator),
      m_StaticComputeShaders(allocator),
      m_SceneGraphicsShaders(allocator),
      m_SceneComputeShaders(allocator),
      m_Logger(logger, ConstString(REV_CSTR_ARGS("ShaderManager")), Logger::TARGET_FILE | Logger::TARGET_CONSOLE)
{
}

ShaderManager::~ShaderManager()
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
        SafeRelease(graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_heap);
        SafeRelease(graphics_shader.desc_heap_table.RTV_desc_heap);
        SafeRelease(graphics_shader.desc_heap_table.sampler_desc_heap);
    }

    for (ComputeShader& compute_shader : m_StaticComputeShaders)
    {
        SafeRelease(compute_shader.signature);
        SafeRelease(compute_shader.root_signature);
        SafeRelease(compute_shader.pipeline_state);
        SafeRelease(compute_shader.shader);
        SafeRelease(compute_shader.desc_heap_table.CBV_SRV_UAV_desc_heap);
        SafeRelease(compute_shader.desc_heap_table.RTV_desc_heap);
        SafeRelease(compute_shader.desc_heap_table.sampler_desc_heap);
    }

    m_Logger.LogInfo("ShaderManager has been destroyed");
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
        SafeRelease(graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_heap);
        SafeRelease(graphics_shader.desc_heap_table.RTV_desc_heap);
        SafeRelease(graphics_shader.desc_heap_table.sampler_desc_heap);
    }

    for (ComputeShader& compute_shader : m_SceneComputeShaders)
    {
        SafeRelease(compute_shader.signature);
        SafeRelease(compute_shader.root_signature);
        SafeRelease(compute_shader.pipeline_state);
        SafeRelease(compute_shader.shader);
        SafeRelease(compute_shader.desc_heap_table.CBV_SRV_UAV_desc_heap);
        SafeRelease(compute_shader.desc_heap_table.RTV_desc_heap);
        SafeRelease(compute_shader.desc_heap_table.sampler_desc_heap);
    }
}

u64 ShaderManager::CreateGraphicsShader(const ConstString& shader_cache_filename, const ConstArray<GPU::ShaderResourceDesc>& resources, bool _static)
{
    Memory                *memory           = Memory::Get();
    Array<GraphicsShader> *graphics_shaders = _static ? &m_StaticGraphicsShaders : &m_SceneGraphicsShaders;
    GPU::MemoryManager    *memory_manager   = GraphicsAPI::GetMemoryManager();

    GraphicsShader *graphics_shader = graphics_shaders->PushBack();
    graphics_shader->_static = _static;
    LoadShaderCache(graphics_shader, shader_cache_filename);
    InitBoundResources(graphics_shader, resources);
    CreateDescHeaps(graphics_shader);
    CreateViews(graphics_shader);
    ConstArray root_parameters = CreateRootSignatureParameters(graphics_shader);

    // @TODO(Roman): Static samplers
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.NumParameters     = cast(u32, root_parameters.Count());
    root_signature_desc.pParameters       = root_parameters.Data();
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers   = null;
    root_signature_desc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
                         // @TODO(Roman): | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT

    CreateRootSignature(graphics_shader, root_signature_desc);

    D3D12_INPUT_ELEMENT_DESC elements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_INPUT_LAYOUT_DESC input_layout;
    input_layout.pInputElementDescs = elements;
    input_layout.NumElements        = cast(u32, ArrayCount(elements));

    CreatePipelineState(graphics_shader, input_layout, false, D3D12_CULL_MODE_NONE, true);

    return graphics_shaders->Count() - 1;
}

void ShaderManager::SetCurrentGraphicsShader(const GraphicsShader& graphics_shader)
{
    DeviceContext *device_context = cast(DeviceContext *, GraphicsAPI::GetDeviceContext());
    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());
    Memory        *memory         = Memory::Get();

    ID3D12GraphicsCommandList *graphics_list = device_context->CurrentGraphicsList();

    // @TODO(Roman): Support triangle strips
    graphics_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphics_list->SetGraphicsRootSignature(graphics_shader.root_signature);
    graphics_list->SetPipelineState(graphics_shader.pipeline_state);

    if (graphics_shader.bound_resources.CBV_count
    ||  graphics_shader.bound_resources.SRV_count
    ||  graphics_shader.bound_resources.UAV_count
    ||  graphics_shader.bound_resources.sampler_count)
    {
        ID3D12DescriptorHeap *desc_heaps[2]    = {null};
        u32                   desc_heaps_count = 0;

        if (graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_heap) desc_heaps[desc_heaps_count++] = graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_heap;
        if (graphics_shader.desc_heap_table.sampler_desc_heap)     desc_heaps[desc_heaps_count++] = graphics_shader.desc_heap_table.sampler_desc_heap;

        if (!desc_heaps_count)
        {
            REV_ERROR_M("Resources has been bound but descriptor heaps has not been created");
        }

        graphics_list->SetDescriptorHeaps(desc_heaps_count, desc_heaps);

        D3D12_GPU_DESCRIPTOR_HANDLE CBV_SRV_UAV_gpu_descriptor_handle_it = graphics_shader.desc_heap_table.CBV_SRV_UAV_gpu_desc_handle;

        u32 root_parameter_index = 0;
        if (graphics_shader.bound_resources.CBV_count)
        {
            graphics_list->SetGraphicsRootDescriptorTable(root_parameter_index++, CBV_SRV_UAV_gpu_descriptor_handle_it);
            CBV_SRV_UAV_gpu_descriptor_handle_it.ptr += graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_size;
        }

        if (graphics_shader.bound_resources.SRV_count)
        {
            graphics_list->SetGraphicsRootDescriptorTable(root_parameter_index++, CBV_SRV_UAV_gpu_descriptor_handle_it);
            CBV_SRV_UAV_gpu_descriptor_handle_it.ptr += graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_size;
        }

        if (graphics_shader.bound_resources.UAV_count)
        {
            //
            // Set UAVs
            //
            graphics_list->SetGraphicsRootDescriptorTable(root_parameter_index++, CBV_SRV_UAV_gpu_descriptor_handle_it);
            CBV_SRV_UAV_gpu_descriptor_handle_it.ptr += graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_size;

            //
            // Change UAVs' state
            //
            D3D12_RESOURCE_BARRIER *barriers    = memory->PushToFA<D3D12_RESOURCE_BARRIER>(graphics_shader.bound_resources.UAV_count);
            D3D12_RESOURCE_BARRIER *barriers_it = barriers;

            for (ResourcesTableNode *UAV = graphics_shader.bound_resources.UAV_first; UAV; UAV = UAV->next)
            {
                if (UAV->resoucre_desc.resource.kind == GPU::RESOURCE_KIND_TEXTURE)
                {
                    Texture& texture = memory_manager->GetTexture(UAV->resoucre_desc.resource);

                    barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                    barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    barriers_it->Transition.pResource   = texture.default_gpu_mem;
                    barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                    barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

                    ++barriers_it;
                }
                else
                {
                    REV_WARNING_M("Incomplete UAVs' state changing for buffers");
                }
            }

            graphics_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);

            //
            // Clear UAVs
            //
            D3D12_GPU_DESCRIPTOR_HANDLE uav_gpu_desc_handle = graphics_shader.desc_heap_table.CBV_SRV_UAV_gpu_desc_handle;
            D3D12_CPU_DESCRIPTOR_HANDLE uav_cpu_desc_handle = graphics_shader.desc_heap_table.CBV_SRV_UAV_cpu_desc_handle;

            for (ResourcesTableNode *UAV = graphics_shader.bound_resources.UAV_first; UAV; UAV = UAV->next)
            {
                if (UAV->resoucre_desc.resource.kind == GPU::RESOURCE_KIND_TEXTURE)
                {
                    Texture& texture = memory_manager->GetTexture(UAV->resoucre_desc.resource);

                    if (UAV->resoucre_desc.uav_is_cleared_with_floats)
                    {
                        graphics_list->ClearUnorderedAccessViewFloat(uav_gpu_desc_handle,
                                                                     uav_cpu_desc_handle,
                                                                     texture.default_gpu_mem,
                                                                     UAV->resoucre_desc.uav_float_clear_color.e,
                                                                     0,
                                                                     null);
                    }
                    else
                    {
                        graphics_list->ClearUnorderedAccessViewUint(uav_gpu_desc_handle,
                                                                    uav_cpu_desc_handle,
                                                                    texture.default_gpu_mem,
                                                                    cast(const UINT *, UAV->resoucre_desc.uav_uint_clear_color.e),
                                                                    0,
                                                                    null);
                    }

                    uav_gpu_desc_handle.ptr += graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_size;
                    uav_cpu_desc_handle.ptr += graphics_shader.desc_heap_table.CBV_SRV_UAV_desc_size;
                }
                else
                {
                    REV_WARNING_M("Incomplete UAVs' clearing for buffers");
                }
            }
        }

        if (graphics_shader.bound_resources.sampler_count)
        {
            graphics_list->SetGraphicsRootDescriptorTable(root_parameter_index, graphics_shader.desc_heap_table.sampler_gpu_desc_handle);
        }
    }

    if (graphics_shader.bound_resources.RTV_count)
    {
        if (graphics_shader.bound_resources.DSV)
        {
            graphics_list->OMSetRenderTargets(graphics_shader.bound_resources.RTV_count,
                                              &graphics_shader.desc_heap_table.RTV_cpu_desc_handle,
                                              true,
                                              &graphics_shader.desc_heap_table.DSV_cpu_desc_handle);
        }
        else
        {
            graphics_list->OMSetRenderTargets(graphics_shader.bound_resources.RTV_count,
                                              &graphics_shader.desc_heap_table.RTV_cpu_desc_handle,
                                              true,
                                              null);
        }

        //
        // Change render targets' states
        //
        D3D12_RESOURCE_BARRIER *barriers    = memory->PushToFA<D3D12_RESOURCE_BARRIER>(graphics_shader.bound_resources.RTV_count);
        D3D12_RESOURCE_BARRIER *barriers_it = barriers;

        for (ResourcesTableNode *RTV = graphics_shader.bound_resources.RTV_first; RTV; RTV = RTV->next)
        {
            Texture& texture = memory_manager->GetTexture(RTV->resoucre_desc.resource);

            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.pResource   = texture.default_gpu_mem;
            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

            ++barriers_it;
        }

        graphics_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);

        //
        // Clear render targets
        //
        D3D12_CPU_DESCRIPTOR_HANDLE shader_handles_it = graphics_shader.desc_heap_table.RTV_cpu_desc_handle;
        for (ResourcesTableNode *RTV = graphics_shader.bound_resources.RTV_first; RTV; RTV = RTV->next)
        {
            graphics_list->ClearRenderTargetView(shader_handles_it, RTV->resoucre_desc.rtv_clear_color.e, 0, null);
            shader_handles_it.ptr += device_context->RTVDescSize();
        }
    }
    else if (graphics_shader.bound_resources.DSV)
    {
        Texture& texture = memory_manager->GetTexture(graphics_shader.bound_resources.DSV.resoucre_desc.resource);
        
        barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers_it->Transition.pResource   = texture.default_gpu_mem;
        barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_READ;
        
        ++barriers_it;
    }
}

void ShaderManager::BindVertexBuffer(const GraphicsShader& graphics_shader, const GPU::ResourceHandle& resource)
{
    REV_CHECK_M(resource.kind == GPU::RESOURCE_KIND_VERTEX_BUFFER, "Resource #%I64u is not a vertex buffer", resource.index);

    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());
    Buffer&        buffer         = memory_manager->GetBuffer(resource);

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = memory_manager->GetBufferGPUVirtualAddress(resource);
    vbv.SizeInBytes    = cast(u32, buffer.actual_size);
    vbv.StrideInBytes  = buffer.stride;

    cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->CurrentGraphicsList()->IASetVertexBuffers(0, 1, &vbv);
}

void ShaderManager::BindIndexBuffer(GraphicsShader& graphics_shader, const GPU::ResourceHandle& resource)
{
    REV_CHECK_M(resource.kind == GPU::RESOURCE_KIND_INDEX_BUFFER, "Resource #%I64u is not an index buffer", resource.index);

    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());
    Buffer&        buffer         = memory_manager->GetBuffer(resource);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = memory_manager->GetBufferGPUVirtualAddress(resource);
    ibv.SizeInBytes    = cast(u32, buffer.actual_size);
    ibv.Format         = buffer.format;

    cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->CurrentGraphicsList()->IASetIndexBuffer(&ibv);

    graphics_shader.index_buffer = resource;
}

ConstArray<GPU::ResourceHandle> ShaderManager::GetLoadableResources(const GraphicsShader& graphics_shader)
{
    u64 resources_count = 0;
    
    for (ResourceTableNode *CBV = graphics_shader.bound_resources.CBV_first; CBV; CBV = CBV->next)
    {
        if (CBV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            ++resources_count;
        }
    }

    for (ResourceTableNode *SRV = graphics_shader.bound_resources.SRV_first; SRV; SRV = SRV->next)
    {
        if (SRV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            ++resources_count;
        }
    }

    for (ResourceTableNode *UAV = graphics_shader.bound_resources.UAV_first; UAV; UAV = UAV->next)
    {
        if (UAV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            ++resources_count;
        }
    }

    for (ResourceTableNode *RTV = graphics_shader.bound_resources.RTV_first; RTV; RTV = RTV->next)
    {
        if (RTV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            ++resources_count;
        }
    }

    if (graphics_shader.bound_resources.DSV)
    {
        if (graphics_shader.bound_resources.DSV.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            ++resources_count;
        }
    }

    GPU::ResourceHandle *resources    = Memory::Get()->PushToTA<GPU::ResourceHandle>(resources_count);
    GPU::ResourceHandle *resources_it = resources;

    for (ResourceTableNode *CBV = graphics_shader.bound_resources.CBV_first; CBV; CBV = CBV->next)
    {
        if (CBV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            *resources_it++ = CBV->resoucre_desc.resource;
        }
    }
    
    for (ResourceTableNode *SRV = graphics_shader.bound_resources.SRV_first; SRV; SRV = SRV->next)
    {
        if (SRV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            *resources_it++ = SRV->resoucre_desc.resource;
        }
    }
    
    for (ResourceTableNode *UAV = graphics_shader.bound_resources.UAV_first; UAV; UAV = UAV->next)
    {
        if (UAV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            *resources_it++ = UAV->resoucre_desc.resource;
        }
    }
    
    for (ResourceTableNode *RTV = graphics_shader.bound_resources.RTV_first; RTV; RTV = RTV->next)
    {
        if (RTV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            *resources_it++ = RTV->resoucre_desc.resource;
        }
    }

    if (graphics_shader.bound_resources.DSV)
    {
        if (graphics_shader.bound_resources.DSV.resource.flags & GPU::RESOURCE_FLAG_CPU_WRITE)
        {
            *resources_it++ = graphics_shader.bound_resources.DSV.resource;
        }
    }

    return ConstArray(resources, resources_count);
}

ConstArray<GPU::ResourceHandle> ShaderManager::GetStorableResources(const GraphicsShader& graphics_shader)
{
    u64 resources_count = 0;
    
    for (ResourceTableNode *CBV = graphics_shader.bound_resources.CBV_first; CBV; CBV = CBV->next)
    {
        if (CBV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            ++resources_count;
        }
    }

    for (ResourceTableNode *SRV = graphics_shader.bound_resources.SRV_first; SRV; SRV = SRV->next)
    {
        if (SRV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            ++resources_count;
        }
    }

    for (ResourceTableNode *UAV = graphics_shader.bound_resources.UAV_first; UAV; UAV = UAV->next)
    {
        if (UAV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            ++resources_count;
        }
    }

    for (ResourceTableNode *RTV = graphics_shader.bound_resources.RTV_first; RTV; RTV = RTV->next)
    {
        if (RTV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            ++resources_count;
        }
    }

    if (graphics_shader.bound_resources.DSV)
    {
        if (graphics_shader.bound_resources.DSV.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            ++resources_count;
        }
    }

    GPU::ResourceHandle *resources    = Memory::Get()->PushToTA<GPU::ResourceHandle>(resources_count);
    GPU::ResourceHandle *resources_it = resources;

    for (ResourceTableNode *CBV = graphics_shader.bound_resources.CBV_first; CBV; CBV = CBV->next)
    {
        if (CBV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            *resources_it++ = CBV->resoucre_desc.resource;
        }
    }
    
    for (ResourceTableNode *SRV = graphics_shader.bound_resources.SRV_first; SRV; SRV = SRV->next)
    {
        if (SRV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            *resources_it++ = SRV->resoucre_desc.resource;
        }
    }
    
    for (ResourceTableNode *UAV = graphics_shader.bound_resources.UAV_first; UAV; UAV = UAV->next)
    {
        if (UAV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            *resources_it++ = UAV->resoucre_desc.resource;
        }
    }
    
    for (ResourceTableNode *RTV = graphics_shader.bound_resources.RTV_first; RTV; RTV = RTV->next)
    {
        if (RTV->resoucre_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            *resources_it++ = RTV->resoucre_desc.resource;
        }
    }

    if (graphics_shader.bound_resources.DSV)
    {
        if (graphics_shader.bound_resources.DSV.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            *resources_it++ = graphics_shader.bound_resources.DSV.resource;
        }
    }

    return ConstArray(resources, resources_count);
}

void ShaderManager::Draw(const GraphicsShader& graphics_shader)
{
    DeviceContext             *device_context = cast(DeviceContext *, GraphicsAPI::GetDeviceContext());
    MemoryManager             *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());
    ID3D12GraphicsCommandList *graphics_list  = device_context->CurrentGraphicsList();

    Buffer& buffer = memory_manager->GetBuffer(graphics_shader.index_buffer);
    graphics_list->DrawIndexedInstanced(cast(u32, buffer.actual_size / buffer.stride), 1, 0, 0, 0);

    u64 barriers_capacity = graphics_shader.bound_resources.RTV_count + graphics_shader.bound_resources.UAV_count + 1;
    if (barriers_capacity)
    {
        D3D12_RESOURCE_BARRIER *barriers    = Memory::Get()->PushToFA<D3D12_RESOURCE_BARRIER>(barriers_capacity);
        D3D12_RESOURCE_BARRIER *barriers_it = barriers;

        for (ResourcesTableNode *RTV = graphics_shader.bound_resources.RTV_first; RTV; RTV = RTV->next)
        {
            Texture& texture = memory_manager->GetTexture(RTV->resoucre_desc.resource);

            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.pResource   = texture.default_gpu_mem;
            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

            ++barriers_it;
        }

        if (graphics_shader.bound_resources.DSV)
        {
            Texture& texture = memory_manager->GetTexture(graphics_shader.bound_resources.DSV.resoucre_desc.resource);
            
            barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers_it->Transition.pResource   = texture.default_gpu_mem;
            barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_READ;
            
            ++barriers_it;
        }

        for (ResourcesTableNode *UAV = graphics_shader.bound_resources.UAV_first; UAV; UAV = UAV->next)
        {
            if (UAV->resoucre_desc.resource.kind == GPU::RESOURCE_KIND_TEXTURE)
            {
                Texture& texture = memory_manager->GetTexture(UAV->resoucre_desc.resource);

                barriers_it->Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barriers_it->Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                barriers_it->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barriers_it->Transition.pResource   = texture.default_gpu_mem;
                barriers_it->Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                barriers_it->Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

                ++barriers_it;
            }
            else
            {
                REV_WARNING_M("Incomplete UAVs' state changing for buffers");
            }
        }

        graphics_list->ResourceBarrier(cast(u32, barriers_it - barriers), barriers);
    }
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
    File file(shader_cache_filename, FILE_FLAG_RES);
    
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

void ShaderManager::InitBoundResources(GraphicsShader *graphics_shader, const ConstArray<GPU::ShaderResourceDesc>& resources)
{
    Memory *memory = Memory::Get();
    Arena  *arena  = graphics_shader->_static ? &memory->PermanentArena() : &memory->SceneArena();

    for (const GPU::ShaderResourceDesc& resource_desc : resources)
    {
        ResourcesTableNode *node = arena->Push<ResourcesTableNode>();
        node->resoucre_desc = resource_desc;

        if (resource_desc.resource.kind == GPU::RESOURCE_KIND_CONSTANT_BUFFER)
        {
            if (graphics_shader->bound_resources.CBV_first)
            {
                node->prev = graphics_shader->bound_resources.CBV_last;

                graphics_shader->bound_resources.CBV_last->next = node;
                graphics_shader->bound_resources.CBV_last       = node;
            }
            else
            {
                graphics_shader->bound_resources.CBV_first = node;
                graphics_shader->bound_resources.CBV_last  = graphics_shader->bound_resources.CBV_first;
            }
            ++graphics_shader->bound_resources.CBV_count;
        }
        else if (resource_desc.resource.kind == GPU::RESOURCE_KIND_SAMPLER)
        {
            if (graphics_shader->bound_resources.sampler_first)
            {
                node->prev = graphics_shader->bound_resources.sampler_first;

                graphics_shader->bound_resources.sampler_last->next = node;
                graphics_shader->bound_resources.sampler_last       = node;
            }
            else
            {
                graphics_shader->bound_resources.sampler_first = node;
                graphics_shader->bound_resources.sampler_last  = graphics_shader->bound_resources.sampler_first;
            }
            ++graphics_shader->bound_resources.sampler_count;
        }
        else if (resource_desc.resource.flags & GPU::RESOURCE_FLAG_RENDER_TARGET)
        {
            if (graphics_shader->bound_resources.RTV_first)
            {
                node->prev = graphics_shader->bound_resources.RTV_first;

                graphics_shader->bound_resources.RTV_last->next = node;
                graphics_shader->bound_resources.RTV_last       = node;
            }
            else
            {
                graphics_shader->bound_resources.RTV_first = node;
                graphics_shader->bound_resources.RTV_last  = graphics_shader->bound_resources.RTV_first;
            }
            ++graphics_shader->bound_resources.RTV_count;
        }
        else if (resource_desc.resource.flags & GPU::RESOURCE_FLAG_CPU_READ)
        {
            if (graphics_shader->bound_resources.UAV_first)
            {
                node->prev = graphics_shader->bound_resources.UAV_first;

                graphics_shader->bound_resources.UAV_last->next = node;
                graphics_shader->bound_resources.UAV_last       = node;
            }
            else
            {
                graphics_shader->bound_resources.UAV_first = node;
                graphics_shader->bound_resources.UAV_last  = graphics_shader->bound_resources.UAV_first;
            }
            ++graphics_shader->bound_resources.UAV_count;
        }
        else
        {
            if (graphics_shader->bound_resources.SRV_first)
            {
                node->prev = graphics_shader->bound_resources.SRV_first;

                graphics_shader->bound_resources.SRV_last->next = node;
                graphics_shader->bound_resources.SRV_last       = node;
            }
            else
            {
                graphics_shader->bound_resources.SRV_first = node;
                graphics_shader->bound_resources.SRV_last  = graphics_shader->bound_resources.SRV_first;
            }
            ++graphics_shader->bound_resources.SRV_count;
        }
    }
}

void ShaderManager::CreateDescHeaps(GraphicsShader *graphics_shader)
{
    ID3D12Device  *device         = cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->Device();
    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());

    u64 cbv_srv_uav_count = graphics_shader->bound_resources.CBV_count + graphics_shader->bound_resources.SRV_count + graphics_shader->bound_resources.UAV_count;
    if (cbv_srv_uav_count)
    {
        D3D12_DESCRIPTOR_HEAP_DESC cbv_srv_uav_desc_heap_desc = {};
        cbv_srv_uav_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbv_srv_uav_desc_heap_desc.NumDescriptors = cast(u32, cbv_srv_uav_count);
        cbv_srv_uav_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        cbv_srv_uav_desc_heap_desc.NodeMask       = 0;

        HRESULT error = device->CreateDescriptorHeap(&cbv_srv_uav_desc_heap_desc, IID_PPV_ARGS(&graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_heap));
        REV_CHECK(CheckResultAndPrintMessages(error));

        graphics_shader->desc_heap_table.CBV_SRV_UAV_gpu_desc_handle = graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_heap->GetGPUDescriptorHandleForHeapStart();
        graphics_shader->desc_heap_table.CBV_SRV_UAV_cpu_desc_handle = graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_heap->GetCPUDescriptorHandleForHeapStart();
        graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_size       = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    if (graphics_shader->bound_resources.RTV_count)
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc = {};
        rtv_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_desc_heap_desc.NumDescriptors = cast(u32, graphics_shader->bound_resources.RTV_count);
        rtv_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtv_desc_heap_desc.NodeMask       = 0;

        HRESULT error = device->CreateDescriptorHeap(&rtv_desc_heap_desc, IID_PPV_ARGS(&graphics_shader->desc_heap_table.RTV_desc_heap));
        REV_CHECK(CheckResultAndPrintMessages(error));

        graphics_shader->desc_heap_table.RTV_cpu_desc_handle = graphics_shader->desc_heap_table.RTV_desc_heap->GetCPUDescriptorHandleForHeapStart();
    }

    if (graphics_shader->bound_resources.sampler_count)
    {
        D3D12_DESCRIPTOR_HEAP_DESC sampler_desc_heap_desc = {};
        sampler_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        sampler_desc_heap_desc.NumDescriptors = cast(u32, graphics_shader->bound_resources.sampler_count);
        sampler_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        sampler_desc_heap_desc.NodeMask       = 0;

        HRESULT error = device->CreateDescriptorHeap(&sampler_desc_heap_desc, IID_PPV_ARGS(&graphics_shader->desc_heap_table.sampler_desc_heap));
        REV_CHECK(CheckResultAndPrintMessages(error));
        
        graphics_shader->desc_heap_table.sampler_gpu_desc_handle  = graphics_shader->desc_heap_table.sampler_desc_heap->GetGPUDescriptorHandleForHeapStart();
    }
}

void ShaderManager::CreateViews(GraphicsShader *graphics_shader)
{
    DeviceContext *device_context = cast(DeviceContext *, GraphicsAPI::GetDeviceContext());
    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());

    ID3D12Device *device = device_context->Device();

    u64 cbv_srv_uav_count = graphics_shader->bound_resources.CBV_count + graphics_shader->bound_resources.SRV_count + graphics_shader->bound_resources.UAV_count;
    if (cbv_srv_uav_count)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cbv_srv_uav_cpu_desc_handle = graphics_shader->desc_heap_table.CBV_SRV_UAV_cpu_desc_handle;

        for (ResourcesTableNode *CBV = graphics_shader->bound_resources.CBV_first; CBV; CBV = CBV->next)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
            cbv_desc.BufferLocation = memory_manager->GetBufferGPUVirtualAddress(CBV->resoucre_desc.resource);
            cbv_desc.SizeInBytes    = cast(u32, memory_manager->GetBuffer(CBV->resoucre_desc.resource).aligned_size);

            device->CreateConstantBufferView(&cbv_desc, cbv_srv_uav_cpu_desc_handle);
            cbv_srv_uav_cpu_desc_handle.ptr += graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_size;
        }

        for (ResourcesTableNode *SRV = graphics_shader->bound_resources.SRV_first; SRV; SRV = SRV->next)
        {
            if (SRV->resoucre_desc.resource.kind == GPU::RESOURCE_KIND_BUFFER)
            {
                Buffer& buffer = memory_manager->GetBuffer(SRV->resoucre_desc.resource);

                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                srv_desc.Format                     = buffer.format;
                srv_desc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
                srv_desc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srv_desc.Buffer.FirstElement        = buffer.default_page_offset / buffer.stride;
                srv_desc.Buffer.NumElements         = cast(u32, buffer.actual_size / buffer.stride);
                srv_desc.Buffer.StructureByteStride = buffer.stride;
                srv_desc.Buffer.Flags               = buffer.ro_rw_type == RO_RW_BUFFER_TYPE_BYTE_ADDRESS
                                                    ? D3D12_BUFFER_SRV_FLAG_RAW
                                                    : D3D12_BUFFER_SRV_FLAG_NONE;

                device->CreateShaderResourceView(memory_manager->GetBufferDefaultGPUMem(SRV->resoucre_desc.resource), &srv_desc, cbv_srv_uav_cpu_desc_handle);
                cbv_srv_uav_cpu_desc_handle.ptr += graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_size;
            }
            else
            {
                Texture& texture = memory_manager->GetTexture(SRV->resoucre_desc.resource);

                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
                srv_desc.Format                  = texture.format;
                srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

                switch (texture.dimension)
                {
                    case TEXTURE_DIMENSION_1D:
                    {
                        srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE1D;
                        srv_desc.Texture1D.MostDetailedMip     = 0;    // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                        srv_desc.Texture1D.MipLevels           = 1;    // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                        srv_desc.Texture1D.ResourceMinLODClamp = 0.5f; // scale bound
                    } break;

                    // @TODO(Roman): What about planar formats (YCbCr for example)?
                    //               Should we specify somehow what plane we want to use?
                    case TEXTURE_DIMENSION_2D:
                    {
                        srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srv_desc.Texture2D.MostDetailedMip     = 0;                  // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                        srv_desc.Texture2D.MipLevels           = texture.mip_levels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                        srv_desc.Texture2D.PlaneSlice          = 0;                  // The index (plane slice number) of the plane to use in the texture.
                        srv_desc.Texture2D.ResourceMinLODClamp = 0.5f;               // scale bound
                    } break;

                    case TEXTURE_DIMENSION_3D:
                    {
                        srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
                        srv_desc.Texture3D.MostDetailedMip     = 0;                  // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                        srv_desc.Texture3D.MipLevels           = texture.mip_levels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                        srv_desc.Texture3D.ResourceMinLODClamp = 0.5f;               // scale bound
                    } break;

                    case TEXTURE_DIMENSION_CUBE:
                    {
                        srv_desc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
                        srv_desc.TextureCube.MostDetailedMip     = 0;                  // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                        srv_desc.TextureCube.MipLevels           = texture.mip_levels; // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                        srv_desc.TextureCube.ResourceMinLODClamp = 0.5f;               // scale bound
                    } break;

                    case TEXTURE_DIMENSION_1D_ARRAY:
                    {
                        srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                        srv_desc.Texture1DArray.MostDetailedMip     = 0;                         // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                        srv_desc.Texture1DArray.MipLevels           = 1;                         // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                        srv_desc.Texture1DArray.FirstArraySlice     = 0;                         // First texture in an array to access [0, desc.DepthOrArraySize)
                        srv_desc.Texture1DArray.ArraySize           = texture.subtextures_count; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
                        srv_desc.Texture1DArray.ResourceMinLODClamp = 0.5f;                      // scale bound
                    } break;

                    // @TODO(Roman): What about planar formats (YCbCr for example)?
                    //               Should we specify somehow what plane we want to use?
                    case TEXTURE_DIMENSION_2D_ARRAY:
                    {
                        srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        srv_desc.Texture2DArray.MostDetailedMip     = 0;                         // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                        srv_desc.Texture2DArray.MipLevels           = texture.mip_levels;        // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                        srv_desc.Texture2DArray.FirstArraySlice     = 0;                         // First texture in an array to access [0, desc.DepthOrArraySize)
                        srv_desc.Texture2DArray.ArraySize           = texture.subtextures_count; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
                        srv_desc.Texture2DArray.PlaneSlice          = 0;                         // The index (plane slice number) of the plane to use in the texture.
                        srv_desc.Texture2DArray.ResourceMinLODClamp = 0.5f;                      // scale bound
                    } break;

                    case TEXTURE_DIMENSION_CUBE_ARRAY:
                    {
                        srv_desc.ViewDimension                        = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                        srv_desc.TextureCubeArray.MostDetailedMip     = 0;                         // Most detailed mip level index. We'are starting to filter a texture from this one. [0, desc.MipLevels)
                        srv_desc.TextureCubeArray.MipLevels           = texture.mip_levels;        // Number of mip levels to use in a filter. [1, desc.MipLevels - MostDetailedMip]
                        srv_desc.TextureCubeArray.First2DArrayFace    = 0;                         // First texture in an array to access [0, desc.DepthOrArraySize)
                        srv_desc.TextureCubeArray.NumCubes            = texture.subtextures_count; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
                        srv_desc.TextureCubeArray.ResourceMinLODClamp = 0.5f;                      // scale bound
                    } break;

                    default:
                    {
                        REV_ERROR_M("Invalid D3D12::TEXTURE_DIMENSION: %I32u", texture.dimension);
                    } break;
                }

                device->CreateShaderResourceView(texture.default_gpu_mem, &srv_desc, cbv_srv_uav_cpu_desc_handle);
                cbv_srv_uav_cpu_desc_handle.ptr += graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_size;
            }
        }

        for (ResourcesTableNode *UAV = graphics_shader->bound_resources.UAV_first; UAV; UAV = UAV->next)
        {
            if (UAV->resoucre_desc.resource.kind == GPU::RESOURCE_KIND_BUFFER)
            {
                Buffer& buffer = memory_manager->GetBuffer(UAV->resoucre_desc.resource);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
                uav_desc.Format                      = buffer.format;
                uav_desc.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
                uav_desc.Buffer.FirstElement         = buffer.default_page_offset / buffer.stride;
                uav_desc.Buffer.NumElements          = cast(u32, buffer.actual_size / buffer.stride);
                uav_desc.Buffer.StructureByteStride  = buffer.stride;
                uav_desc.Buffer.CounterOffsetInBytes = 0;
                uav_desc.Buffer.Flags                = buffer.ro_rw_type == RO_RW_BUFFER_TYPE_BYTE_ADDRESS
                                                     ? D3D12_BUFFER_UAV_FLAG_RAW
                                                     : D3D12_BUFFER_UAV_FLAG_NONE;

                device->CreateUnorderedAccessView(memory_manager->GetBufferDefaultGPUMem(UAV->resoucre_desc.resource), null, &uav_desc, cbv_srv_uav_cpu_desc_handle);
                cbv_srv_uav_cpu_desc_handle.ptr += graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_size;
            }
            else
            {
                Texture& texture = memory_manager->GetTexture(UAV->resoucre_desc.resource);

                D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
                uav_desc.Format = texture.format;

                switch (texture.dimension)
                {
                    case TEXTURE_DIMENSION_1D:
                    {
                        uav_desc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
                        uav_desc.Texture1D.MipSlice = 0; // A mip slice includes one mip level for EVERY texture in an array.
                                                         // So we choose wich one to use in view.
                    } break;

                    // @TODO(Roman): What about planar formats (YCbCr for example)?
                    //               Should we specify somehow which one plane we want to use?
                    case TEXTURE_DIMENSION_2D:
                    {
                        uav_desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
                        uav_desc.Texture2D.MipSlice   = 0; // A mip slice includes one mipmap level for EVERY texture in an array.
                                                           // So we choose wich one to use in view.
                        uav_desc.Texture2D.PlaneSlice = 0; // The index (plane slice number) of the plane to use in the view.
                    } break;

                    case TEXTURE_DIMENSION_3D:
                    {
                        uav_desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE3D;
                        uav_desc.Texture3D.MipSlice    = 0;             // A mip slice includes one mipmap level for EVERY texture in an array.
                                                                        // So we choose wich one to use in view.
                        uav_desc.Texture3D.FirstWSlice = 0;             // The zero-based index of the first depth slice to be accessed.
                        uav_desc.Texture3D.WSize       = texture.depth; // The number of depth slices.
                    } break;

                    case TEXTURE_DIMENSION_1D_ARRAY:
                    {
                        uav_desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                        uav_desc.Texture1DArray.MipSlice        = 0;                         // A mip slice includes one mipmap level for EVERY texture
                                                                                             // in an array. So we choose wich one to use in view.
                        uav_desc.Texture1DArray.FirstArraySlice = 0;                         // First texture in an array to access [0, desc.DepthOrArraySize)
                        uav_desc.Texture1DArray.ArraySize       = texture.subtextures_count; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
                    } break;

                    // @TODO(Roman): What about planar formats (YCbCr for example)?
                    //               Should we specify somehow which one plane we want to use?
                    case TEXTURE_DIMENSION_2D_ARRAY:
                    {
                        uav_desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                        uav_desc.Texture1DArray.MipSlice        = 0;                         // A mip slice includes one mipmap level for EVERY texture in an array.
                                                                                             // So we choose wich one to use in view.
                        uav_desc.Texture2DArray.FirstArraySlice = 0;                         // First texture in an array to access [0, desc.DepthOrArraySize)
                        uav_desc.Texture2DArray.ArraySize       = texture.subtextures_count; // Numbre of textrues in array. [1, desc.DepthOrArraySize]
                        uav_desc.Texture2DArray.PlaneSlice      = 0;                         // The index (plane slice number) of the plane to use in the texture.
                    } break;

                    default:
                    {
                        REV_ERROR_M("Invalid TEXTURE_DIMENSION: %I32u", texture.dimension);
                    } break;
                }

                device->CreateUnorderedAccessView(texture.default_gpu_mem, null, &uav_desc, cbv_srv_uav_cpu_desc_handle);
                cbv_srv_uav_cpu_desc_handle.ptr += graphics_shader->desc_heap_table.CBV_SRV_UAV_desc_size;
            }
        }
    }

    if (graphics_shader->bound_resources.RTV_count)
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpu_descriptor_handle = graphics_shader->desc_heap_table.RTV_cpu_desc_handle;

        for (ResourcesTableNode *RTV = graphics_shader->bound_resources.RTV_first; RTV; RTV = RTV->next)
        {
            if (RTV->resoucre_desc.resource.kind == GPU::RESOURCE_KIND_BUFFER)
            {
                Buffer& buffer = memory_manager->GetBuffer(RTV->resoucre_desc.resource);

                D3D12_RENDER_TARGET_VIEW_DESC rtv_desc{};
                rtv_desc.Format              = buffer.format;
                rtv_desc.ViewDimension       = D3D12_RTV_DIMENSION_BUFFER;
                rtv_desc.Buffer.FirstElement = buffer.default_page_offset / buffer.stride;
                rtv_desc.Buffer.NumElements  = cast(u32, buffer.actual_size / buffer.stride);

                device->CreateRenderTargetView(memory_manager->GetBufferDefaultGPUMem(RTV->resoucre_desc.resource), &rtv_desc, rtv_cpu_descriptor_handle);
                rtv_cpu_descriptor_handle.ptr += device_context->RTVDescSize();
            }
            else
            {
                Texture& texture = memory_manager->GetTexture(RTV->resoucre_desc.resource);

                D3D12_RENDER_TARGET_VIEW_DESC rtv_desc{};
                rtv_desc.Format = texture.format;

                switch (texture.dimension)
                {
                    case TEXTURE_DIMENSION_1D:
                    {
                        rtv_desc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE1D;
                        rtv_desc.Texture1D.MipSlice = 0;
                    } break;

                    // @TODO(Roman): What about planar formats (YCbCr for example)?
                    //               Should we specify somehow which one plane we want to use?
                    case TEXTURE_DIMENSION_2D:
                    {
                        rtv_desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
                        rtv_desc.Texture2D.MipSlice   = 0;
                        rtv_desc.Texture2D.PlaneSlice = 0;
                    } break;

                    case TEXTURE_DIMENSION_3D:
                    {
                        rtv_desc.ViewDimension         = D3D12_RTV_DIMENSION_TEXTURE3D;
                        rtv_desc.Texture3D.MipSlice    = 0;
                        rtv_desc.Texture3D.FirstWSlice = 0;
                        rtv_desc.Texture3D.WSize       = texture.depth;
                    } break;

                    case TEXTURE_DIMENSION_1D_ARRAY:
                    {
                        rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                        rtv_desc.Texture1DArray.MipSlice        = 0;
                        rtv_desc.Texture1DArray.FirstArraySlice = 0;
                        rtv_desc.Texture1DArray.ArraySize       = texture.subtextures_count;
                    } break;

                    // @TODO(Roman): What about planar formats (YCbCr for example)?
                    //               Should we specify somehow which one plane we want to use?
                    case TEXTURE_DIMENSION_2D_ARRAY:
                    {
                        rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                        rtv_desc.Texture2DArray.MipSlice        = 0;
                        rtv_desc.Texture2DArray.FirstArraySlice = 0;
                        rtv_desc.Texture2DArray.ArraySize       = texture.subtextures_count;
                        rtv_desc.Texture2DArray.PlaneSlice      = 0;
                    } break;

                    default:
                    {
                        REV_ERROR_M("Invalid D3D12::TEXTURE_DIMENSION: %I32u", texture.dimension);
                    } break;
                }

                device->CreateRenderTargetView(texture.default_gpu_mem, &rtv_desc, rtv_cpu_descriptor_handle);
                rtv_cpu_descriptor_handle.ptr += device_context->RTVDescSize();
            }
        }
    }

    if (graphics_shader->bound_resources.sampler_count)
    {
        u32                         sampler_descriptor_size       = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        D3D12_CPU_DESCRIPTOR_HANDLE sampler_cpu_descriptor_handle = graphics_shader->desc_heap_table.sampler_desc_heap->GetCPUDescriptorHandleForHeapStart();

        for (ResourcesTableNode *it = graphics_shader->bound_resources.sampler_first; it; it = it->next)
        {
            Sampler& sampler = memory_manager->GetSampler(it->resoucre_desc.resource);

            device->CreateSampler(&sampler.desc, sampler_cpu_descriptor_handle);
            sampler_cpu_descriptor_handle.ptr += sampler_descriptor_size;
        }
    }
}

ConstArray<D3D12_ROOT_PARAMETER> ShaderManager::CreateRootSignatureParameters(GraphicsShader *graphics_shader)
{
    Memory *memory = Memory::Get();

    u32 num_root_parameters = 0;
    if (graphics_shader->bound_resources.CBV_count)     ++num_root_parameters;
    if (graphics_shader->bound_resources.SRV_count)     ++num_root_parameters;
    if (graphics_shader->bound_resources.UAV_count)     ++num_root_parameters;
    if (graphics_shader->bound_resources.sampler_count) ++num_root_parameters;

    D3D12_ROOT_PARAMETER *root_parameters    = memory->PushToFA<D3D12_ROOT_PARAMETER>(num_root_parameters);
    D3D12_ROOT_PARAMETER *root_parameters_it = root_parameters;

    if (graphics_shader->bound_resources.CBV_count)
    {
        root_parameters_it->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_parameters_it->DescriptorTable.NumDescriptorRanges = cast(u32, graphics_shader->bound_resources.CBV_count);
        root_parameters_it->ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_DESCRIPTOR_RANGE *ranges    = memory->PushToFA<D3D12_DESCRIPTOR_RANGE>(graphics_shader->bound_resources.CBV_count);
        D3D12_DESCRIPTOR_RANGE *ranges_it = ranges;

        for (ResourcesTableNode *CBV = graphics_shader->bound_resources.CBV_first; CBV; CBV = CBV->next)
        {
            ranges_it->RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
            ranges_it->NumDescriptors                    = 1;
            ranges_it->BaseShaderRegister                = CBV->resoucre_desc.cbv_srv_sampler_shader_register;
            ranges_it->RegisterSpace                     = CBV->resoucre_desc.cbv_srv_sampler_register_space;
            ranges_it->OffsetInDescriptorsFromTableStart = cast(u32, ranges_it - ranges);

            ++ranges_it;
        }

        root_parameters_it->DescriptorTable.pDescriptorRanges = ranges;

        ++root_parameters_it;
    }

    if (graphics_shader->bound_resources.SRV_count)
    {
        root_parameters_it->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_parameters_it->DescriptorTable.NumDescriptorRanges = cast(u32, graphics_shader->bound_resources.SRV_count);
        root_parameters_it->ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_DESCRIPTOR_RANGE *ranges    = memory->PushToFA<D3D12_DESCRIPTOR_RANGE>(graphics_shader->bound_resources.SRV_count);
        D3D12_DESCRIPTOR_RANGE *ranges_it = ranges;

        for (ResourcesTableNode *SRV = graphics_shader->bound_resources.SRV_first; SRV; SRV = SRV->next)
        {
            ranges_it->RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            ranges_it->NumDescriptors                    = 1;
            ranges_it->BaseShaderRegister                = SRV->resoucre_desc.cbv_srv_sampler_shader_register;
            ranges_it->RegisterSpace                     = SRV->resoucre_desc.cbv_srv_sampler_register_space;
            ranges_it->OffsetInDescriptorsFromTableStart = cast(u32, ranges_it - ranges);

            ++ranges_it;
        }

        root_parameters_it->DescriptorTable.pDescriptorRanges = ranges;

        ++root_parameters_it;
    }

    if (graphics_shader->bound_resources.UAV_count)
    {
        root_parameters_it->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_parameters_it->DescriptorTable.NumDescriptorRanges = cast(u32, graphics_shader->bound_resources.UAV_count);
        root_parameters_it->ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_DESCRIPTOR_RANGE *ranges    = memory->PushToFA<D3D12_DESCRIPTOR_RANGE>(graphics_shader->bound_resources.UAV_count);
        D3D12_DESCRIPTOR_RANGE *ranges_it = ranges;

        for (ResourcesTableNode *UAV = graphics_shader->bound_resources.UAV_first; UAV; UAV = UAV->next)
        {
            ranges_it->RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
            ranges_it->NumDescriptors                    = 1;
            ranges_it->BaseShaderRegister                = UAV->resoucre_desc.uav_shader_register;
            ranges_it->RegisterSpace                     = UAV->resoucre_desc.uav_register_space;
            ranges_it->OffsetInDescriptorsFromTableStart = cast(u32, ranges_it - ranges);

            ++ranges_it;
        }

        root_parameters_it->DescriptorTable.pDescriptorRanges = ranges;

        ++root_parameters_it;
    }

    if (graphics_shader->bound_resources.sampler_count)
    {
        root_parameters_it->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        root_parameters_it->DescriptorTable.NumDescriptorRanges = cast(u32, graphics_shader->bound_resources.sampler_count);
        root_parameters_it->ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_DESCRIPTOR_RANGE *ranges    = memory->PushToFA<D3D12_DESCRIPTOR_RANGE>(graphics_shader->bound_resources.sampler_count);
        D3D12_DESCRIPTOR_RANGE *ranges_it = ranges;

        for (ResourcesTableNode *sampler = graphics_shader->bound_resources.sampler_first; sampler; sampler = sampler->next)
        {
            ranges_it->RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            ranges_it->NumDescriptors                    = 1;
            ranges_it->BaseShaderRegister                = sampler->resoucre_desc.cbv_srv_sampler_shader_register;
            ranges_it->RegisterSpace                     = sampler->resoucre_desc.cbv_srv_sampler_register_space;
            ranges_it->OffsetInDescriptorsFromTableStart = cast(u32, ranges_it - ranges);

            ++ranges_it;
        }

        root_parameters_it->DescriptorTable.pDescriptorRanges = ranges;

        ++root_parameters_it;
    }

    return ConstArray(root_parameters, num_root_parameters);
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
    MemoryManager *memory_manager = cast(MemoryManager *, GraphicsAPI::GetMemoryManager());

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
        blend_desc.RenderTarget[graphics_shader->bound_resources.RTV_count + 1] = rtbd_sum;
        blend_desc.RenderTarget[graphics_shader->bound_resources.RTV_count + 2] = rtbd_mul;
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
    gpsd.IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // @TODO(Roman): Support triangle strips
    gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    {
        gpsd.RTVFormats[gpsd.NumRenderTargets++] = DXGI_FORMAT_R8G8B8A8_UNORM;
        for (ResourcesTableNode *RTV = graphics_shader->bound_resources.RTV_first; RTV; RTV = RTV->next)
        {
            if (RTV->resoucre_desc.resource.kind == GPU::RESOURCE_KIND_BUFFER)
            {
                Buffer& buffer = memory_manager->GetBuffer(RTV->resoucre_desc.resource);
                gpsd.RTVFormats[gpsd.NumRenderTargets++] = buffer.format;
            }
            else
            {
                Texture& texture = memory_manager->GetTexture(RTV->resoucre_desc.resource);
                gpsd.RTVFormats[gpsd.NumRenderTargets++] = texture.format;
            }
        }
        if (blending_enabled)
        {
            gpsd.RTVFormats[gpsd.NumRenderTargets++] = DXGI_FORMAT_R32G32B32A32_FLOAT;
            gpsd.RTVFormats[gpsd.NumRenderTargets++] = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }
    gpsd.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
    gpsd.SampleDesc.Count      = 1;
    gpsd.SampleDesc.Quality    = 0;
    gpsd.NodeMask              = 0;
    gpsd.CachedPSO             = cached_pipeline_state;
    gpsd.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

    ID3D12Device *device = cast(DeviceContext *, GraphicsAPI::GetDeviceContext())->Device();

    HRESULT error = device->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&graphics_shader->pipeline_state));
    REV_CHECK(CheckResultAndPrintMessages(error));
}

}
