//
// GraphicsProgram
//

#pragma once

#include "core/core.h"

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
        u32 count;
        ShaderDesc descs[MAX_GRAPHICS_SHADERS];
    } sd;

    struct
    {
        D3D12_INPUT_LAYOUT_DESC input_layout;
        b32 blending_enabled;
        b32 depth_test_enabled;
        D3D12_CULL_MODE cull_mode;
    } psd;

    struct
    {
        D3D12_ROOT_SIGNATURE_DESC desc;
    } rsd;
} GraphicsProgramDesc;

typedef struct CBV_SRV_UAV_Buffer CBV_SRV_UAV_Buffer;
struct CBV_SRV_UAV_Buffer
{
    CBV_SRV_UAV_Buffer        *next;
    const char                *name;
    u64                        name_len;
    D3D12_ROOT_PARAMETER_TYPE  type;
};

typedef struct GraphicsProgram
{
    ID3DBlob            *signature;
    ID3D12RootSignature *root_signature;

    ID3D12PipelineState *pipeline_state;

    CBV_SRV_UAV_Buffer *buffers;

    u32    shaders_count;
    Shader shaders[MAX_GRAPHICS_SHADERS];
} GraphicsProgram;

CENGINE_FUN void CreateGraphicsProgram(
    IN       Engine           *engine,
    IN       const char       *file_with_shaders,
    OPTIONAL D3D_SHADER_MACRO *predefines,
    OUT      GraphicsProgram  *graphics_program
);

CENGINE_FUN void DestroyGraphicsProgram(
    IN GraphicsProgram *graphics_program
);

CENGINE_FUN void BindGraphicsProgram(
    IN Engine          *engine,
    IN GraphicsProgram *graphics_program
);

#if 0
CENGINE_FUN void SetGraphicsProgramConstants(
    IN Engine          *engine,
    IN GraphicsProgram *graphics_program,
    IN void            *constants,
    IN u32              slot_index,
    IN u32              count
);
#endif

CENGINE_FUN void SetGraphicsProgramBuffer(
    IN Engine          *engine,
    IN GraphicsProgram *graphics_program,
    IN const char      *name,
    IN u64              name_len,
    IN GPUMemoryBlock  *buffer
);

CENGINE_FUN void SetGraphicsProgramTables(
    IN Engine                *engine,
    IN GraphicsProgram       *graphics_program,
    IN ID3D12DescriptorHeap **heap_desc,
    IN u32                   *slot_indices,
    IN u32                   *desc_sizes,
    IN u32                    count
);
