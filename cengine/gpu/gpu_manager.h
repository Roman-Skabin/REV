//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

enum GPU_MANAGER_CONSTANTS
{
    SWAP_CHAIN_BUFFERS_COUNT = 2,
};

// @TODO(Roman): Enable several adapters support

typedef struct GPUManager
{
    // @TODO(Roman): Enable entire DEBUG stuff for GPU.
#if DEBUG
    ID3D12Debug                 *debug;
    IDXGIDebug1                 *dxgi_debug;
    ID3D12DebugCommandQueue     *debug_queue; // [SWAP_CHAIN_BUFFERS_COUNT];
    ID3D12DebugCommandList      *debug_list;  // [SWAP_CHAIN_BUFFERS_COUNT];
#endif
    IDXGIFactory2               *factory;
    IDXGIAdapter1               *adapter;
    ID3D12Device                *device;

    ID3D12CommandQueue          *graphics_queue;
    ID3D12CommandAllocator      *graphics_allocators[SWAP_CHAIN_BUFFERS_COUNT];
    ID3D12GraphicsCommandList   *graphics_lists[SWAP_CHAIN_BUFFERS_COUNT];
    IDXGISwapChain4             *swap_chain;

    ID3D12CommandQueue          *compute_queue;
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

    ID3D12Fence                 *graphics_fences[SWAP_CHAIN_BUFFERS_COUNT];
    u64                          graphics_fences_values[SWAP_CHAIN_BUFFERS_COUNT];
    HANDLE                       graphics_fence_event;

    ID3D12Fence                 *compute_fences[SWAP_CHAIN_BUFFERS_COUNT];
    u64                          compute_fences_values[SWAP_CHAIN_BUFFERS_COUNT];
    HANDLE                       compute_fence_event;

    b32                          vsync;
    b32                          first_frame;
    b32                          tearing_supported;

    HRESULT                      error;
} GPUManager;

CENGINE_FUN void SetVSync(Engine *engine, b32 enable);
