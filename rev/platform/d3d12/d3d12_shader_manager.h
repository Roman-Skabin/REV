//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "graphics/shader_manager.h"
#include "core/memory.h"
#include "tools/list.hpp"
#include "tools/array.hpp"
#include "platform/d3d12/d3d12_memory_manager.h"

#include <d3d12.h>

namespace REV::D3D12
{
    struct GraphicsShader final
    {
        ID3DBlob                   *signature             = null;
        ID3D12RootSignature        *root_signature        = null;
        ID3D12PipelineState        *pipeline_state        = null;
        Array<GPU::ResourceHandle>  bound_resources;
        GPU::ResourceHandle         index_buffer;
        ID3DBlob                   *vertex_shader         = null;
        ID3DBlob                   *pixel_shader          = null;
        ID3DBlob                   *hull_shader           = null;
        ID3DBlob                   *domain_shader         = null;
        ID3DBlob                   *geometry_shader       = null;
        ID3D12DescriptorHeap       *cbv_srv_uav_desc_heap = null;
        ID3D12DescriptorHeap       *sampler_desc_heap     = null;
        bool                        _static               = false;

        REV_INLINE GraphicsShader(Allocator *allocator) : bound_resources(allocator) {}
        REV_INLINE ~GraphicsShader() {}

        REV_DELETE_CONSTRS_AND_OPS(GraphicsShader);
    };

    struct ComputeShader final
    {
        ID3DBlob                   *signature             = null;
        ID3D12RootSignature        *root_signature        = null;
        ID3D12PipelineState        *pipeline_state        = null;
        Array<GPU::ResourceHandle>  bound_resources;
        ID3DBlob                   *shader                = null;
        ID3D12DescriptorHeap       *cbv_srv_uav_desc_heap = null;
        ID3D12DescriptorHeap       *sampler_desc_heap     = null;
        bool                        _static               = false;

        REV_INLINE ComputeShader(Allocator *allocator) : bound_resources(allocator) {}
        REV_INLINE ~ComputeShader() {}

        REV_DELETE_CONSTRS_AND_OPS(ComputeShader);
    };

    class ShaderManager final
    {
    public:
        ShaderManager(Allocator *allocator, const Logger& logger);
        ~ShaderManager();

        void FreeSceneShaders();

        u64 CreateGraphicsShader(
            const ConstString&                  shader_cache_filename,
            const ConstArray<AssetHandle>&      textures,
            const ConstArray<GPU::CBufferDesc>& cbuffers,
            const ConstArray<GPU::SamplerDesc>& samplers,
            bool                                _static
        );

        void SetCurrentGraphicsShader(const GraphicsShader& graphics_shader);

        void BindVertexBuffer(GraphicsShader& graphics_shader, const GPU::ResourceHandle& resource_handle);
        void BindIndexBuffer(GraphicsShader& graphics_shader, const GPU::ResourceHandle& resource_handle);

        void Draw(const GraphicsShader& graphics_shader);

        ID3DBlob *CompileShader(const ConstString& hlsl_code, const char *name, const char *entry_point, const char *target);

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

    private:
        void LoadShaderCache(GraphicsShader *graphics_shader, const ConstString& shader_cache_filename);
        void CreateDescHeapsAndViews(GraphicsShader *graphics_shader, const ConstArray<AssetHandle>& textures, const ConstArray<GPU::CBufferDesc>& cbuffers, const ConstArray<GPU::SamplerDesc>& samplers);
        void CreateRootSignature(GraphicsShader *graphics_shader, const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc);
        void CreatePipelineState(GraphicsShader *graphics_shader, const D3D12_INPUT_LAYOUT_DESC& input_layout, bool blending_enabled, D3D12_CULL_MODE cull_mode, bool depth_test_enabled);

        REV_DELETE_CONSTRS_AND_OPS(ShaderManager);

    private:
        Allocator             *m_Allocator;
        Array<GraphicsShader>  m_StaticGraphicsShaders;
        Array<ComputeShader>   m_StaticComputeShaders;
        Array<GraphicsShader>  m_SceneGraphicsShaders;
        Array<ComputeShader>   m_SceneComputeShaders;
        Logger                 m_Logger;
    };
}
