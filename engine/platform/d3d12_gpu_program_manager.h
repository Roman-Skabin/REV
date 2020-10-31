//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "renderer/gpu_program_manager.h"
#include "core/memory.h"
#include "tools/list.hpp"
#include "platform/d3d12_gpu_memory_manager.h"

#include <d3dcompiler.h>

namespace D3D12
{
    class ENGINE_API Shader final
    {
    public:
        Shader();
        ~Shader();

        void Compile(const char *shader, u64 length, const char *name, const char *entry_point, const char *target);

        constexpr const ID3DBlob              *Blob()     const { return m_Blob;     }
        constexpr const D3D12_SHADER_BYTECODE& Bytecode() const { return m_Bytecode; }

        constexpr ID3DBlob              *Blob()     { return m_Blob;     }
        constexpr D3D12_SHADER_BYTECODE& Bytecode() { return m_Bytecode; }

        Shader& operator=(Shader&& other) noexcept;

    private:
        Shader(const Shader&) = delete;
        Shader(Shader&&)      = delete;

        Shader& operator=(const Shader&) = delete;

    private:
        ID3DBlob              *m_Blob;
        D3D12_SHADER_BYTECODE  m_Bytecode;
    };

    class ENGINE_API GraphicsProgram final : public IGraphicsProgram
    {
    public:
        using ResourcePointersList = List<GPUResource *>;

        GraphicsProgram(Allocator *allocator, const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename);
        ~GraphicsProgram();

        virtual void AttachHullShader(const StaticString<MAX_PATH>& filename)     override;
        virtual void AttachDomainShader(const StaticString<MAX_PATH>& filename)   override;
        virtual void AttachGeometryShader(const StaticString<MAX_PATH>& filename) override;

        virtual void BindResource(IGPUResource *resource) override;

        virtual void BindVertexBuffer(IGPUResource *resource) override;
        virtual void BindIndexBuffer(IGPUResource *resource)  override;

        virtual void DrawVertices() override;
        virtual void DrawIndices()  override;

        constexpr const ID3DBlob             *Signature()     const  { return m_Signature;      }
        constexpr const ID3D12RootSignature  *RootSignature() const  { return m_RootSignature;  }
        constexpr const ID3D12PipelineState  *PipelineState() const  { return m_PipelineState;  }
        constexpr const ResourcePointersList& BoundResources() const { return m_BoundResources; }

        constexpr ID3DBlob             *Signature()      { return m_Signature;      }
        constexpr ID3D12RootSignature  *RootSignature()  { return m_RootSignature;  }
        constexpr ID3D12PipelineState  *PipelineState()  { return m_PipelineState;  }
        constexpr ResourcePointersList& BoundResources() { return m_BoundResources; }

    private:
        void AttachMainShaders(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename);

        void CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc);
        void CreatePipelineState(
            bool                           blending_enabled,
            D3D12_CULL_MODE                cull_mode,
            bool                           depth_test_enabled,
            const D3D12_INPUT_LAYOUT_DESC& input_layout
        );

    public:
        GraphicsProgram& operator=(GraphicsProgram&& other) noexcept;

    private:
        GraphicsProgram(const GraphicsProgram&) = delete;
        GraphicsProgram(GraphicsProgram&&)      = delete;

        GraphicsProgram& operator=(const GraphicsProgram&) = delete;

    private:
        ID3DBlob             *m_Signature;
        ID3D12RootSignature  *m_RootSignature;
        ID3D12PipelineState  *m_PipelineState;
        GPUResource          *m_VertexBuffer;
        GPUResource          *m_IndexBuffer;
        ResourcePointersList  m_BoundResources;
        Shader                m_VertexShader;
        Shader                m_PixelShader;
        Shader                m_HullShader;
        Shader                m_DomainShader;
        Shader                m_GeometryShader;
    };

    class ENGINE_API ComputeProgram final : public IComputeProgram
    {
    public:
        ComputeProgram();
        ~ComputeProgram();
    private:
    };

    class ENGINE_API GPUProgramManager final : public IGPUProgramManager
    {
    public:
        using GraphicsProgramList = List<GraphicsProgram>;
        using ComputeProgramList  = List<ComputeProgram>;

        GPUProgramManager(Allocator *allocator);
        ~GPUProgramManager();

        virtual void Destroy() override;

        virtual IGraphicsProgram *CreateGraphicsProgram(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename) override;

        virtual void SetCurrentGraphicsProgram(IGraphicsProgram *graphics_program) override;

        void *operator new(size_t)   { return Memory::Get()->PushToPA<GPUProgramManager>(); }
        void operator delete(void *) {}

    private:
        Allocator           *m_Allocator;
        GraphicsProgramList  m_GraphicsPrograms;
        ComputeProgramList   m_ComputePrograms;
    };
}
