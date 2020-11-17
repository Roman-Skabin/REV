//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "platform/d3d12_gpu_program_manager.h"
#include "platform/d3d12_renderer.h"
#include "renderer/graphics_api.h"
#include "tools/async_file.h"

namespace D3D12
{

//
// Shader
//

Shader::Shader()
    : m_Blob(null),
      m_Bytecode{null, 0}
{
}

Shader::~Shader()
{
    SafeRelease(m_Blob);
}

void Shader::Compile(const char *shader, u64 length, const char *name, const char *entry_point, const char *target)
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    u32 compile_flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#if DEBUG
    compile_flags |= D3DCOMPILE_DEBUG
                  |  D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3
                  |  D3DCOMPILE_SKIP_VALIDATION;
#endif

    if (renderer->HalfPrecisionSupported())
    {
        compile_flags |= D3DCOMPILE_PARTIAL_PRECISION;
    }

    ID3DBlob *errors = null;
    HRESULT error = D3DCompile(shader,
                               length,
                               name,
                               null,
                               null,
                               entry_point,
                               target,
                               compile_flags,
                               0,
                               &m_Blob,
                               &errors);
    if (FAILED(error))
    {
        FailedM("Shader compilation failure:\n\n%s", errors->GetBufferPointer());
    }
    SafeRelease(errors);

    m_Bytecode.pShaderBytecode = m_Blob->GetBufferPointer();
    m_Bytecode.BytecodeLength  = m_Blob->GetBufferSize();
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        m_Blob     = other.m_Blob;
        m_Bytecode = other.m_Bytecode;

        other.m_Blob                     = null;
        other.m_Bytecode.pShaderBytecode = null;
    }
    return *this;
}

//
// GraphicsProgram
//

GraphicsProgram::GraphicsProgram(Allocator *allocator, const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename)
    : m_Signature(null),
      m_RootSignature(null),
      m_PipelineState(null),
      m_VertexBuffer(null),
      m_IndexBuffer(null),
      m_BoundResources(allocator),
      m_VertexShader(),
      m_PixelShader(),
      m_HullShader(),
      m_DomainShader(),
      m_GeometryShader()
{
    AttachMainShaders(vs_filename, ps_filename);

    // @Temp(Roman): Will be set on shader parsing.
    D3D12_INPUT_ELEMENT_DESC elements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_INPUT_LAYOUT_DESC input_layout;
    input_layout.pInputElementDescs = elements;
    input_layout.NumElements        = cast<u32>(ArrayCount(elements));

    D3D12_ROOT_PARAMETER parameter;
    parameter.ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameter.Descriptor.ShaderRegister = 0;
    parameter.Descriptor.RegisterSpace  = 0;
    parameter.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.NumParameters     = 1;
    root_signature_desc.pParameters       = &parameter;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers   = null;
    root_signature_desc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // @TODO(Roman): Parse Shaders.

    CreateRootSignature(root_signature_desc);
    CreatePipelineState(false, D3D12_CULL_MODE_NONE, true, input_layout);
}

GraphicsProgram::~GraphicsProgram()
{
    SafeRelease(m_PipelineState);
    SafeRelease(m_RootSignature);
    SafeRelease(m_Signature);
}

void GraphicsProgram::AttachMainShaders(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename)
{
    Memory *memory = Memory::Get();

    AsyncFile vs_file(vs_filename, AsyncFile::FLAGS::READ);
    AsyncFile ps_file(ps_filename, AsyncFile::FLAGS::READ);

    u32 vs_length = vs_file.Size();
    u32 ps_length = ps_file.Size();

    char *vs = memory->PushToTA<char>(vs_length + 1);
    char *ps = memory->PushToTA<char>(ps_length + 1);

    vs_file.Read(vs, vs_length);
    ps_file.Read(ps, ps_length);

    AsyncFile::WaitForAll(vs_file, ps_file);

    m_VertexShader.Compile(vs, vs_length, vs_filename.Data(), "VSMain", "vs_5_1");
    m_PixelShader.Compile(ps,  ps_length, ps_filename.Data(), "PSMain", "ps_5_1");
}

void GraphicsProgram::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC vrsd = {};
    vrsd.Version = D3D_ROOT_SIGNATURE_VERSION_1_0; // renderer->HighestRootSignatureVersion();
    if (vrsd.Version == D3D_ROOT_SIGNATURE_VERSION_1_0)
    {
        vrsd.Desc_1_0       = desc;
        vrsd.Desc_1_0.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                          // @TODO(Roman): support stream output
                          /*| D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT*/;
    }
    else if (vrsd.Version == D3D_ROOT_SIGNATURE_VERSION_1_1)
    {
        // @TODO(Roman): Root Signature v1.1 support
        FailedM("Root Signature v1.1 is not supported yet");
    }
    else
    {
        // @NOTE(Roman): This is the first and the last time
        //               we're asserting Root Signature version.
        //               It's not needed to do this another places.
        FailedM("Unsuppored or unknown Root Signature version");
    }

    ID3DBlob *error_blob = 0;
    HRESULT error = D3D12SerializeVersionedRootSignature(&vrsd, &m_Signature, &error_blob);
    if (FAILED(error))
    {
        FailedM("Versioned Root Signature creation failure: %s", error_blob->GetBufferPointer());
    }
    SafeRelease(error_blob);

    error = renderer->Device()->CreateRootSignature(0,
                                                    m_Signature->GetBufferPointer(),
                                                    m_Signature->GetBufferSize(),
                                                    IID_PPV_ARGS(&m_RootSignature));
    Check(SUCCEEDED(error));
}

