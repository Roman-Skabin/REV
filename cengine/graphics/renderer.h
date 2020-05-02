//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "math/vec.h"

#ifndef ENGINE_DEFINED
#define ENGINE_DEFINED
    typedef struct Engine Engine;
#endif

enum
{
    BUFFERS_COUNT = 2,
};

typedef struct Renderer
{
#if DEBUG
    ID3D12Debug                     *debug;
#endif
    IDXGIFactory2                   *factory;
    IDXGIAdapter1                   *adapter;
    ID3D12Device                    *device;

    ID3D12CommandQueue              *queue;
    ID3D12CommandAllocator          *graphics_allocators[BUFFERS_COUNT];
    ID3D12GraphicsCommandList       *graphics_lists[BUFFERS_COUNT];
    IDXGISwapChain4                 *swap_chain;

    ID3D12CommandAllocator          *compute_allocators[BUFFERS_COUNT];
    ID3D12GraphicsCommandList       *compute_lists[BUFFERS_COUNT];

    ID3D12DescriptorHeap            *rtv_heap_desc;
    ID3D12Resource                  *rt_buffers[BUFFERS_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE      rtv_cpu_desc_handle;
    u32                              rtv_desc_size;
    u32                              current_buffer;

    ID3D12DescriptorHeap            *ds_heap_desc;
    ID3D12Resource                  *ds_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE      dsv_cpu_desc_handle;

    ID3D12Fence                     *fences[BUFFERS_COUNT];
    u64                              fences_values[BUFFERS_COUNT];
    HANDLE                           fence_event;

    b32                              vsync;
    b32                              first_frame;
    b32                              tearing_supported;

    HRESULT error;
} Renderer;

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

// @CleanUp(Roman): maybe it's useless
typedef enum SHADER_KIND
{
    SHADER_KIND_VERTEX,
    SHADER_KIND_HULL,
    SHADER_KIND_DOMAIN,
    SHADER_KIND_GEOMETRY,
    SHADER_KIND_PIXEL,
    SHADER_KIND_COMPUTE,
} SHADER_KIND;

typedef struct ShaderPipelineArg
{
    const char  *semantic_name;
    u32          semantic_index;
    DXGI_FORMAT  format;
} ShaderPipelineArg;

typedef struct Shader
{
    const char *target;
    char       *name;
    const char *entry_point;

    D3D_SHADER_MACRO *defines;
    ID3DInclude      *include;

    ID3DBlob              *blob;
    D3D12_SHADER_BYTECODE  bytecode;
} Shader;

typedef struct GraphicsProgram
{
    ID3DBlob            *grahpics_signature;
    ID3D12RootSignature *graphics_root_signature;

    ID3D12PipelineState *graphics_pipeline_state;

    ShaderPipelineArg *args;
    u32                args_count;

    u32    shaders_count;
    Shader shaders[5];
} GraphicsProgram;

CENGINE_FUN void GraphicsProgram_Create(Engine *engine, const char *file_with_shaders, GraphicsProgram *graphics_program);
CENGINE_FUN void GraphicsProgram_Destroy(GraphicsProgram *graphics_program);
CENGINE_FUN void GraphicsProgram_Bind(Engine *engine, GraphicsProgram *graphics_program);
CENGINE_FUN void GraphicsProgram_SetConstants(Engine *engine, GraphicsProgram *graphics_program, void *constants, u32 slot_index, u32 count);
CENGINE_FUN void GraphicsProgram_SetBuffer(Engine *engine, GraphicsProgram *graphics_program, ID3D12Resource *buffers, u32 slot_index);
CENGINE_FUN void GraphicsProgram_SetTables(Engine *engine, GraphicsProgram *graphics_program, ID3D12DescriptorHeap **heap_desc, u32 *slot_indices, u32 *desc_sizes, u32 count);
