// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/shader_manager.h"
#include "tools/array.hpp"

#include "platform/d3d12/d3d12_memory_manager.h"

namespace REV::D3D12
{
    struct CBV_SRV_UAV_Sampler_TableNode final
    {
        CBV_SRV_UAV_Sampler_TableNode *next          = null;
        CBV_SRV_UAV_Sampler_TableNode *prev          = null;
        GPU::ShaderResourceDesc        resoucre_desc;
    };

    struct CBV_SRV_UAV_Sampler_Table final
    {
        CBV_SRV_UAV_Sampler_TableNode *CBV_first     = null;
        CBV_SRV_UAV_Sampler_TableNode *CBV_last      = null;
        u64                            CBV_count     = 0;
        CBV_SRV_UAV_Sampler_TableNode *SRV_first     = null;
        CBV_SRV_UAV_Sampler_TableNode *SRV_last      = null;
        u64                            SRV_count     = 0;
        CBV_SRV_UAV_Sampler_TableNode *UAV_first     = null;
        CBV_SRV_UAV_Sampler_TableNode *UAV_last      = null;
        u64                            UAV_count     = 0;
        CBV_SRV_UAV_Sampler_TableNode *sampler_first = null;
        CBV_SRV_UAV_Sampler_TableNode *sampler_last  = null;
        u64                            sampler_count = 0;
    };

    struct GraphicsShader final
    {
        ID3DBlob                    *signature;
        ID3D12RootSignature         *root_signature;
        ID3D12PipelineState         *pipeline_state;
        CBV_SRV_UAV_Sampler_Table    bound_resources;
        GPU::ResourceHandle          index_buffer;
        ID3DBlob                    *vertex_shader;
        ID3DBlob                    *pixel_shader;
        ID3DBlob                    *hull_shader;
        ID3DBlob                    *domain_shader;
        ID3DBlob                    *geometry_shader;
        ID3D12DescriptorHeap        *cbv_srv_uav_desc_heap;
        u32                          cbv_srv_uav_desc_size;
        D3D12_GPU_DESCRIPTOR_HANDLE  cbv_srv_uav_gpu_desc_handle;
        ID3D12DescriptorHeap        *sampler_desc_heap;
        D3D12_GPU_DESCRIPTOR_HANDLE  sampler_gpu_desc_handle;
        bool                         _static;
    };

    struct ComputeShader final
    {
        ID3DBlob                    *signature;
        ID3D12RootSignature         *root_signature;
        ID3D12PipelineState         *pipeline_state;
        CBV_SRV_UAV_Sampler_Table    bound_resources;
        ID3DBlob                    *shader;
        ID3D12DescriptorHeap        *cbv_srv_uav_desc_heap;
        u32                          cbv_srv_uav_desc_heap_size;
        D3D12_CPU_DESCRIPTOR_HANDLE  cbv_srv_uav_desc_heap_cpu_desc_handle;
        D3D12_GPU_DESCRIPTOR_HANDLE  cbv_srv_uav_desc_heap_gpu_desc_handle;
        ID3D12DescriptorHeap        *sampler_desc_heap;
        bool                         _static;
    };

    class ShaderManager final
    {
    public:
        ShaderManager(Allocator *allocator, const Logger& logger);
        ~ShaderManager();

        void FreeSceneShaders();

        u64 CreateGraphicsShader(const ConstString& shader_cache_filename, const ConstArray<GPU::ShaderResourceDesc>& resources, bool _static);

        void SetCurrentGraphicsShader(const GraphicsShader& graphics_shader);

        void BindVertexBuffer(GraphicsShader& graphics_shader, const GPU::ResourceHandle& resource);
        void BindIndexBuffer(GraphicsShader& graphics_shader, const GPU::ResourceHandle& resource);

        void Draw(const GraphicsShader& graphics_shader);

        ID3DBlob *CompileShader(const ConstString& hlsl_code, const char *name, const char *entry_point, const char *target);

        #pragma region inline_getters
        REV_INLINE const GraphicsShader& GetGraphicsShader(const GPU::ShaderHandle& shader_handle) const
        {
            return shader_handle._static
                 ? m_StaticGraphicsShaders[shader_handle.index]
                 : m_SceneGraphicsShaders[shader_handle.index];
        }

        REV_INLINE const ComputeShader& GetComputeShader(const GPU::ShaderHandle& shader_handle) const
        {
            return shader_handle._static
                 ? m_StaticComputeShaders[shader_handle.index]
                 : m_SceneComputeShaders[shader_handle.index];
        }

        REV_INLINE GraphicsShader& GetGraphicsShader(const GPU::ShaderHandle& shader_handle)
        {
            return shader_handle._static
                 ? m_StaticGraphicsShaders[shader_handle.index]
                 : m_SceneGraphicsShaders[shader_handle.index];
        }

        REV_INLINE ComputeShader& GetComputeShader(const GPU::ShaderHandle& shader_handle)
        {
            return shader_handle._static
                 ? m_StaticComputeShaders[shader_handle.index]
                 : m_SceneComputeShaders[shader_handle.index];
        }

        REV_INLINE const Logger& GetLogger() const { return m_Logger; }
        REV_INLINE       Logger& GetLogger()       { return m_Logger; }
        #pragma endregion inline_getters

    private:
        void LoadShaderCache(GraphicsShader *graphics_shader, const ConstString& shader_cache_filename);
        void InitBoundResources(GraphicsShader *graphics_shader, const ConstArray<GPU::ShaderResourceDesc>& resources);
        void CreateDescHeapsAndViews(GraphicsShader *graphics_shader);
        ConstArray<D3D12_ROOT_PARAMETER> CreateRootSignatureParameters(GraphicsShader *graphics_shader);
        void CreateRootSignature(GraphicsShader *graphics_shader, const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc);
        void CreatePipelineState(GraphicsShader *graphics_shader, const D3D12_INPUT_LAYOUT_DESC& input_layout, bool blending_enabled, D3D12_CULL_MODE cull_mode, bool depth_test_enabled);

        REV_DELETE_CONSTRS_AND_OPS(ShaderManager);

    private:
        Array<GraphicsShader>  m_StaticGraphicsShaders;
        Array<ComputeShader>   m_StaticComputeShaders;
        Array<GraphicsShader>  m_SceneGraphicsShaders;
        Array<ComputeShader>   m_SceneComputeShaders;
        Logger                 m_Logger;
    };
}