void GraphicsProgram::CreatePipelineState(
    bool                           blending_enabled,
    D3D12_CULL_MODE                cull_mode,
    bool                           depth_test_enabled,
    const D3D12_INPUT_LAYOUT_DESC& input_layout)
{
    // @TODO(Roman): support stream output
    D3D12_STREAM_OUTPUT_DESC sod;
    sod.pSODeclaration   = 0;
    sod.NumEntries       = 0;
    sod.pBufferStrides   = 0;
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
    depth_desc.StencilReadMask              = U8_MAX;
    depth_desc.StencilWriteMask             = U8_MAX;
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
    gpsd.pRootSignature        = m_RootSignature;
    gpsd.VS                    = m_VertexShader.Bytecode();
    gpsd.PS                    = m_PixelShader.Bytecode();
    gpsd.DS                    = m_DomainShader.Bytecode();
    gpsd.HS                    = m_HullShader.Bytecode();
    gpsd.GS                    = m_GeometryShader.Bytecode();
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

    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    HRESULT error = renderer->Device()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_PipelineState));
    Check(SUCCEEDED(error));
}

void GraphicsProgram::AttachHullShader(const StaticString<MAX_PATH>& filename)
{
    CheckM(!m_HullShader.Blob(), "Hull Shader is already attached");

    Memory *memory = Memory::Get();

    AsyncFile file(filename, AsyncFile::FLAGS::READ);

    u32 shader_size = file.Size();

    char *shader = memory->PushToTA<char>(shader_size + 1);

    file.Read(shader, shader_size);
    file.Wait();

    m_HullShader.Compile(shader, shader_size, filename.Data(), "HSMain", "hs_5_1");
}

void GraphicsProgram::AttachDomainShader(const StaticString<MAX_PATH>& filename)
{
    CheckM(!m_DomainShader.Blob(), "Domain Shader is already attached");

    Memory *memory = Memory::Get();

    AsyncFile file(filename, AsyncFile::FLAGS::READ);

    u32 shader_size = file.Size();

    char *shader = memory->PushToTA<char>(shader_size + 1);

    file.Read(shader, shader_size);
    file.Wait();

    m_DomainShader.Compile(shader, shader_size, filename.Data(), "DSMain", "ds_5_1");
}

void GraphicsProgram::AttachGeometryShader(const StaticString<MAX_PATH>& filename)
{
    CheckM(!m_GeometryShader.Blob(), "Geometry Shader is already attached");

    Memory *memory = Memory::Get();

    AsyncFile file(filename, AsyncFile::FLAGS::READ);

    u32 shader_size = file.Size();

    char *shader = memory->PushToTA<char>(shader_size + 1);

    file.Read(shader, shader_size);
    file.Wait();

    m_GeometryShader.Compile(shader, shader_size, filename.Data(), "GSMain", "gs_5_1");
}

void GraphicsProgram::AttachResource(IGPUResource *resource)
{
    GPUResource *d3d12_resource = cast<GPUResource *>(resource);
    CheckM(d3d12_resource->Kind() != GPUResource::KIND::VB && d3d12_resource->Kind() != GPUResource::KIND::IB, "You can't attach vertex nor index buffer.");

    m_BoundResources.PushBack(d3d12_resource);
}

void GraphicsProgram::BindVertexBuffer(IGPUResource *resource)
{
    GPUResource *vb = cast<GPUResource *>(resource);
    CheckM(vb->Kind() == GPUResource::KIND::VB, "Resource \"%s\" is not a vertex buffer", vb->Name());

    m_VertexBuffer = vb;

    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    ID3D12GraphicsCommandList *graphics_list = renderer->CurrentGraphicsList();

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = m_VertexBuffer->Handle()->GetGPUVirtualAddress();
    vbv.SizeInBytes    = m_VertexBuffer->VertexCount() * m_VertexBuffer->VertexStride();
    vbv.StrideInBytes  = m_VertexBuffer->VertexStride();

    graphics_list->IASetVertexBuffers(0, 1, &vbv);
}

