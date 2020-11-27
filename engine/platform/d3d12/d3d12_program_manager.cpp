//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "platform/d3d12/d3d12_program_manager.h"
#include "graphics/graphics_api.h"
#include "tools/async_file.h"
#include "platform/d3d12/d3d12_common.h"

#include <d3dcompiler.h>

namespace D3D12
{

//
// GraphicsProgram
//

GraphicsProgram& GraphicsProgram::operator=(GraphicsProgram&& other) noexcept
{
    if (this != &other)
    {
        signature       = other.signature;
        root_signature  = other.root_signature;
        pipeline_state  = other.pipeline_state;
        vertex_buffer   = other.vertex_buffer;
        index_buffer    = other.index_buffer;
        bound_resources = RTTI::move(other.bound_resources);
        vertex_shader   = other.vertex_shader;
        pixel_shader    = other.pixel_shader;
        hull_shader     = other.hull_shader;
        domain_shader   = other.domain_shader;
        geometry_shader = other.geometry_shader;
    }
    return *this;
}

//
// ComputeProgram
//

ComputeProgram& ComputeProgram::operator=(ComputeProgram&& other) noexcept
{
    if (this != &other)
    {
        signature       = other.signature;
        root_signature  = other.root_signature;
        pipeline_state  = other.pipeline_state;
        shader          = other.shader;
        bound_resources = RTTI::move(other.bound_resources);
    }
    return *this;
}

//
// ProgramManager
//

ProgramManager::ProgramManager(Allocator *allocator)
    : m_Allocator(allocator),
      m_GraphicsPrograms(allocator),
      m_ComputePrograms(allocator)
{
}

ProgramManager::~ProgramManager()
{
    if (m_Allocator)
    {
        for (GraphicsProgram& graphics_program : m_GraphicsPrograms)
        {
            SafeRelease(graphics_program.signature);
            SafeRelease(graphics_program.root_signature);
            SafeRelease(graphics_program.pipeline_state);
            SafeRelease(graphics_program.vertex_shader);
            SafeRelease(graphics_program.pixel_shader);
            SafeRelease(graphics_program.hull_shader);
            SafeRelease(graphics_program.domain_shader);
            SafeRelease(graphics_program.geometry_shader);
        }

        for (ComputeProgram& compute_program : m_ComputePrograms)
        {
            SafeRelease(compute_program.signature);
            SafeRelease(compute_program.root_signature);
            SafeRelease(compute_program.pipeline_state);
            SafeRelease(compute_program.shader);
        }
    }
}

u64 ProgramManager::CreateGraphicsProgram(const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename)
{
    GraphicsProgram graphics_program(m_Allocator);
    graphics_program.signature       = null;
    graphics_program.root_signature  = null;
    graphics_program.pipeline_state  = null;
    graphics_program.vertex_shader   = null;
    graphics_program.pixel_shader    = null;
    graphics_program.hull_shader     = null;
    graphics_program.domain_shader   = null;
    graphics_program.geometry_shader = null;

    AttachMainShaders(graphics_program, vs_filename, ps_filename);

    // @TODO(Roman): Parse shaders?
    // @CleanUp(Roman): Hardcoded.

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

    CreateRootSignature(graphics_program, root_signature_desc);
    
    D3D12_INPUT_ELEMENT_DESC elements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//      { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//      { "NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_INPUT_LAYOUT_DESC input_layout;
    input_layout.pInputElementDescs = elements;
    input_layout.NumElements        = cast<u32>(ArrayCount(elements));

    CreatePipelineState(graphics_program, input_layout, false, D3D12_CULL_MODE_NONE, true);

    m_GraphicsPrograms.PushBack(RTTI::forward<GraphicsProgram>(graphics_program));
    return m_GraphicsPrograms.Count() - 1;
}

void ProgramManager::SetCurrentGraphicsProgram(const GraphicsProgram& graphics_program)
{
    Renderer        *renderer       = cast<Renderer *>(GraphicsAPI::GetRenderer());
    MemoryManager   *memory_manager = cast<MemoryManager *>(GraphicsAPI::GetMemoryManager());

    ID3D12GraphicsCommandList *graphics_list = renderer->CurrentGraphicsList();

    graphics_list->SetGraphicsRootSignature(graphics_program.root_signature);

    graphics_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphics_list->SetPipelineState(graphics_program.pipeline_state);

    u32 bound_resources_count = cast<u32>(graphics_program.bound_resources.Count());
    if (bound_resources_count)
    {
        ID3D12DescriptorHeap **desc_heaps = Memory::Get()->PushToTA<ID3D12DescriptorHeap *>(bound_resources_count);

        u64 desc_heap_index = 0;
        for (const GPU::ResourceHandle& resource : graphics_program.bound_resources)
        {
            switch (resource.kind)
            {
                case GPU::RESOURCE_KIND::CB:
                {
                    Buffer&   buffer    = memory_manager->GetBuffer(resource.index);
                    DescHeap& desc_heap = memory_manager->GetDescHeap(buffer.desc_heap_index);

                    desc_heaps[desc_heap_index++] = desc_heap.handle;
                } break;

                case GPU::RESOURCE_KIND::SR:
                {
                    Texture&  texture   = memory_manager->GetTexture(resource.index);
                    DescHeap& desc_heap = memory_manager->GetDescHeap(texture.desc_heap_index);

                    desc_heaps[desc_heap_index++] = desc_heap.handle;
                } break;

                case GPU::RESOURCE_KIND::UA:
                {
                    FailedM("Write-back resources are not supported yet");
                } break;

                default:
                {
                    FailedM("Illegal resoucre type: 0x%I64X", resource.kind);
                } break;
            }
        }

        graphics_list->SetDescriptorHeaps(bound_resources_count, desc_heaps);

        u32 resource_index = 0;
        for (const GPU::ResourceHandle& resource : graphics_program.bound_resources)
        {
            switch (resource.kind)
            {
                case GPU::RESOURCE_KIND::CB:
                {
                    graphics_list->SetGraphicsRootConstantBufferView(resource_index, memory_manager->GetBufferGPUVirtualAddress(resource.index));
                } break;

                case GPU::RESOURCE_KIND::SR:
                {
                    graphics_list->SetGraphicsRootShaderResourceView(resource_index, memory_manager->GetTextureGPUVirtualAddress(resource.index));
                } break;

                case GPU::RESOURCE_KIND::UA:
                {
                    FailedM("Write-back resources are not supported yet");
                    // graphics_list->SetGraphicsRootUnorderedAccessView(resource_index, memory_manager->GetUAGPUVirtualAddress());
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

void ProgramManager::AttachHullShader(GraphicsProgram& graphics_program, const StaticString<MAX_PATH>& filename)
{
    AsyncFile file(filename, AsyncFile::FLAGS::READ);

    u32   hlsl_code_length = file.Size();
    char *hlsl_code        = Memory::Get()->PushToTA<char>(hlsl_code_length);

    file.Read(hlsl_code, hlsl_code_length);
    file.Wait();

    graphics_program.hull_shader = CompileShader(hlsl_code, hlsl_code_length, filename.Data(), "HSMain", "hs_5_1");
}

void ProgramManager::AttachDomainShader(GraphicsProgram& graphics_program, const StaticString<MAX_PATH>& filename)
{
    AsyncFile file(filename, AsyncFile::FLAGS::READ);

    u32   hlsl_code_length = file.Size();
    char *hlsl_code        = Memory::Get()->PushToTA<char>(hlsl_code_length);

    file.Read(hlsl_code, hlsl_code_length);
    file.Wait();

    graphics_program.domain_shader = CompileShader(hlsl_code, hlsl_code_length, filename.Data(), "DSMain", "ds_5_1");
}

void ProgramManager::AttachGeometryShader(GraphicsProgram& graphics_program, const StaticString<MAX_PATH>& filename)
{
    AsyncFile file(filename, AsyncFile::FLAGS::READ);

    u32   hlsl_code_length = file.Size();
    char *hlsl_code        = Memory::Get()->PushToTA<char>(hlsl_code_length);

    file.Read(hlsl_code, hlsl_code_length);
    file.Wait();

    graphics_program.geometry_shader = CompileShader(hlsl_code, hlsl_code_length, filename.Data(), "GSMain", "gs_5_1");
}

void ProgramManager::AttachResource(GraphicsProgram& graphics_program, GPU::ResourceHandle resource_handle)
{
    CheckM(resource_handle.kind != GPU::RESOURCE_KIND::VB && resource_handle.kind != GPU::RESOURCE_KIND::IB, "You can't attach vertex nor index buffer.");
    graphics_program.bound_resources.PushBack(resource_handle);
}

void ProgramManager::BindVertexBuffer(GraphicsProgram& graphics_program, GPU::ResourceHandle resource_handle)
{
    CheckM(resource_handle.kind == GPU::RESOURCE_KIND::VB, "Resource #%I64u is not a vertex buffer", resource_handle.index);

    MemoryManager *memory_manager = cast<MemoryManager *>(GraphicsAPI::GetMemoryManager());
    Buffer&        buffer         = memory_manager->GetBuffer(resource_handle.index);

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = memory_manager->GetBufferGPUVirtualAddress(resource_handle.index);
    vbv.SizeInBytes    = cast<u32>(buffer.actual_size);
    vbv.StrideInBytes  = buffer.vstride;

    cast<Renderer *>(GraphicsAPI::GetRenderer())->CurrentGraphicsList()->IASetVertexBuffers(0, 1, &vbv);

    graphics_program.vertex_buffer = resource_handle;
}

void ProgramManager::BindIndexBuffer(GraphicsProgram& graphics_program, GPU::ResourceHandle resource_handle)
{
    CheckM(resource_handle.kind == GPU::RESOURCE_KIND::IB, "Resource #%I64u is not an index buffer", resource_handle.index);

    MemoryManager *memory_manager = cast<MemoryManager *>(GraphicsAPI::GetMemoryManager());
    Buffer&        buffer         = memory_manager->GetBuffer(resource_handle.index);

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = memory_manager->GetBufferGPUVirtualAddress(resource_handle.index);
    ibv.SizeInBytes    = cast<u32>(buffer.actual_size);
    ibv.Format         = DXGI_FORMAT_R32_UINT;

    cast<Renderer *>(GraphicsAPI::GetRenderer())->CurrentGraphicsList()->IASetIndexBuffer(&ibv);

    graphics_program.index_buffer = resource_handle;
}

void ProgramManager::DrawVertices(const GraphicsProgram& graphics_program)
{
    Buffer& buffer = cast<MemoryManager *>(GraphicsAPI::GetMemoryManager())->GetBuffer(graphics_program.vertex_buffer.index);
    cast<Renderer *>(GraphicsAPI::GetRenderer())->CurrentGraphicsList()->DrawInstanced(buffer.vcount, 1, 0, 0);
}

void ProgramManager::DrawIndices(const GraphicsProgram& graphics_program)
{
    Buffer& buffer = cast<MemoryManager *>(GraphicsAPI::GetMemoryManager())->GetBuffer(graphics_program.index_buffer.index);
    cast<Renderer *>(GraphicsAPI::GetRenderer())->CurrentGraphicsList()->DrawIndexedInstanced(buffer.icount, 1, 0, 0, 0);
}

ProgramManager& ProgramManager::operator=(ProgramManager&& other) noexcept
{
    if (this != &other)
    {
        m_Allocator        = other.m_Allocator;
        m_GraphicsPrograms = RTTI::move(other.m_GraphicsPrograms);
        m_ComputePrograms  = RTTI::move(other.m_ComputePrograms);

        other.m_Allocator = null;
    }
    return *this;
}

void ProgramManager::AttachMainShaders(GraphicsProgram& graphics_program, const StaticString<MAX_PATH>& vs_filename, const StaticString<MAX_PATH>& ps_filename)
{
    AsyncFile vs_file(vs_filename, AsyncFile::FLAGS::READ);
    AsyncFile ps_file(ps_filename, AsyncFile::FLAGS::READ);

    u32 vs_length = vs_file.Size();
    u32 ps_length = ps_file.Size();

    char *vs = Memory::Get()->PushToTA<char>(vs_length + ps_length);
    char *ps = vs + vs_length;

    vs_file.Read(vs, vs_length);
    ps_file.Read(ps, ps_length);

    AsyncFile::WaitForAll(vs_file, ps_file);

    graphics_program.vertex_shader = CompileShader(vs, vs_length, vs_filename.Data(), "VSMain", "vs_5_1");
    graphics_program.pixel_shader  = CompileShader(ps, ps_length, ps_filename.Data(), "PSMain", "ps_5_1");
}

void ProgramManager::CreateRootSignature(GraphicsProgram& graphics_program, const D3D12_ROOT_SIGNATURE_DESC& root_signature_desc)
{
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC vrsd = {};
    vrsd.Version = D3D_ROOT_SIGNATURE_VERSION_1_0; // renderer->HighestRootSignatureVersion();
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
        FailedM("Root Signature v1.1 is not supported yet");
    }
    else
    {
        // @NOTE(Roman): This is the first and the last time
        //               we're asserting Root Signature version.
        //               It's not needed to do this another places.
        FailedM("Unsuppored or unknown Root Signature version");
    }

    ID3DBlob *error_blob = null;
    HRESULT   error      = D3D12SerializeVersionedRootSignature(&vrsd, &graphics_program.signature, &error_blob);
    if (Failed(error))
    {
        FailedM("Versioned Root Signature creation failure: %s", error_blob->GetBufferPointer());
    }
    SafeRelease(error_blob);

    error = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device()->CreateRootSignature(0,
                                                                                        graphics_program.signature->GetBufferPointer(),
                                                                                        graphics_program.signature->GetBufferSize(),
                                                                                        IID_PPV_ARGS(&graphics_program.root_signature));
    Check(CheckResultAndPrintMessages(error));
}

void ProgramManager::CreatePipelineState(GraphicsProgram& graphics_program, const D3D12_INPUT_LAYOUT_DESC& input_layout, bool blending_enabled, D3D12_CULL_MODE cull_mode, bool depth_test_enabled)
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
    gpsd.pRootSignature        = graphics_program.root_signature;
    gpsd.VS.pShaderBytecode    = graphics_program.vertex_shader->GetBufferPointer();
    gpsd.VS.BytecodeLength     = graphics_program.vertex_shader->GetBufferSize();
    gpsd.PS.pShaderBytecode    = graphics_program.pixel_shader->GetBufferPointer();
    gpsd.PS.BytecodeLength     = graphics_program.pixel_shader->GetBufferSize();
    if (graphics_program.domain_shader)
    {
        gpsd.DS.pShaderBytecode = graphics_program.domain_shader->GetBufferPointer();
        gpsd.DS.BytecodeLength  = graphics_program.domain_shader->GetBufferSize();
    }
    if (graphics_program.hull_shader)
    {
        gpsd.HS.pShaderBytecode = graphics_program.hull_shader->GetBufferPointer();
        gpsd.HS.BytecodeLength  = graphics_program.hull_shader->GetBufferSize();
    }
    if (graphics_program.geometry_shader)
    {
        gpsd.GS.pShaderBytecode = graphics_program.geometry_shader->GetBufferPointer();
        gpsd.GS.BytecodeLength  = graphics_program.geometry_shader->GetBufferSize();
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

    HRESULT error = cast<Renderer *>(GraphicsAPI::GetRenderer())->Device()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&graphics_program.pipeline_state));
    Check(CheckResultAndPrintMessages(error));
}

ID3DBlob *ProgramManager::CompileShader(const char *hlsl_code, u32 hlsl_code_length, const char *name, const char *entry_point, const char *target)
{
    u32 compile_flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#if DEBUG
    compile_flags |= D3DCOMPILE_DEBUG
                  |  D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3
                  |  D3DCOMPILE_SKIP_VALIDATION;
#endif
    if (cast<Renderer *>(GraphicsAPI::GetRenderer())->HalfPrecisionSupported())
    {
        compile_flags |= D3DCOMPILE_PARTIAL_PRECISION;
    }

    ID3DBlob *code   = null;
    ID3DBlob *errors = null;
    HRESULT   error  = D3DCompile(hlsl_code, hlsl_code_length, name, null, null, entry_point, target, compile_flags, 0, &code, &errors);

    if (Failed(error))
    {
        FailedM("Shader compilation failure:\n\n%s", errors->GetBufferPointer());
    }

    SafeRelease(errors);

    return code;
}

}
