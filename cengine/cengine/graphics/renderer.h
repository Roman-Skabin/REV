//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "math/mat.h"

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
    ID3D12Debug                 *debug;
#endif
    IDXGIFactory2               *factory;
    IDXGIAdapter1               *adapter;
    ID3D12Device                *device;
    ID3D12CommandQueue          *queue;
    ID3D12CommandAllocator      *allocators[BUFFERS_COUNT];
    ID3D12GraphicsCommandList   *graphcis_lists[BUFFERS_COUNT];
    IDXGISwapChain4             *swap_chain;

    ID3D12DescriptorHeap        *rtv_heap_desc;
    ID3D12Resource              *rt_buffers[BUFFERS_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE  rtv_cpu_desc_handle;
    u32                          rtv_desc_size;
    u32                          current_buffer;

    ID3D12DescriptorHeap        *ds_heap_desc;
    ID3D12Resource              *ds_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE  dsv_cpu_desc_handle;

    ID3D12Fence                 *fences[BUFFERS_COUNT];
    u64                          fences_value[BUFFERS_COUNT];
    HANDLE                       fence_event;

    HRESULT error;
    b32     vsync;
} Renderer;

typedef struct Triangle
{
    v4 vertices[3];
    m4 model;
    m4 view;
} Triangle;

typedef struct Rect
{
    v4 vertices[4];
    m4 model;
    m4 view;
} Rect;

typedef struct Mesh
{
    v4  *vertices;
    u64  count;
    m4   model;
    m4   view;
} Mesh;

CEXTERN void SetVSync(Engine *engine, b32 enable);

CEXTERN void DrawOpaqueTriangles(Engine *engine, Triangle **triangles, u64 count);
CEXTERN void DrawTranslucentTriangles(Engine *engine, Triangle **triangles, u64 count);

CEXTERN void DrawOpaqueRects(Engine *engine, Rect **rects, u64 count);
CEXTERN void DrawTranslucentRects(Engine *engine, Rect **rects, u64 count);

CEXTERN void DrawOpaqueMeshes(Engine *engine, Mesh **meshes, u64 count);
CEXTERN void DrawTranslucentMeshes(Engine *engine, Mesh **meshes, u64 count);