void GraphicsProgram::BindIndexBuffer(IGPUResource *resource)
{
    GPUResource *ib = cast<GPUResource *>(resource);
    CheckM(ib->Kind() == GPUResource::KIND::IB, "Resource \"%s\" is not an index buffer", ib->Name());

    m_IndexBuffer = ib;

    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    ID3D12GraphicsCommandList *graphics_list = renderer->CurrentGraphicsList();

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = m_IndexBuffer->Handle()->GetGPUVirtualAddress();
    ibv.SizeInBytes    = m_IndexBuffer->IndexCount() * m_IndexBuffer->IndexStride();
    ibv.Format         = DXGI_FORMAT_R32_UINT;

    graphics_list->IASetIndexBuffer(&ibv);
}

void GraphicsProgram::DrawVertices()
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    ID3D12GraphicsCommandList *graphics_list = renderer->CurrentGraphicsList();
    graphics_list->DrawInstanced(m_VertexBuffer->VertexCount(), 1, 0, 0);
}

void GraphicsProgram::DrawIndices()
{
    Renderer *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());

    ID3D12GraphicsCommandList *graphics_list = renderer->CurrentGraphicsList();
    graphics_list->DrawIndexedInstanced(m_IndexBuffer->IndexCount(), 1, 0, 0, 0);
}

GraphicsProgram& GraphicsProgram::operator=(GraphicsProgram&& other) noexcept
{
    if (this != &other)
    {
        // moving vtable
        *cast<u64 *>(this)   = *cast<u64 *>(&other);
        *cast<u64 *>(&other) = 0;

        m_Signature      = other.m_Signature;
        m_RootSignature  = other.m_RootSignature;
        m_PipelineState  = other.m_PipelineState;
        m_VertexBuffer   = other.m_VertexBuffer;
        m_IndexBuffer    = other.m_IndexBuffer;
        m_BoundResources = RTTI::move(other.m_BoundResources);
        m_VertexShader   = RTTI::move(other.m_VertexShader);
        m_PixelShader    = RTTI::move(other.m_PixelShader);
        m_HullShader     = RTTI::move(other.m_HullShader);
        m_DomainShader   = RTTI::move(other.m_DomainShader);
        m_GeometryShader = RTTI::move(other.m_GeometryShader);

        other.m_Signature     = null;
        other.m_RootSignature = null;
        other.m_PipelineState = null;
        other.m_VertexBuffer  = null;
        other.m_IndexBuffer   = null;
    }
    return *this;
}

//
// ComputeProgram
//

ComputeProgram::ComputeProgram()
{
}

ComputeProgram::~ComputeProgram()
{
}

//
// GPUProgramManager
//

GPUProgramManager::GPUProgramManager(Allocator *allocator)
    : m_Allocator(allocator),
      m_GraphicsPrograms(m_Allocator),
      m_ComputePrograms(m_Allocator)
{
}

GPUProgramManager::~GPUProgramManager()
{
}

void GPUProgramManager::Destroy()
{
    this->~GPUProgramManager();
}

IGraphicsProgram *GPUProgramManager::CreateGraphicsProgram(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename)
{
    m_GraphicsPrograms.EmplaceBack(m_Allocator, vs_filename, ps_filename);
    return &m_GraphicsPrograms.Last()->Data();
}

void GPUProgramManager::SetCurrentGraphicsProgram(IGraphicsProgram *graphics_program)
{
    Renderer        *renderer = cast<Renderer *>(GraphicsAPI::GetRenderer());
    GraphicsProgram *program  = cast<GraphicsProgram *>(graphics_program);
    Memory          *memory   = Memory::Get();

    ID3D12GraphicsCommandList *graphics_list = renderer->CurrentGraphicsList();

    graphics_list->SetGraphicsRootSignature(program->RootSignature());

    graphics_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphics_list->SetPipelineState(program->PipelineState());

    u32 bound_resources_count = cast<u32>(program->BoundResources().Count());
    if (bound_resources_count)
    {
        ID3D12DescriptorHeap **desc_heaps = memory->PushToTA<ID3D12DescriptorHeap *>(bound_resources_count);

        u64 desc_heap_index = 0;
        for (GPUResource *it : program->BoundResources())
        {
            desc_heaps[desc_heap_index++] = it->DescHeap()->Handle();
        }

        graphics_list->SetDescriptorHeaps(bound_resources_count, desc_heaps);

        u32 resource_index = 0;
        for (GPUResource *it : program->BoundResources())
        {
            switch (it->Kind())
            {
                case GPUResource::KIND::CB:
                {
                    graphics_list->SetGraphicsRootConstantBufferView(resource_index, it->Handle()->GetGPUVirtualAddress());
                } break;

                case GPUResource::KIND::SR:
                {
                    graphics_list->SetGraphicsRootShaderResourceView(resource_index, it->Handle()->GetGPUVirtualAddress());
                } break;

                case GPUResource::KIND::UA:
                {
                    graphics_list->SetGraphicsRootUnorderedAccessView(resource_index, it->Handle()->GetGPUVirtualAddress());
                } break;

                default:
                {
                    // @TODO(Roman): Add samplers, constants and tables
                    FailedM("Samplers, constants and tables are not supported yet");
                } break;
            }

            ++resource_index;
        }
    }
}

}
