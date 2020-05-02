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

internal byte *ReadEntireShaderFile(Engine *engine, const char *filename, u32 *filesize)
{
    if (filename)
    {
        HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
        CheckM(file != INVALID_HANDLE_VALUE, "File does not exist");

        *filesize    = GetFileSize(file, 0);
        byte *buffer = PushToTransientArea(engine->memory, *filesize);

        DebugResult(ReadFile(file, buffer, *filesize, 0, 0));

        DebugResult(CloseHandle(file));
        return buffer;
    }
    return 0;
}

// @TODO(Roman): remove or rewrite
internal void CompileShader(
    IN       Engine           *engine,
    IN       const char       *filename,
    OPTIONAL const char       *name,
    OPTIONAL D3D_SHADER_MACRO *defines,
    OPTIONAL ID3DInclude      *includes,
    IN       const char       *entry_point,
    IN       const char       *target,
    OUT      Shader           *shader)
{
    shader->name        = name;
    shader->defines     = defines;
    shader->include     = includes;
    shader->entry_point = entry_point;
    shader->target      = target;

    u32   source_size = 0;
    byte *source      = ReadEntireShaderFile(engine, filename, &source_size);

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
    engine->renderer.error = D3DCompile(source,
                                        source_size,
                                        shader->name,
                                        shader->defines,
                                        shader->include,
                                        shader->entry_point,
                                        shader->target,
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

/////////////////////////////////////////////////////////////
//// EVERYTHING BELOW IS BROKEN       ///////////////////////
/////////////////////////////////////////////////////////////

//
// GraphicsProgram
//

typedef struct Token
{
    const char *start;
    const char *end;
} Token;

internal byte *GetNextToken(Token *token, byte *code)
{
    while (*code && isspace(*code)) ++code;

    if (*code)
    {
        token->start = code;
        if (*code == '#' || *code == '(' || *code == ')' || *code == '"')
        {
            token->end = ++code;
        }
        else
        {
            while (*code && (!isspace(*code) || *code != '(' || *code != ')' || *code != '"'))
            {
                ++code;
            }
            token->end = code;
        }
    }
    return code;
}

internal void CheckSyntax(Token *token, const char *expected, u32 expected_len)
{
    if ((token->end - token->start != expected_len)
    ||  memcmp(token->start, expected, token->end - token->start))
    {
        MessageF(MESSAGE_TYPE_ERROR,
                 "Shader syntax error: exected '%s', got '%.*s'",
                 expected,
                 token->end - token->start, token->start);
    }
}
#define CheckSyntaxCSTR(token, expected) CheckSyntax(token, expected, CSTRLEN(expected))
#define CheckSyntaxChar(token, expected) CheckSyntax(token, expected, 1)

#define TokenEqualsCSTR(token, cstr)                                 \
    (((token)->end - (token)->start == CSTRLEN(cstr))                \
    && !memcmp((token)->start, cstr, (token)->end - (token)->start))

internal byte *ParsePragma(Engine *engine, Token *token, byte *code, GraphicsProgram *program)
{
    code = GetNextToken(&token, code);

    if (TokenEqualsCSTR(token, "type"))
    {
        code = GetNextToken(&token, code);
        CheckSyntaxChar(token, '(');
        code = GetNextToken(&token, code);

        Shader *shader = program->shaders + program->shaders_count;
        if (TokenEqualsCSTR(token, "vertex"))
        {
            shader->target = "vs_5_0";
            shader->entry_point = "VSMain";
        }
        else if (TokenEqualsCSTR(token, "hull"))
        {
            shader->target = "hs_5_0";
            shader->entry_point = "HSMain";
        }
        else if (TokenEqualsCSTR(token, "domain"))
        {
            shader->target = "ds_5_0";
            shader->entry_point = "DSMain";
        }
        else if (TokenEqualsCSTR(token, "geometry"))
        {
            shader->target = "gs_5_0";
            shader->entry_point = "GSMain";
        }
        else if (TokenEqualsCSTR(token, "pixel"))
        {
            shader->target = "ps_5_0";
            shader->entry_point = "PSMain";
        }
        else
        {
            MessageF(MESSAGE_TYPE_ERROR,
                     "Shader syntax error: wrong shader type "
                     "expected 'vertex' or 'hull' or 'domain' "
                     "or 'geometry' or 'pixel', got %.*s",
                     token->end - token->start, token->start);
        }
    }
    else if (TokenEqualsCSTR(token, "name"))
    {
        code = GetNextToken(&token, code);
        CheckSyntaxChar(token, '(');
        code = GetNextToken(&token, code);
        CheckSyntaxChar(token, '"');
        code = GetNextToken(&token, code);

        Shader *shader = program->shaders + program->shaders_count;
        shader->name = PushToTA(char, engine->memory, token->end - token->start);
        CopyMemory(shader->name, token->start, token->end - token->start);

        code = GetNextToken(&token, code);
        CheckSyntaxChar(token, '"');
    }

    code = GetNextToken(&token, code);
    CheckSyntaxChar(token, ')');
    return code = GetNextToken(&token, code);
}

internal byte *ParseDefine(Engine *engine, Token *token, byte *code, GraphicsProgram *program)
{
    code = GetNextToken(&token, code);

    Shader *shader = program->shaders + program->shaders_count;

    // shader->defines[0].Definition = ;
}

internal void ParseFileWithShaders(Engine *engine, const char *file, GraphicsProgram *program)
{
    u32   size = 0;
    byte *code = ReadEntireShaderFile(engine, file, size);

    Token token = {0};
    code = GetNextToken(&token, code);

    while (*code)
    {
        if (*token.start == '#')
        {
            code = GetNextToken(&token, code);
            if (TokenEqualsCSTR(&token, "pragma"))
            {
                code = ParsePragma(engine, &token, code, program);
            }
            else if (TokenEqualsCSTR(&token, "define"))
            {
                code = ParseDefine(engine, &token, code, program);
            }
        }
    }
}

void GraphicsProgram_Create(Engine *engine, const char *file_with_shaders, GraphicsProgram *graphics_program)
{
#if 0
    ParseFileWithShaders(engine, file_with_shaders, graphics_program);

    ID3DBlob *error = 0;
    engine->renderer.error = D3D12SerializeRootSignature(&rsd,
                                                         D3D_ROOT_SIGNATURE_VERSION_1_0,
                                                         &shader_resources->signature,
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
                                                                                  shader_resources->signature->lpVtbl->GetBufferPointer(shader_resources->signature),
                                                                                  shader_resources->signature->lpVtbl->GetBufferSize(shader_resources->signature),
                                                                                  &IID_ID3D12RootSignature,
                                                                                  &shader_resources->root_signature);
    Check(SUCCEEDED(engine->renderer.error));
#endif
}

void GraphicsProgram_Destroy(GraphicsProgram *graphics_program)
{
}

void GraphicsProgram_Bind(Engine *engine, GraphicsProgram *graphics_program)
{
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

#if 0
    void BindShaderResources(Engine *engine, ShaderResources *shader_resources)
    {
        Check(engine);
        Check(shader_resources);

        switch (shader_resources->kind)
        {
            case SHADER_RESOURCES_KIND_GRAPHICS:
            {
                ID3D12GraphicsCommandList *graphics_list = engine->renderer.graphics_lists[engine->renderer.current_buffer];
                graphics_list->lpVtbl->SetGraphicsRootSignature(graphics_list, shader_resources->root_signature);
            } break;

            case SHADER_RESOURCES_KIND_COMPUTE:
            {
                ID3D12GraphicsCommandList *compute_list = engine->renderer.compute_lists[engine->renderer.current_buffer];
                compute_list->lpVtbl->SetGraphicsRootSignature(compute_list, shader_resources->root_signature);
            } break;
        }
    }

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
    // PipelineState
    //

    void CreateGraphicsPipelineState(
        IN       Engine              *engine,
        IN       Shader              *vertex_shader,
        OPTIONAL Shader              *hull_shader,
        OPTIONAL Shader              *domain_shader,
        OPTIONAL Shader              *geometry_shader,
        IN       Shader              *pixel_shader,
        IN       b32                  enable_blending,
        IN       u32                  shader_args_count,
        IN       ShaderArg           *shader_args,
        IN       ShadersResources    *shaders_resources,
        OUT      PipelineState       *pipeline_state)
    {
        Check(engine);
        Check(vertex_shader);
        Check(pixel_shader);
        Check(shader_args && shader_args_count);
        Check(shaders_resources);
        Check(pipeline_state);
        CheckM(shaders_resources->kind == SHADERS_RESOURCES_KIND_GRAPHICS, "SHADERS_RESOURCES_KIND must be SHADERS_RESOURCES_KIND_GRAPHICS if you want to create graphics pipeline state");

        pipeline_state->shaders_resources = shaders_resources;
        pipeline_state->kind              = PIPELINE_STATE_KIND_GRAPHICS;

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
        gpsd.pRootSignature        = pipeline_state->shaders_resources->root_signature;
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
                                                                                              &pipeline_state->pipeline_state);
        Check(SUCCEEDED(engine->renderer.error));
    }

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

    void DestroyPipelineState(PipelineState *pipeline_state)
    {
        Check(pipeline_state);
        SafeRelease(pipeline_state->pipeline_state);
        pipeline_state->shaders_resources = 0;
    }

    void SetPipelineState(Engine *engine, PipelineState *pipeline_state)
    {
        Check(engine);
        Check(pipeline_state);

        switch (pipeline_state->kind)
        {
            case PIPELINE_STATE_KIND_GRAPHICS:
            {
                ID3D12GraphicsCommandList *graphics_list = engine->renderer.graphics_lists[engine->renderer.current_buffer];

                graphics_list->lpVtbl->IASetPrimitiveTopology(graphics_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                graphics_list->lpVtbl->SetPipelineState(graphics_list, pipeline_state->pipeline_state);
            } break;

            case PIPELINE_STATE_KIND_COMPUTE:
            {
                ID3D12GraphicsCommandList *compute_list = engine->renderer.compute_lists[engine->renderer.current_buffer];

                compute_list->lpVtbl->SetPipelineState(compute_list, pipeline_state->pipeline_state);
            } break;
        }
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
