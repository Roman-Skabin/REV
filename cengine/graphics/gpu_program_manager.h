//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "gpu/gpu_memory_manager.h"
#include "tools/buffer.h"

enum
{
    MAX_GRAPHICS_SHADERS = 5, // max various graphics shaders for one pipeline
};

typedef struct Shader
{
    ID3DInclude           *include;
    ID3DBlob              *blob;
    D3D12_SHADER_BYTECODE  bytecode;
} Shader;

typedef struct ShaderDesc
{
    char        target[8];
    const char *name;
    const char *entry_point;
    u64         entry_point_len;
    const char *code_start;
    const char *code_end;
} ShaderDesc;

typedef struct GraphicsProgramDesc
{
    struct
    {
        const char *filename;
        u32         count;
        ShaderDesc  descs[MAX_GRAPHICS_SHADERS];
    } sd;

    struct
    {
        D3D12_INPUT_LAYOUT_DESC input_layout;
        b32                     blending_enabled;
        b32                     depth_test_enabled;
        D3D12_CULL_MODE         cull_mode;
    } psd;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsd;
} GraphicsProgramDesc;

typedef struct GraphicsProgram GraphicsProgram;
struct GraphicsProgram
{
    ExtendsList(GraphicsProgram);
    ID3DBlob             *signature;
    ID3D12RootSignature  *root_signature;
    ID3D12PipelineState  *pipeline_state;
    // @NOTE(Roman):      Vertex and index buffers creates separately
    //                    from the graphics program. At least while we
    //                    do not have a UI.
    BUF GPUResource     **resources;
    u32                   shaders_count;
    Shader                shaders[MAX_GRAPHICS_SHADERS];
};

typedef struct GPUProgramManager
{
    LIST GraphicsProgram *graphics_programs;
    // @TODO(Roman): Compute programs
} GPUProgramManager;

CENGINE_FUN GraphicsProgram *AddGraphicsProgram(
    in  Engine           *engine,
    in  const char       *file_with_shaders,
    opt D3D_SHADER_MACRO *predefines
);

CENGINE_FUN void DestroyGraphicsProgram(
    in GraphicsProgram *graphics_program
);

CENGINE_FUN void BindGraphicsProgram(
    in Engine          *engine,
    in GraphicsProgram *graphics_program
);
