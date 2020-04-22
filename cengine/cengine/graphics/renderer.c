//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/renderer.h"
#include "cengine.h"

#define SafeRelease(directx_interface)                           \
{                                                                \
    if (directx_interface)                                       \
    {                                                            \
        (directx_interface)->lpVtbl->Release(directx_interface); \
        (directx_interface) = 0;                                 \
    }                                                            \
}

void SetVSync(Engine *engine, b32 enable)
{
    if (engine->renderer.vsync != enable)
        engine->renderer.vsync = enable;
}

VertexBuffer CreateVertexBuffer(Engine *engine, void *vertices, u32 count, u32 stride)
{
    VertexBuffer buffer;
    buffer.res    = 0;
    buffer.count  = count;
    buffer.stride = stride;

    D3D12_HEAP_PROPERTIES hp;
    hp.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    hp.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    hp.CreationNodeMask     = 0;
    hp.VisibleNodeMask      = 0;

    D3D12_RESOURCE_DESC rd;
    rd.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Alignment          = 0;
    rd.Width              = buffer.count * buffer.stride;
    rd.Height             = 1;
    rd.DepthOrArraySize   = 1;
    rd.MipLevels          = 1;
    rd.Format             = DXGI_FORMAT_UNKNOWN;
    rd.SampleDesc.Count   = 1;
    rd.SampleDesc.Quality = 0;
    rd.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rd.Flags              = D3D12_RESOURCE_FLAG_NONE;

    // @Optimize(Roman): Make em placed?
    engine->renderer.error = engine->renderer.device->lpVtbl->CreateCommittedResource(engine->renderer.device,
                                                                                      &hp,
                                                                                      D3D12_HEAP_FLAG_NONE,
                                                                                      &rd,
                                                                                      D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
                                                                                    | D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                                      0,
                                                                                      &IID_ID3D12Resource,
                                                                                      &buffer.res);
    Check(SUCCEEDED(engine->renderer.error));

    D3D12_RANGE read_range;
    read_range.Begin = 0;
    read_range.End   = 0;

    byte *vertex_data = 0;
    engine->renderer.error = buffer.res->lpVtbl->Map(buffer.res, 0, &read_range, &vertex_data);
    Check(SUCCEEDED(engine->renderer.error));

    CopyMemory(vertex_data, vertices, buffer.count * buffer.stride);

    buffer.res->lpVtbl->Unmap(buffer.res, 0, 0);

    return buffer;
}

void DestroyVertexBuffer(VertexBuffer *buffer)
{
    SafeRelease(buffer->res);
    buffer->count  = 0;
    buffer->stride = 0;
}

IndexBuffer CreateIndexBuffer(Engine *engine, u32 *indecies, u32 count)
{
    IndexBuffer buffer;
    buffer.res   = 0;
    buffer.count = count;

    D3D12_HEAP_PROPERTIES hp;
    hp.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    hp.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    hp.CreationNodeMask     = 0;
    hp.VisibleNodeMask      = 0;

    D3D12_RESOURCE_DESC rd;
    rd.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Alignment          = 0;
    rd.Width              = buffer.count * sizeof(u32);
    rd.Height             = 1;
    rd.DepthOrArraySize   = 1;
    rd.MipLevels          = 1;
    rd.Format             = DXGI_FORMAT_UNKNOWN;
    rd.SampleDesc.Count   = 1;
    rd.SampleDesc.Quality = 0;
    rd.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rd.Flags              = D3D12_RESOURCE_FLAG_NONE;

    // @Optimize(Roman): Make em placed?
    engine->renderer.error = engine->renderer.device->lpVtbl->CreateCommittedResource(engine->renderer.device,
                                                                                      &hp,
                                                                                      D3D12_HEAP_FLAG_NONE,
                                                                                      &rd,
                                                                                      D3D12_RESOURCE_STATE_INDEX_BUFFER
                                                                                    | D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                                      0,
                                                                                      &IID_ID3D12Resource,
                                                                                      &buffer.res);
    Check(SUCCEEDED(engine->renderer.error));

    D3D12_RANGE read_range;
    read_range.Begin = 0;
    read_range.End   = 0;

    byte *index_data = 0;
    engine->renderer.error = buffer.res->lpVtbl->Map(buffer.res, 0, &read_range, &index_data);
    Check(SUCCEEDED(engine->renderer.error));

    CopyMemory(index_data, indecies, buffer.count * sizeof(u32));

    buffer.res->lpVtbl->Unmap(buffer.res, 0, 0);

    return buffer;
}

