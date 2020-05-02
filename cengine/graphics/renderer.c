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

//
// Renderer
//

void SetVSync(Engine *engine, b32 enable)
{
    if (engine->renderer.vsync != enable)
        engine->renderer.vsync = enable;
}

//
// VertexBuffer
//

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

void SetVertexBuffer(Engine *engine, VertexBuffer *buffer)
{
    ID3D12GraphicsCommandList *graphics_list = engine->renderer.graphics_lists[engine->renderer.current_buffer];

    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = buffer->res->lpVtbl->GetGPUVirtualAddress(buffer->res);
    vbv.SizeInBytes    = buffer->count * buffer->stride;
    vbv.StrideInBytes  = buffer->stride;

    graphics_list->lpVtbl->IASetVertexBuffers(graphics_list, 0, 1, &vbv);
}

void DrawVertices(Engine *engine, VertexBuffer *buffer)
{
    ID3D12GraphicsCommandList *graphics_list = engine->renderer.graphics_lists[engine->renderer.current_buffer];
    graphics_list->lpVtbl->DrawInstanced(graphics_list, buffer->count, 1, 0, 0);
}

//
// IndexBuffer
//

IndexBuffer CreateIndexBuffer(Engine *engine, u32 *indices, u32 count)
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

    CopyMemory(index_data, indices, buffer.count * sizeof(u32));

    buffer.res->lpVtbl->Unmap(buffer.res, 0, 0);

    return buffer;
}

void DestroyIndexBuffer(IndexBuffer *buffer)
{
    SafeRelease(buffer->res);
    buffer->count = 0;   
}

void SetIndexBuffer(Engine *engine, IndexBuffer *buffer)
{
    ID3D12GraphicsCommandList *graphics_list = engine->renderer.graphics_lists[engine->renderer.current_buffer];

    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = buffer->res->lpVtbl->GetGPUVirtualAddress(buffer->res);
    ibv.SizeInBytes    = buffer->count * sizeof(u32);
    ibv.Format         = DXGI_FORMAT_R32_UINT;

    graphics_list->lpVtbl->IASetIndexBuffer(graphics_list, &ibv);
}

void DrawIndices(Engine *engine, IndexBuffer *buffer)
{
    ID3D12GraphicsCommandList *graphics_list = engine->renderer.graphics_lists[engine->renderer.current_buffer];
    graphics_list->lpVtbl->DrawIndexedInstanced(graphics_list, buffer->count, 1, 0, 0, 0);
}

//
// Shader
//

