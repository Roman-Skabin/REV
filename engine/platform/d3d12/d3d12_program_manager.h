//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "graphics/program_manager.h"
#include "core/memory.h"
#include "tools/list.hpp"
#include "tools/buffer.hpp"
#include "platform/d3d12/d3d12_memory_manager.h"

#include <d3d12.h>

namespace D3D12
{
    struct ENGINE_API GraphicsProgram final
    {
        ID3DBlob                      *signature;
        ID3D12RootSignature           *root_signature;
        ID3D12PipelineState           *pipeline_state;
        GPU::ResourceHandle            vertex_buffer;   // @CleanUp(Roman): redundant?
        GPU::ResourceHandle            index_buffer;    // @CleanUp(Roman): redundant?
        ::Buffer<GPU::ResourceHandle>  bound_resources; // @CleanUp(Roman): redundant?
        ID3DBlob                      *vertex_shader;
        ID3DBlob                      *pixel_shader;
        ID3DBlob                      *hull_shader;
        ID3DBlob                      *domain_shader;
        ID3DBlob                      *geometry_shader;

        GraphicsProgram(Allocator *allocator) : bound_resources(allocator) {}
        ~GraphicsProgram() {}

        GraphicsProgram& operator=(const GraphicsProgram&) { FailedM("GraphicsProgram movable obly"); return *this; }
        GraphicsProgram& operator=(GraphicsProgram&& other) noexcept;
    };

    struct ENGINE_API ComputeProgram final
    {
        ID3DBlob                      *signature;
        ID3D12RootSignature           *root_signature;
        ID3D12PipelineState           *pipeline_state;
        ID3DBlob                      *shader;
        ::Buffer<GPU::ResourceHandle>  bound_resources; // @CleanUp(Roman): redundant?

        ComputeProgram(Allocator *allocator) : bound_resources(allocator) {}
        ~ComputeProgram() {}

        ComputeProgram& operator=(const ComputeProgram&) { FailedM("ComputeProgram movable obly"); return *this; }
        ComputeProgram& operator=(ComputeProgram&& other) noexcept;
    };

    class ENGINE_API ProgramManager final
    {
    public:
        ProgramManager(Allocator *allocator);
        ~ProgramManager();

        u64 CreateGraphicsProgram(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename);

        void SetCurrentGraphicsProgram(const GraphicsProgram& graphics_program);

        void AttachHullShader(GraphicsProgram& graphics_program, const StaticString<MAX_PATH>& filename);
        void AttachDomainShader(GraphicsProgram& graphics_program, const StaticString<MAX_PATH>& filename);
        void AttachGeometryShader(GraphicsProgram& graphics_program, const StaticString<MAX_PATH>& filename);

        void AttachResource(GraphicsProgram& graphics_program, GPU::ResourceHandle resource_handle);

        void BindVertexBuffer(GraphicsProgram& graphics_program, GPU::ResourceHandle resource_handle);
        void BindIndexBuffer(GraphicsProgram& graphics_program, GPU::ResourceHandle resource_handle);

        void DrawVertices(const GraphicsProgram& graphics_program);
        void DrawIndices(const GraphicsProgram& graphics_program);

        const GraphicsProgram& GetGraphicsProgram(u64 index) const { return m_GraphicsPrograms[index]; }
        const ComputeProgram&  GetComputeProgram(u64 index)  const { return m_ComputePrograms[index];  }

        GraphicsProgram& GetGraphicsProgram(u64 index) { return m_GraphicsPrograms[index]; }
        ComputeProgram&  GetComputeProgram(u64 index)  { return m_ComputePrograms[index];  }

        ProgramManager& operator=(ProgramManager&& other) noexcept;

    private:
        void AttachMainShaders(GraphicsProgram& graphics_program, const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename);
        void CreateRootSignature(GraphicsProgram& graphics_program, const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc);
        void CreatePipelineState(GraphicsProgram& graphics_program, const D3D12_INPUT_LAYOUT_DESC& input_layout, bool blending_enabled, D3D12_CULL_MODE cull_mode, bool depth_test_enabled);

        ID3DBlob *CompileShader(const char *hlsl_code, u32 hlsl_code_length, const char *name, const char *entry_point, const char *target);

        ProgramManager(const ProgramManager&) = delete;
        ProgramManager(ProgramManager&&)      = delete;

        ProgramManager& operator=(const ProgramManager&) = delete;

    private:
        Allocator                 *m_Allocator;
        ::Buffer<GraphicsProgram>  m_GraphicsPrograms;
        ::Buffer<ComputeProgram>   m_ComputePrograms;
    };
}
