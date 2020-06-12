//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/core/core_renderer.h"
#include "graphics/shader_parser/shader_parser.h"
#include "gpu/gpu_memory_manager.h"
#include "cengine.h"

enum GPU_MEMORY_BLOCK_KIND
{
    GPU_MEMORY_BLOCK_KIND_VB  = 1,
    GPU_MEMORY_BLOCK_KIND_IB,
    GPU_MEMORY_BLOCK_KIND_CBV,
    GPU_MEMORY_BLOCK_KIND_SRV,
    GPU_MEMORY_BLOCK_KIND_UAV,
    GPU_MEMORY_BLOCK_KIND_SAMPLER,
};

#define SafeRelease(directx_interface)                           \
{                                                                \
    if (directx_interface)                                       \
    {                                                            \
        (directx_interface)->lpVtbl->Release(directx_interface); \
        (directx_interface) = 0;                                 \
    }                                                            \
}

//
// Shader
//

internal void CompileShader(Engine *engine, Shader *shader, ShaderDesc *desc, D3D_SHADER_MACRO *predefines)
{
    u32 compile_flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#if DEBUG
    compile_flags |= D3DCOMPILE_DEBUG
                  |  D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3
                  |  D3DCOMPILE_SKIP_VALIDATION
                  |  D3DCOMPILE_PARTIAL_PRECISION;
#endif

    ID3DBlob *errors = 0;
    engine->gpu_manager.error = D3DCompile(desc->code_start,
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
    if (FAILED(engine->gpu_manager.error))
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

void CreateGraphicsProgram(
    IN       Engine           *engine,
    IN       const char       *file_with_shaders,
    OPTIONAL D3D_SHADER_MACRO *predefines,
    OUT      GraphicsProgram  *graphics_program)
{
    Check(engine);
    Check(file_with_shaders);
    Check(graphics_program);

    GraphicsProgramDesc gpd = {0};
    // default values:
    gpd.psd.blending_enabled   = false;
    gpd.psd.depth_test_enabled = true;
    gpd.psd.cull_mode          = D3D12_CULL_MODE_NONE;
    gpd.rsd.desc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                             // @TODO(Roman): support stream output
                             /*| D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT*/;

    ParseGraphicsShaders(engine, file_with_shaders, graphics_program, &gpd);

    graphics_program->shaders_count = gpd.sd.count;

    for (u32 i = 0; i < graphics_program->shaders_count; ++i)
    {
        CompileShader(engine,
                      graphics_program->shaders + i,
                      gpd.sd.descs + i,
                      predefines);
    }

    ID3DBlob *error = 0;
    engine->gpu_manager.error = D3D12SerializeRootSignature(&gpd.rsd.desc,
                                                            D3D_ROOT_SIGNATURE_VERSION_1_0,
                                                            &graphics_program->signature,
                                                            &error);
#if DEVDEBUG
    if (FAILED(engine->gpu_manager.error))
    {
        MessageBoxA(0, error->lpVtbl->GetBufferPointer(error), "Debug Error!", MB_OK | MB_ICONERROR);
        __debugbreak();
        ExitProcess(1);
    }
#else
    Check(SUCCEEDED(engine->gpu_manager.error));
#endif

    SafeRelease(error);

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateRootSignature(engine->gpu_manager.device,
                                                                                        1,
                                                                                        graphics_program->signature->lpVtbl->GetBufferPointer(graphics_program->signature),
                                                                                        graphics_program->signature->lpVtbl->GetBufferSize(graphics_program->signature),
                                                                                        &IID_ID3D12RootSignature,
                                                                                        &graphics_program->root_signature);
    Check(SUCCEEDED(engine->gpu_manager.error));

    // @TODO(Roman): support stream output
    D3D12_STREAM_OUTPUT_DESC sod;
    sod.pSODeclaration   = 0;
    sod.NumEntries       = 0;
    sod.pBufferStrides   = 0;
    sod.NumStrides       = 0;
    sod.RasterizedStream = 0;

    D3D12_BLEND_DESC blend_desc = {0};
    if (gpd.psd.blending_enabled)
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
    rasterizer_desc.CullMode              = gpd.psd.cull_mode;
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
    gpsd.pRootSignature = graphics_program->root_signature;
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
    if (gpd.psd.blending_enabled)
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
    gpsd.NodeMask              = 1;
    gpsd.CachedPSO             = cached_pipeline_state;
    gpsd.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateGraphicsPipelineState(engine->gpu_manager.device,
                                                                                                    &gpsd,
                                                                                                    &IID_ID3D12PipelineState,
                                                                                                    &graphics_program->pipeline_state);
    Check(SUCCEEDED(engine->gpu_manager.error));
}

void DestroyGraphicsProgram(
    IN GraphicsProgram *graphics_program)
{
    for (u32 i = 0; i < graphics_program->shaders_count; ++i)
    {
        SafeRelease(graphics_program->shaders[i].blob);
    }
    SafeRelease(graphics_program->pipeline_state);
    SafeRelease(graphics_program->root_signature);
    SafeRelease(graphics_program->signature);
}

void BindGraphicsProgram(
    IN Engine          *engine,
    IN GraphicsProgram *graphics_program)
{
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    graphics_list->lpVtbl->SetGraphicsRootSignature(graphics_list, graphics_program->root_signature);

    graphics_list->lpVtbl->IASetPrimitiveTopology(graphics_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphics_list->lpVtbl->SetPipelineState(graphics_list, graphics_program->pipeline_state);
}

#if 0
void SetGraphicsProgramConstants(
    IN Engine          *engine,
    IN GraphicsProgram *graphics_program,
    IN void            *constants,
    IN u32              slot_index,
    IN u32              count)
{
    Check(engine);
    Check(graphics_program);

    if (count)
    {
        ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
        graphics_list->lpVtbl->SetGraphicsRoot32BitConstants(graphics_list, slot_index, count, constants, 0);
    }
}
#endif

void SetGraphicsProgramBuffer(
    IN Engine          *engine,
    IN GraphicsProgram *graphics_program,
    IN const char      *name,
    IN u64              name_len,
    IN GPUMemoryBlock  *buffer)
{
    Check(engine);
    Check(graphics_program);
    Check(name);
    Check(name_len);
    Check(buffer);

    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    u32 index = 0;
    for (CBV_SRV_UAV_Buffer *it = graphics_program->buffers; it; ++it)
    {
        if (it->name_len == name_len && RtlEqualMemory(it->name, name, name_len))
        {
            break;
        }
        ++index;
    }

    switch (graphics_program->buffers[index].type)
    {
        case D3D12_ROOT_PARAMETER_TYPE_CBV:
        {
            CheckM(buffer->kind == GPU_MEMORY_BLOCK_KIND_CBV, "The GPU memory block you're trying to bind to a pipeline constant buffer is not a constant buffer");
            graphics_list->lpVtbl->SetGraphicsRootConstantBufferView(graphics_list, index, buffer->resource->lpVtbl->GetGPUVirtualAddress(buffer->resource));
        } break;

        case D3D12_ROOT_PARAMETER_TYPE_SRV:
        {
            CheckM(buffer->kind == GPU_MEMORY_BLOCK_KIND_SRV, "The GPU memory block you're trying to bind to a pipeline shader resource is not a shader resource");
            graphics_list->lpVtbl->SetGraphicsRootShaderResourceView(graphics_list, index, buffer->resource->lpVtbl->GetGPUVirtualAddress(buffer->resource));
        } break;

        case D3D12_ROOT_PARAMETER_TYPE_UAV:
        {
            CheckM(buffer->kind == GPU_MEMORY_BLOCK_KIND_UAV, "The GPU memory block you're trying to bind to a pipeline unordered access resource is not an unordered access resource");
            graphics_list->lpVtbl->SetGraphicsRootUnorderedAccessView(graphics_list, index, buffer->resource->lpVtbl->GetGPUVirtualAddress(buffer->resource));
        } break;
    }
}

void SetGraphicsProgramTables(
    IN Engine                *engine,
    IN GraphicsProgram       *graphics_program,
    IN ID3D12DescriptorHeap **heap_desc,
    IN u32                   *slot_indices,
    IN u32                   *desc_sizes,
    IN u32                    count)
{
    // case SHADERS_RESOURCE_TYPE_TABLE:
    // {
    //     graphics_list->lpVtbl->SetDescriptorHeaps(graphics_list, desc->tables.count, desc->tables.heap_descs);
    //     for (u32 i = 0; i < desc->tables.heap_descs; ++i)
    //     {
    //         ID3D12DescriptorHeap *heap = desc->tables.heap_descs[i];
    //         D3D12_GPU_DESCRIPTOR_HANDLE base_desc = heap->lpVtbl->GetGPUDescriptorHandleForHeapStart(heap);
    //         base_desc.ptr += index * desc->tables.descs_sizes[i];
    //         graphics_list->lpVtbl->SetGraphicsRootDescriptorTable(graphics_list, index, base_desc);
    //     }
    // } break;
}

//
// Useful-ish garbage :)
//

#if 0
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
        cpsd.NodeMask       = 1;
        cpsd.CachedPSO      = cached_pipeline_state;
        cpsd.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;

        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateComputePipelineState(engine->gpu_manager.device,
                                                                                             &cpsd,
                                                                                             &IID_ID3D12PipelineState,
                                                                                             &pipeline_state->pipeline_state);
        Check(SUCCEEDED(engine->gpu_manager.error));
    }

    void ExecuteComputePipelineState(Engine *engine, PipelineState *pipeline_state, u32 grids, u32 blocks, u32 threads)
    {
        Check(engine);
        Check(pipeline_state);
        Check(pipeline_state->kind == PIPELINE_STATE_KIND_COMPUTE);

        ID3D12GraphicsCommandList *compute_list = engine->gpu_manager.compute_lists[engine->gpu_manager.current_buffer];
        compute_list->lpVtbl->Dispatch(compute_list, threads, blocks, grids);
    }
#endif