internal void CompileShader(Engine *engine, Shader *shader, ShaderDesc *desc, D3D_SHADER_MACRO *predefines)
{
    u32 compile_flags = 0; // D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR ?
#if DEBUG
    compile_flags |= D3DCOMPILE_DEBUG
                  |  D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3
                  |  D3DCOMPILE_SKIP_VALIDATION
                  |  D3DCOMPILE_PARTIAL_PRECISION;
#endif

    ID3DBlob *errors = 0;
    engine->renderer.error = D3DCompile(desc->code_start,
                                        desc->code_end - desc->code_start,
                                        desc->name,
                                        predefines,
                                        shader->include,
                                        desc->entry_point,
                                        desc->target,
                                        compile_flags,
                                        0,
                                        &shader->blob,
                                        &errors);
    if (FAILED(engine->renderer.error))
    {
        MessageBoxA(0, errors->lpVtbl->GetBufferPointer(errors), "Shader compilation failure", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }
    shader->bytecode.pShaderBytecode = shader->blob->lpVtbl->GetBufferPointer(shader->blob);
    shader->bytecode.BytecodeLength  = shader->blob->lpVtbl->GetBufferSize(shader->blob);

    SafeRelease(errors);
}

//
// GraphicsProgram
//

CEXTERN void ParseShaders(Engine *engine, const char *file_with_shaders, GraphicsProgramDesc *gpd);

void GraphicsProgram_Create(Engine *engine, const char *file_with_shaders, D3D_SHADER_MACRO *predefines, GraphicsProgram *graphics_program)
{
    Check(engine);
    Check(file_with_shaders);
    Check(graphics_program);

    GraphicsProgramDesc gpd = {0};
    ParseShaders(engine, file_with_shaders, &gpd);

    graphics_program->shaders_count = gpd.sd.count;

    for (u32 i = 0; i < graphics_program->shaders_count; ++i)
    {
        CompileShader(engine,
                      graphics_program->shaders + i,
                      gpd.sd.descs + i,
                      predefines);
    }

    // @TODO(Roman): Parse root signature stuff from shaders
    D3D12_ROOT_SIGNATURE_DESC rsd;
    rsd.NumParameters     = 0;
    rsd.pParameters       = 0;
    rsd.NumStaticSamplers = 0;
    rsd.pStaticSamplers   = 0;
    rsd.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                        // @TODO(Roman): support stream output
                        /*| D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT*/;

    ID3DBlob *error = 0;
    engine->renderer.error = D3D12SerializeRootSignature(&rsd,
                                                         D3D_ROOT_SIGNATURE_VERSION_1_0,
                                                         &graphics_program->signature,
                                                         &error);
#if DEVDEBUG
    if (FAILED(engine->renderer.error))
    {
        MessageBoxA(0, error->lpVtbl->GetBufferPointer(error), "Debug Error!", MB_OK | MB_ICONERROR);
        __debugbreak();
        ExitProcess(1);
    }
#else
    Check(SUCCEEDED(engine->renderer.error));
#endif

    SafeRelease(error);

    engine->renderer.error = engine->renderer.device->lpVtbl->CreateRootSignature(engine->renderer.device,
                                                                                  0,
                                                                                  graphics_program->signature->lpVtbl->GetBufferPointer(graphics_program->signature),
                                                                                  graphics_program->signature->lpVtbl->GetBufferSize(graphics_program->signature),
                                                                                  &IID_ID3D12RootSignature,
                                                                                  &graphics_program->root_signature);
    Check(SUCCEEDED(engine->renderer.error));

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
    rasterizer_desc.MultisampleEnable     = false;
    rasterizer_desc.AntialiasedLineEnable = false;
    rasterizer_desc.ForcedSampleCount     = 0;
    rasterizer_desc.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_DEPTH_STENCIL_DESC depth_desc;
    depth_desc.DepthEnable                  = gpd.psd.depth_test_enabled;
    depth_desc.DepthWriteMask               = gpd.psd.blending_enabled ? D3D12_DEPTH_WRITE_MASK_ZERO : D3D12_DEPTH_WRITE_MASK_ALL;
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
    gpsd.pRootSignature        = graphics_program->root_signature;
    for (u32 i = 0; i < graphics_program->shaders_count; ++i)
    {
        Shader *shader = graphics_program->shaders + i;

        switch (*gpd.sd.descs[i].target)
        {
            case 'v':
            {
                Check(!gpsd.VS.pShaderBytecode);
                gpsd.VS = shader->bytecode;
            } break;
            
            case 'h':
            {
                Check(!gpsd.HS.pShaderBytecode);
                gpsd.HS = shader->bytecode;
            } break;
            
            case 'd':
            {
                Check(!gpsd.DS.pShaderBytecode);
                gpsd.DS = shader->bytecode;
            } break;
            
            case 'g':
            {
                Check(!gpsd.GS.pShaderBytecode);
                gpsd.GS = shader->bytecode;
            } break;
            
            case 'p':
            {
                Check(!gpsd.PS.pShaderBytecode);
                gpsd.PS = shader->bytecode;
            } break;
        }
    }
    gpsd.StreamOutput          = sod;
    gpsd.BlendState            = blend_desc;
    gpsd.SampleMask            = UINT_MAX;
    gpsd.RasterizerState       = rasterizer_desc;
    gpsd.DepthStencilState     = depth_desc;
    gpsd.InputLayout           = gpd.psd.input_layout;
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
                                                                                          &graphics_program->pipeline_state);
    Check(SUCCEEDED(engine->renderer.error));
}

void GraphicsProgram_Destroy(GraphicsProgram *graphics_program)
{
    for (u32 i = 0; i < graphics_program->shaders_count; ++i)
    {
        SafeRelease(graphics_program->shaders[i].blob);
    }
    SafeRelease(graphics_program->pipeline_state);
    SafeRelease(graphics_program->root_signature);
    SafeRelease(graphics_program->signature);
}

