//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/logger.h"

// @TODO(Roman): Various number of back buffers?
enum GPU_MANAGER_CONSTANTS
{
    SWAP_CHAIN_BUFFERS_COUNT = 2,
};

// @TODO(Roman): Enable multiple adapters support
struct GPUManager final
{
    Logger                       logger;

#if DEBUG
    ID3D12Debug1                *debug;
    IDXGIDebug1                 *dxgi_debug;
    ID3D12InfoQueue             *info_queue;
    IDXGIInfoQueue              *dxgi_info_queue;
#endif

    IDXGIFactory2               *factory;
    IDXGIAdapter1               *adapter;
    ID3D12Device                *device;

    ID3D12CommandQueue          *graphics_queue;
    ID3D12CommandAllocator      *graphics_allocators[SWAP_CHAIN_BUFFERS_COUNT];
    ID3D12GraphicsCommandList   *graphics_lists[SWAP_CHAIN_BUFFERS_COUNT];
#if DEBUG
    ID3D12DebugCommandList      *debug_graphics_lists[SWAP_CHAIN_BUFFERS_COUNT];
#endif

    IDXGISwapChain4             *swap_chain;

    ID3D12DescriptorHeap        *rtv_heap_desc;
    ID3D12Resource              *rt_buffers[SWAP_CHAIN_BUFFERS_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE  rtv_cpu_desc_handle;
    u32                          rtv_desc_size;
    u32                          current_buffer;

    ID3D12DescriptorHeap        *dsv_heap_desc;
    ID3D12Resource              *ds_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE  dsv_cpu_desc_handle;

    ID3D12Fence                 *fences[SWAP_CHAIN_BUFFERS_COUNT];
    u64                          fences_values[SWAP_CHAIN_BUFFERS_COUNT];
    HANDLE                       fence_event;

    b32                          vsync;
    b32                          first_frame;
    b32                          tearing_supported;

    struct
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS               options;
        D3D12_FEATURE_DATA_D3D12_OPTIONS1              options1;
        D3D12_FEATURE_DATA_D3D12_OPTIONS2              options2;
        D3D12_FEATURE_DATA_D3D12_OPTIONS3              options3;
        D3D12_FEATURE_DATA_D3D12_OPTIONS4              options4;
        D3D12_FEATURE_DATA_D3D12_OPTIONS5              options5;
        D3D12_FEATURE_DATA_D3D12_OPTIONS6              options6;
        D3D12_FEATURE_DATA_ARCHITECTURE1               architecture;
        D3D12_FEATURE_DATA_ROOT_SIGNATURE              root_signature;
        D3D12_FEATURE_DATA_SHADER_MODEL                shader_model;
        D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT virtual_address;
    } features;

    HRESULT                      error;
};

ENGINE_FUN void SetVSync(in Engine *engine, in b32 enable);

#if DEBUG
ENGINE_FUN void LogDirectXMessages(in Engine *engine);
#endif
