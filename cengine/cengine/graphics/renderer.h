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

    ID3DBlob                        *graphics_signature;
    ID3D12RootSignature             *graphics_root_signature;

    ID3DBlob                        *compute_signature;      // Currently not supported
    ID3D12RootSignature             *compute_root_signature; // Currently not supported

    ID3D12CommandQueue              *queue;
    ID3D12CommandAllocator          *allocators[BUFFERS_COUNT];
    ID3D12GraphicsCommandList       *graphics_lists[BUFFERS_COUNT];
    IDXGISwapChain4                 *swap_chain;

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

CEXTERN void SetVSync(Engine *engine, b32 enable);

typedef struct VertexBuffer
{
    ID3D12Resource *res;
    u32             count;
    u32             stride;
} VertexBuffer;

CEXTERN VertexBuffer CreateVertexBuffer(Engine *engine, void *vertices, u32 count, u32 stride);
CEXTERN void         DestroyVertexBuffer(VertexBuffer *buffer);

typedef struct IndexBuffer
{
    ID3D12Resource *res;
    u32             count;
} IndexBuffer;

CEXTERN IndexBuffer CreateIndexBuffer(Engine *engine, u32 *indecies, u32 count);
CEXTERN void        DestroyIndexBuffer(IndexBuffer *buffer);

typedef enum SHADER_KIND
{
    SHADER_KIND_VERTEX,
    SHADER_KIND_HULL,
    SHADER_KIND_DOMAIN,
    SHADER_KIND_GEOMETRY,
    SHADER_KIND_PIXEL,
} SHADER_KIND;

typedef struct Shader
{
    ID3DBlob              *blob;
    D3D12_SHADER_BYTECODE  bytecode;
} Shader;

CEXTERN Shader CreateShader(Engine *engine, const char *filename, const char *entry_point, SHADER_KIND kind);
CEXTERN void   DestroyShader(Shader *shader);

typedef struct ShaderArg
{
    const char  *semantic_name;
    u32          semantic_index;
    DXGI_FORMAT  format;
    u32          input_slot;
} ShaderArg;

typedef struct PipelineStage
{
    ID3D12PipelineState *pipeline;
    VertexBuffer        *vertex_buffer;
    IndexBuffer         *index_buffer;
} PipelineStage;

CEXTERN void CreatePipelineStage(
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
    OUT      PipelineStage *pipeline_stage
);

CEXTERN void DestroyPipelineStage(PipelineStage *pipeline_stage);

CEXTERN void RenderPipelineStage(Engine *engine, PipelineStage *pipeline_stage);