void DestroyIndexBuffer(IndexBuffer *buffer)
{
    SafeRelease(buffer->res);
    buffer->count = 0;   
}

internal byte *ReadEntireShaderFile(Engine *engine, const char *filename, u32 *filesize)
{
    if (filename)
    {
        HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
        CheckM(file != INVALID_HANDLE_VALUE, "File does not exist");

        *filesize     = GetFileSize(file, 0);
        byte *buffer = PushToTransientArea(engine->memory, *filesize);

        DebugResult(ReadFile(file, buffer, *filesize, 0, 0));

        DebugResult(CloseHandle(file));
        return buffer;
    }
    return 0;
}

Shader CreateShader(Engine *engine, const char *filename, const char *entry_point, SHADER_KIND kind)
{
    Shader shader = {0};

    if (filename && entry_point)
    {
        u32   source_size = 0;
        byte *source      = ReadEntireShaderFile(engine, filename, &source_size);

        u32 compile_flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
    #if DEBUG
        compile_flags |= D3DCOMPILE_DEBUG
                      |  D3DCOMPILE_SKIP_OPTIMIZATION;
    #else
        compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3
                      |  D3DCOMPILE_SKIP_VALIDATION
                      |  D3DCOMPILE_PARTIAL_PRECISION;
    #endif

        ID3DBlob *errors = 0;

        switch (kind)
        {
            case SHADER_KIND_VERTEX:
            {
                engine->renderer.error = D3DCompile(source,
                                                    source_size,
                                                    0,
                                                    0, // @TODO(Roman): support defines
                                                    0, // @TODO(Roman): support includes?
                                                    entry_point,
                                                    "vs_5_0",
                                                    compile_flags,
                                                    0,
                                                    &shader.blob,
                                                    &errors);
                if (FAILED(engine->renderer.error))
                {
                    MessageBoxA(0, errors->lpVtbl->GetBufferPointer(errors), "Vertex shader compilation failure", MB_OK | MB_ICONERROR);
                    ExitProcess(1);
                }
                shader.bytecode.pShaderBytecode = shader.blob->lpVtbl->GetBufferPointer(shader.blob);
                shader.bytecode.BytecodeLength  = shader.blob->lpVtbl->GetBufferSize(shader.blob);
            } break;

            case SHADER_KIND_HULL:
            {
                engine->renderer.error = D3DCompile(source,
                                                    source_size,
                                                    0,
                                                    0, // @TODO(Roman): support defines
                                                    0, // @TODO(Roman): support includes?
                                                    entry_point,
                                                    "hs_5_0",
                                                    compile_flags,
                                                    0,
                                                    &shader.blob,
                                                    &errors);
                if (FAILED(engine->renderer.error))
                {
                    MessageBoxA(0, errors->lpVtbl->GetBufferPointer(errors), "Hull shader compilation failure", MB_OK | MB_ICONERROR);
                    ExitProcess(1);
                }
                shader.bytecode.pShaderBytecode = shader.blob->lpVtbl->GetBufferPointer(shader.blob);
                shader.bytecode.BytecodeLength  = shader.blob->lpVtbl->GetBufferSize(shader.blob);
            } break;

            case SHADER_KIND_DOMAIN:
            {
                engine->renderer.error = D3DCompile(source,
                                                    source_size,
                                                    0,
                                                    0, // @TODO(Roman): support defines
                                                    0, // @TODO(Roman): support includes?
                                                    entry_point,
                                                    "ds_5_0",
                                                    compile_flags,
                                                    0,
                                                    &shader.blob,
                                                    &errors);
                if (FAILED(engine->renderer.error))
                {
                    MessageBoxA(0, errors->lpVtbl->GetBufferPointer(errors), "Domain shader compilation failure", MB_OK | MB_ICONERROR);
                    ExitProcess(1);
                }
                shader.bytecode.pShaderBytecode = shader.blob->lpVtbl->GetBufferPointer(shader.blob);
                shader.bytecode.BytecodeLength  = shader.blob->lpVtbl->GetBufferSize(shader.blob);
            } break;

            case SHADER_KIND_GEOMETRY:
            {
                engine->renderer.error = D3DCompile(source,
                                                    source_size,
                                                    0,
                                                    0, // @TODO(Roman): support defines
                                                    0, // @TODO(Roman): support includes?
                                                    entry_point,
                                                    "gs_5_0",
                                                    compile_flags,
                                                    0,
                                                    &shader.blob,
                                                    &errors);
                if (FAILED(engine->renderer.error))
                {
                    MessageBoxA(0, errors->lpVtbl->GetBufferPointer(errors), "Geometry shader compilation failure", MB_OK | MB_ICONERROR);
                    ExitProcess(1);
                }
                shader.bytecode.pShaderBytecode = shader.blob->lpVtbl->GetBufferPointer(shader.blob);
                shader.bytecode.BytecodeLength  = shader.blob->lpVtbl->GetBufferSize(shader.blob);
            } break;

            case SHADER_KIND_PIXEL:
            {
                engine->renderer.error = D3DCompile(source,
                                                    source_size,
                                                    0,
                                                    0, // @TODO(Roman): support defines
                                                    0, // @TODO(Roman): support includes?
                                                    entry_point,
                                                    "ps_5_0",
                                                    compile_flags,
                                                    0,
                                                    &shader.blob,
                                                    &errors);
                if (FAILED(engine->renderer.error))
                {
                    MessageBoxA(0, errors->lpVtbl->GetBufferPointer(errors), "Pixel shader compilation failure", MB_OK | MB_ICONERROR);
                    ExitProcess(1);
                }
                shader.bytecode.pShaderBytecode = shader.blob->lpVtbl->GetBufferPointer(shader.blob);
                shader.bytecode.BytecodeLength  = shader.blob->lpVtbl->GetBufferSize(shader.blob);
            } break;
        }

        SafeRelease(errors);
    }

    return shader;
}

