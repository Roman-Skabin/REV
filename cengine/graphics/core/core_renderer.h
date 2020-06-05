//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "math/vec.h"

enum _CORE_RENDERER_CONSTANTS
{
    SWAP_CHAIN_BUFFERS_COUNT = 2,
};

typedef struct CoreRenderer
{
#if DEBUG
    ID3D12Debug                 *debug;
    ID3D12DebugCommandQueue     *deubg_queue;
    ID3D12DebugCommandList      *debug_list;
#endif
    IDXGIFactory2               *factory;
    IDXGIAdapter1               *adapter;
    ID3D12Device                *device;

    ID3D12CommandQueue          *queue;
    ID3D12CommandAllocator      *graphics_allocators[SWAP_CHAIN_BUFFERS_COUNT];
    ID3D12GraphicsCommandList   *graphics_lists[SWAP_CHAIN_BUFFERS_COUNT];
    IDXGISwapChain4             *swap_chain;

    ID3D12CommandAllocator      *compute_allocators[SWAP_CHAIN_BUFFERS_COUNT];
    ID3D12GraphicsCommandList   *compute_lists[SWAP_CHAIN_BUFFERS_COUNT];

    ID3D12DescriptorHeap        *rtv_heap_desc;
    ID3D12Resource              *rt_buffers[SWAP_CHAIN_BUFFERS_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE  rtv_cpu_desc_handle;
    u32                          rtv_desc_size;
    u32                          current_buffer;

    ID3D12DescriptorHeap        *ds_heap_desc;
    ID3D12Resource              *ds_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE  dsv_cpu_desc_handle;

    ID3D12Fence                 *fences[SWAP_CHAIN_BUFFERS_COUNT];
    u64                          fences_values[SWAP_CHAIN_BUFFERS_COUNT];
    HANDLE                       fence_event;

    b32                          vsync;
    b32                          first_frame;
    b32                          tearing_supported;

    HRESULT                      error;
} CoreRenderer;

CENGINE_FUN void SetVSync(Engine *engine, b32 enable);

//
// VertexBuffer
//

typedef struct VertexBuffer
{
    ID3D12Resource *res;
    u32             count;
    u32             stride;
} VertexBuffer;

CENGINE_FUN VertexBuffer CreateVertexBuffer(Engine *engine, void *vertices, u32 count, u32 stride);
CENGINE_FUN void         DestroyVertexBuffer(VertexBuffer *buffer);
CENGINE_FUN void         SetVertexBuffer(Engine *engine, VertexBuffer *buffer);
CENGINE_FUN void         DrawVertices(Engine *engine, VertexBuffer *buffer);

//
// IndexBuffer
//

typedef struct IndexBuffer
{
    ID3D12Resource *res;
    u32             count;
} IndexBuffer;

CENGINE_FUN IndexBuffer CreateIndexBuffer(Engine *engine, u32 *indices, u32 count);
CENGINE_FUN void        DestroyIndexBuffer(IndexBuffer *buffer);
CENGINE_FUN void        SetIndexBuffer(Engine *engine, IndexBuffer *buffer);
CENGINE_FUN void        DrawIndices(Engine *engine, IndexBuffer *buffer);

//
// GraphicsProgram
//

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
    const char *target;
    const char *name;
    const char *entry_point;
    u64         entry_point_len;
    // @TODO(Roman): target?
    const char *code_start;
    const char *code_end;
} ShaderDesc;

typedef struct GraphicsProgramDesc
{
    struct
    {
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

typedef struct GraphicsProgram
{
    ID3DBlob            *signature;
    ID3D12RootSignature *root_signature;

    ID3D12PipelineState *pipeline_state;

    u32                  shaders_count;
    Shader               shaders[MAX_GRAPHICS_SHADERS];
} GraphicsProgram;

CENGINE_FUN void GraphicsProgram_Create(Engine *engine, const char *file_with_shaders, D3D_SHADER_MACRO *predefines, GraphicsProgram *graphics_program);
CENGINE_FUN void GraphicsProgram_Destroy(GraphicsProgram *graphics_program);
CENGINE_FUN void GraphicsProgram_Bind(Engine *engine, GraphicsProgram *graphics_program);
CENGINE_FUN void GraphicsProgram_SetConstants(Engine *engine, GraphicsProgram *graphics_program, void *constants, u32 slot_index, u32 count);
CENGINE_FUN void GraphicsProgram_SetBuffer(Engine *engine, GraphicsProgram *graphics_program, ID3D12Resource *buffers, u32 slot_index);
CENGINE_FUN void GraphicsProgram_SetTables(Engine *engine, GraphicsProgram *graphics_program, ID3D12DescriptorHeap **heap_desc, u32 *slot_indices, u32 *desc_sizes, u32 count);