void GraphicsProgram_Bind(Engine *engine, GraphicsProgram *graphics_program)
{
    ID3D12GraphicsCommandList *graphics_list = engine->renderer.graphics_lists[engine->renderer.current_buffer];

    graphics_list->lpVtbl->SetGraphicsRootSignature(graphics_list, graphics_program->root_signature);

    graphics_list->lpVtbl->IASetPrimitiveTopology(graphics_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphics_list->lpVtbl->SetPipelineState(graphics_list, graphics_program->pipeline_state);
}

void GraphicsProgram_SetConstants(Engine *engine, GraphicsProgram *graphics_program, void *constants, u32 slot_index, u32 count)
{
}

void GraphicsProgram_SetBuffer(Engine *engine, GraphicsProgram *graphics_program, ID3D12Resource *buffers, u32 slot_index)
{
}

void GraphicsProgram_SetTables(Engine *engine, GraphicsProgram *graphics_program, ID3D12DescriptorHeap **heap_desc, u32 *slot_indices, u32 *desc_sizes, u32 count)
{
}

//
// Useful-ish garbage :)
//

#if 0

    void SetShaderConstants(Engine *engine, ShaderResources *shader_resources, u32 index, void *constants, u32 count, u32 start_offset)
    {
        Check(engine);
        Check(shader_resources);
        Check(constants);

        if (count)
        {
            switch (shader_resources->kind)
            {
                case SHADER_RESOURCES_KIND_GRAPHICS:
                {
                    ID3D12GraphicsCommandList *graphics_list = engine->renderer.graphics_lists[engine->renderer.current_buffer];
                    graphics_list->lpVtbl->SetGraphicsRoot32BitConstants(graphics_list, index, count, constants, start_offset);
                } break;

                case SHADER_RESOURCES_KIND_COMPUTE:
                {
                    ID3D12GraphicsCommandList *compute_list = engine->renderer.compute_lists[engine->renderer.current_buffer];
                    compute_list->lpVtbl->SetComputeRoot32BitConstants(compute_list, index, count, constants, start_offset);
                } break;
            }
        }
    }

    {
        case SHADERS_RESOURCE_TYPE_BUFFER_CBV:
        {
            ID3D12Resource *resource = data;
            graphics_list->lpVtbl->SetGraphicsRootConstantBufferView(graphics_list, index, resource->lpVtbl->GetGPUVirtualAddress(resource));
        } break;
        case SHADERS_RESOURCE_TYPE_BUFFER_SRV:
        {
            ID3D12Resource *resource = data;
            graphics_list->lpVtbl->SetGraphicsRootShaderResourceView(graphics_list, index, resource->lpVtbl->GetGPUVirtualAddress(resource));
        } break;
        case SHADERS_RESOURCE_TYPE_BUFFER_UAV:
        {
            ID3D12Resource *resource = data;
            graphics_list->lpVtbl->SetGraphicsRootUnorderedAccessView(graphics_list, index, resource->lpVtbl->GetGPUVirtualAddress(resource));
        } break;
        case SHADERS_RESOURCE_TYPE_TABLE:
        {
            graphics_list->lpVtbl->SetDescriptorHeaps(graphics_list, desc->tables.count, desc->tables.heap_descs);
            for (u32 i = 0; i < desc->tables.heap_descs; ++i)
            {
                ID3D12DescriptorHeap *heap = desc->tables.heap_descs[i];
                D3D12_GPU_DESCRIPTOR_HANDLE base_desc = heap->lpVtbl->GetGPUDescriptorHandleForHeapStart(heap);
                base_desc.ptr += index * desc->tables.descs_sizes[i];
                graphics_list->lpVtbl->SetGraphicsRootDescriptorTable(graphics_list, index, base_desc);
            }
        } break;
    }

    //
    // Compute PipelineState
    //

    void CreateComputePipelineState(
        IN  Engine           *engine,
        IN  Shader           *compute_shader,
        IN  ShadersResources *shaders_resources,
        OUT PipelineState    *pipeline_state)
    {
        Check(engine);
        Check(compute_shader);
        Check(shaders_resources);
        Check(pipeline_state);
        CheckM(shaders_resources->kind == SHADERS_RESOURCES_KIND_COMPUTE, "SHADERS_RESOURCES_KIND must be SHADERS_RESOURCES_KIND_COMPUTE if you want to create compute pipeline state");

        pipeline_state->shaders_resources = shaders_resources;
        pipeline_state->kind              = PIPELINE_STATE_KIND_COMPUTE;

        D3D12_CACHED_PIPELINE_STATE cached_pipeline_state;
        cached_pipeline_state.pCachedBlob           = 0;
        cached_pipeline_state.CachedBlobSizeInBytes = 0;

        // @TODO(Roman): Cached pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC cpsd;
        cpsd.pRootSignature = pipeline_state->shaders_resources->root_signature;
        cpsd.CS             = compute_shader->bytecode;
        cpsd.NodeMask       = 0;
        cpsd.CachedPSO      = cached_pipeline_state;
        cpsd.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;

        engine->renderer.error = engine->renderer.device->lpVtbl->CreateComputePipelineState(engine->renderer.device,
                                                                                             &cpsd,
                                                                                             &IID_ID3D12PipelineState,
                                                                                             &pipeline_state->pipeline_state);
        Check(SUCCEEDED(engine->renderer.error));
    }

    void ExecuteComputePipelineState(Engine *engine, PipelineState *pipeline_state, u32 grids, u32 blocks, u32 threads)
    {
        Check(engine);
        Check(pipeline_state);
        Check(pipeline_state->kind == PIPELINE_STATE_KIND_COMPUTE);

        ID3D12GraphicsCommandList *compute_list = engine->renderer.compute_lists[engine->renderer.current_buffer];
        compute_list->lpVtbl->Dispatch(compute_list, threads, blocks, grids);
    }
#endif