void DestroyShader(Shader *shader)
{
    if (shader)
    {
        shader->bytecode.pShaderBytecode = 0;
        shader->bytecode.BytecodeLength  = 0;
        SafeRelease(shader->blob);
    }
}

void CreatePipelineStage(
    IN       Engine        *engine,
    IN       Shader        *vertex_shader,
    OPTIONAL Shader        *hull_shader,
    OPTIONAL Shader        *domain_shader,
    OPTIONAL Shader        *geometry_shader,
    IN       Shader        *pixel_shader,
    IN       b32            enable_blending,
    IN       u32            shader_args_count,
    IN       ShaderArg     *shader_args,
    IN       VertexBuffer  *vertex_buffer,
    OPTIONAL IndexBuffer   *index_buffer,
    OUT      PipelineStage *pipeline_stage)
{
    Check(engine);
    Check(vertex_shader);
    Check(pixel_shader);
    Check(shader_args && shader_args_count);
    Check(vertex_buffer);
    Check(pipeline_stage);

    pipeline_stage->vertex_buffer = vertex_buffer;
    pipeline_stage->index_buffer  = index_buffer;

    // @TODO(Roman): support stream output
    D3D12_STREAM_OUTPUT_DESC sod;
    sod.pSODeclaration   = 0;
    sod.NumEntries       = 0;
    sod.pBufferStrides   = 0;
    sod.NumStrides       = 0;
    sod.RasterizedStream = 0;

    D3D12_RENDER_TARGET_BLEND_DESC rtbd;
    rtbd.BlendEnable           = false;
    rtbd.LogicOpEnable         = false;
    rtbd.SrcBlend              = D3D12_BLEND_ONE;
    rtbd.DestBlend             = D3D12_BLEND_ZERO;
    rtbd.BlendOp               = D3D12_BLEND_OP_ADD;
    rtbd.SrcBlendAlpha         = D3D12_BLEND_ONE;
    rtbd.DestBlendAlpha        = D3D12_BLEND_ZERO;
    rtbd.BlendOpAlpha          = D3D12_BLEND_OP_ADD;
    rtbd.LogicOp               = D3D12_LOGIC_OP_NOOP;
    rtbd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_BLEND_DESC blend_desc;
    blend_desc.AlphaToCoverageEnable  = false;
    blend_desc.IndependentBlendEnable = false;
    for (u32 i = 0; i < ArrayCount(blend_desc.RenderTarget); ++i)
    {
        blend_desc.RenderTarget[i] = rtbd;
    }

    D3D12_RASTERIZER_DESC rasterizer_desc;
    rasterizer_desc.FillMode              = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode              = D3D12_CULL_MODE_BACK;
    rasterizer_desc.FrontCounterClockwise = false;
    rasterizer_desc.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_desc.DepthClipEnable       = true;
    rasterizer_desc.MultisampleEnable     = false; // true?
    rasterizer_desc.AntialiasedLineEnable = false; // true?
    rasterizer_desc.ForcedSampleCount     = 0;
    rasterizer_desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_DEPTH_STENCIL_DESC depth_desc;
    depth_desc.DepthEnable                  = true;
    depth_desc.DepthWriteMask               = enable_blending ? D3D12_DEPTH_WRITE_MASK_ZERO : D3D12_DEPTH_WRITE_MASK_ALL;
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

    D3D12_INPUT_ELEMENT_DESC *ieds = PushToTA(D3D12_INPUT_ELEMENT_DESC, engine->memory, shader_args_count);
    for (u32 i = 0; i < shader_args_count; ++i)
    {
        ieds[i].SemanticName         = shader_args[i].semantic_name;
        ieds[i].SemanticIndex        = shader_args[i].semantic_index;
        ieds[i].Format               = shader_args[i].format;
        ieds[i].InputSlot            = shader_args[i].input_slot;
        ieds[i].AlignedByteOffset    = i > 0 ? D3D12_APPEND_ALIGNED_ELEMENT : 0;
        ieds[i].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        ieds[i].InstanceDataStepRate = 0;
    }

    D3D12_INPUT_LAYOUT_DESC input_layout;
    input_layout.NumElements        = shader_args_count;
    input_layout.pInputElementDescs = ieds;

    // @TODO(Roman): Cached pipeline state
    D3D12_CACHED_PIPELINE_STATE cached_pipeline_state;
    cached_pipeline_state.pCachedBlob           = 0;
    cached_pipeline_state.CachedBlobSizeInBytes = 0;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd;
    gpsd.pRootSignature        = engine->renderer.graphics_root_signature;
    gpsd.VS                    = vertex_shader->bytecode;
    gpsd.PS                    = pixel_shader->bytecode;
    gpsd.DS                    = domain_shader   ? domain_shader->bytecode   : (D3D12_SHADER_BYTECODE){0};
    gpsd.HS                    = hull_shader     ? hull_shader->bytecode     : (D3D12_SHADER_BYTECODE){0};
    gpsd.GS                    = geometry_shader ? geometry_shader->bytecode : (D3D12_SHADER_BYTECODE){0};
    gpsd.StreamOutput          = sod;
    gpsd.BlendState            = blend_desc;
    gpsd.SampleMask            = UINT_MAX;
    gpsd.RasterizerState       = rasterizer_desc;
    gpsd.DepthStencilState     = depth_desc;
    gpsd.InputLayout           = input_layout;
    gpsd.IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // @TODO(Roman): support triangle strips
    gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gpsd.NumRenderTargets      = 1;
    for (u32 i = 0; i < gpsd.NumRenderTargets; ++i)
    {
        gpsd.RTVFormats[i] = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    for (u32 i = gpsd.NumRenderTargets; i < ArrayCount(gpsd.RTVFormats); ++i)
    {
        gpsd.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    }
    gpsd.DSVFormat             = DXGI_FORMAT_D32_FLOAT;
    gpsd.SampleDesc.Count      = 1;
    gpsd.SampleDesc.Quality    = 0;
    gpsd.NodeMask              = 0;
    gpsd.CachedPSO             = cached_pipeline_state;
    gpsd.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

    engine->renderer.error = engine->renderer.device->lpVtbl->CreateGraphicsPipelineState(engine->renderer.device,
                                                                                          &gpsd,
                                                                                          &IID_ID3D12PipelineState,
                                                                                          &pipeline_stage->pipeline);
    Check(SUCCEEDED(engine->renderer.error));
}

void DestroyPipelineStage(PipelineStage *pipeline_stage)
{
    SafeRelease(pipeline_stage->pipeline);
}

void RenderPipelineStage(Engine *engine, PipelineStage *pipeline_stage)
{
    if (pipeline_stage->index_buffer)
    {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = pipeline_stage->index_buffer->res->lpVtbl->GetGPUVirtualAddress(pipeline_stage->index_buffer->res);
        ibv.SizeInBytes    = pipeline_stage->index_buffer->count * sizeof(u32);
        ibv.Format         = DXGI_FORMAT_R32_UINT;

        engine->renderer.graphics_lists[engine->renderer.current_buffer]->lpVtbl->IASetIndexBuffer(engine->renderer.graphics_lists[engine->renderer.current_buffer],
                                                                                                   &ibv);
    }

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = pipeline_stage->vertex_buffer->res->lpVtbl->GetGPUVirtualAddress(pipeline_stage->vertex_buffer->res);
    vbv.SizeInBytes    = pipeline_stage->vertex_buffer->count * pipeline_stage->vertex_buffer->stride;
    vbv.StrideInBytes  = pipeline_stage->vertex_buffer->stride;

    engine->renderer.graphics_lists[engine->renderer.current_buffer]->lpVtbl->IASetVertexBuffers(engine->renderer.graphics_lists[engine->renderer.current_buffer],
                                                                                                 0,
                                                                                                 1,
                                                                                                 &vbv);

    engine->renderer.graphics_lists[engine->renderer.current_buffer]->lpVtbl->IASetPrimitiveTopology(engine->renderer.graphics_lists[engine->renderer.current_buffer],
                                                                                                     D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    engine->renderer.graphics_lists[engine->renderer.current_buffer]->lpVtbl->SetPipelineState(engine->renderer.graphics_lists[engine->renderer.current_buffer],
                                                                                               pipeline_stage->pipeline);

    if (pipeline_stage->index_buffer)
    {
        engine->renderer.graphics_lists[engine->renderer.current_buffer]->lpVtbl->DrawIndexedInstanced(engine->renderer.graphics_lists[engine->renderer.current_buffer],
                                                                                                       pipeline_stage->index_buffer->count,
                                                                                                       1,
                                                                                                       0,
                                                                                                       0,
                                                                                                       0);
    }
    else
    {
        engine->renderer.graphics_lists[engine->renderer.current_buffer]->lpVtbl->DrawInstanced(engine->renderer.graphics_lists[engine->renderer.current_buffer],
                                                                                                pipeline_stage->vertex_buffer->count,
                                                                                                1,
                                                                                                0,
                                                                                                0);
    }
}
