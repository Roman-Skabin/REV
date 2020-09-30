//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/pch.h"
#include "platform/d3d12/d3d12_gpu_manager.h"

D3D12GPUManager::D3D12GPUManager(Window *window, const Logger& logger)
    : m_Logger(logger, "GPU Manager Logger", Logger::TARGET::FILE | Logger::TARGET::CONSOLE),
      m_Window(window),
      m_Flags(FLAGS::FIRST_FRAME),
      m_Error(S_OK)
{
    CreateDebugLayer();
    CreateFactory();
    CreateAdapterAndDevice();
    GetInfoAboutFeatures();
    CreateGraphicsQueueAllocatorsAndLists();
    CreateSwapChain();
    GetSwapChainRenderTargets();
    CreateDepthBuffer();
    CreateFences();
    
    m_Logger.LogSuccess("GPUManager has been created");
}

D3D12GPUManager::D3D12GPUManager(D3D12GPUManager&& other) noexcept
    : m_Logger(RTTI::move(other.m_Logger)),
      m_Window(other.m_Window),
#if DEBUG
      m_Debug(other.m_Debug),
      m_DXGIDebug(other.m_DXGIDebug),
      m_InfoQueue(other.m_InfoQueue),
#endif
      m_Factory(other.m_Factory),
      m_Adapter(other.m_Adapter),
      m_Device(other.m_Device),
      m_GraphicsQueue(other.m_GraphicsQueue),
      m_SwapChain(other.m_SwapChain),
      m_RTVHeapDesc(other.m_RTVHeapDesc),
      m_RTVCPUDescHandle(other.m_RTVCPUDescHandle),
      m_RTVDescSize(other.m_RTVDescSize),
      m_CurrentBuffer(other.m_CurrentBuffer),
      m_DSVHeapDesc(other.m_DSVHeapDesc),
      m_DSBuffer(other.m_DSBuffer),
      m_DSVCPUDescHandle(other.m_DSVCPUDescHandle),
      m_FenceEvent(other.m_FenceEvent),
      m_Flags(other.m_Flags),
      m_Features(other.m_Features),
      m_Error(other.m_Error)
{
    other.m_Window        = null;
#if DEBUG
    other.m_Debug         = null;
    other.m_DXGIDebug     = null;
    other.m_InfoQueue     = null;
#endif
    other.m_Factory       = null;
    other.m_Adapter       = null;
    other.m_Device        = null;
    other.m_GraphicsQueue = null;
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        m_GraphicsAllocators[i] = other.m_GraphicsAllocators[i];
        m_GraphicsLists[i]      = other.m_GraphicsLists[i];
#if DEBUG
        m_DebugGraphicsLists[i] = other.m_DebugGraphicsLists[i];
#endif
        m_RTBuffers[i]          = other.m_RTBuffers[i];
        m_Fences[i]             = other.m_Fences[i];
        m_FencesValues[i]       = other.m_FencesValues[i];

        other.m_GraphicsAllocators[i] = null;
        other.m_GraphicsLists[i]      = null;
#if DEBUG
        other.m_DebugGraphicsLists[i] = null;
#endif
        other.m_RTBuffers[i]          = null;
        other.m_Fences[i]             = null;
    }
    other.m_SwapChain   = null;
    other.m_RTVHeapDesc = null;
    other.m_DSVHeapDesc = null;
    other.m_DSBuffer    = null;
    other.m_FenceEvent  = null;
}

D3D12GPUManager::~D3D12GPUManager()
{
    Destroy();
}

void D3D12GPUManager::Destroy()
{
    if (m_FenceEvent) DebugResult(CloseHandle(m_FenceEvent));

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(m_Fences[i]);
        SafeRelease(m_RTBuffers[i]);
    #if DEBUG
        SafeRelease(m_DebugGraphicsLists[i]);
    #endif
        SafeRelease(m_GraphicsLists[i]);
        SafeRelease(m_GraphicsAllocators[i]);
    }

    SafeRelease(m_DSBuffer);
    SafeRelease(m_DSVHeapDesc);
    SafeRelease(m_RTVHeapDesc);
    SafeRelease(m_SwapChain);
    SafeRelease(m_GraphicsQueue);
    SafeRelease(m_Device);
    SafeRelease(m_Adapter);
    SafeRelease(m_Factory);

#if DEBUG
    if (m_DXGIDebug)
    {
        m_Error = m_DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        Check(SUCCEEDED(m_Error));

        // @TODO(Roman): ...
        // LogDirectXMessages(engine);

        m_DXGIDebug->DisableLeakTrackingForThread();
    }

    SafeRelease(m_InfoQueue);
    SafeRelease(m_DXGIDebug);
    SafeRelease(m_Debug);
#endif

    m_Logger.LogInfo("GPUManager has been destroyed");
}

void D3D12GPUManager::WaitForGPU()
{
    ID3D12Fence *current_fence       = m_Fences[m_CurrentBuffer];
    u64          current_fence_value = m_FencesValues[m_CurrentBuffer];

    m_GraphicsQueue->Signal(current_fence, current_fence_value);

    u64 value = current_fence->GetCompletedValue();
    if (value < current_fence_value)
    {
        m_Error = current_fence->SetEventOnCompletion(current_fence_value, m_FenceEvent);
        Check(SUCCEEDED(m_Error));

        while (WaitForSingleObjectEx(m_FenceEvent, INFINITE, false) != WAIT_OBJECT_0)
        {
        }
    }

    m_FencesValues[m_CurrentBuffer] = current_fence_value + 1;
}

void D3D12GPUManager::FlushGPU()
{
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12Fence *fence       = m_Fences[i];
        u64          fence_value = m_FencesValues[i];

        m_Error = m_GraphicsQueue->Signal(fence, fence_value);
        Check(SUCCEEDED(m_Error));

        m_Error = fence->SetEventOnCompletion(fence_value, m_FenceEvent);
        Check(SUCCEEDED(m_Error));

        while (WaitForSingleObjectEx(m_FenceEvent, INFINITE, false) != WAIT_OBJECT_0)
        {
        }

        ++m_FencesValues[i];
    }
}

void D3D12GPUManager::ResizeBuffers()
{
    // Wait for the GPU, release swap chain buffers buffers
    FlushGPU();
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(m_RTBuffers[i]);
    }
    SafeRelease(m_DSBuffer);

    // Resize buffers
    u32 flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if ((m_Flags & FLAGS::TEARING_SUPPORTED) != FLAGS::NONE)
    {
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    UINT      node_masks[]     = { 0, 0 };
    IUnknown *present_queues[] = { m_GraphicsQueue, m_GraphicsQueue };

    m_Error = m_SwapChain->ResizeBuffers1(SWAP_CHAIN_BUFFERS_COUNT,
                                          m_Window->Size().w,
                                          m_Window->Size().h,
                                          DXGI_FORMAT_R8G8B8A8_UNORM,
                                          flags,
                                          node_masks,
                                          present_queues);
    Check(SUCCEEDED(m_Error));

    // Recreate RT buffer & RTV
    m_RTVCPUDescHandle = m_RTVHeapDesc->GetCPUDescriptorHandleForHeapStart();

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12Resource **rt_buffer = m_RTBuffers + i;
        m_Error = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(rt_buffer));
        Check(SUCCEEDED(m_Error));

        m_Device->CreateRenderTargetView(m_RTBuffers[i], 0, m_RTVCPUDescHandle);

        m_RTVCPUDescHandle.ptr += m_RTVDescSize;
    }

    m_RTVCPUDescHandle.ptr -= SWAP_CHAIN_BUFFERS_COUNT * m_RTVDescSize;
    m_CurrentBuffer         = m_SwapChain->GetCurrentBackBufferIndex();
    m_RTVCPUDescHandle.ptr += m_CurrentBuffer * m_RTVDescSize;

    // Recreate DS buffer & DSV
    m_DSVCPUDescHandle = m_DSVHeapDesc->GetCPUDescriptorHandleForHeapStart();

    D3D12_HEAP_PROPERTIES ds_heap_properties;
    ds_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    ds_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    ds_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    ds_heap_properties.CreationNodeMask     = 0;
    ds_heap_properties.VisibleNodeMask      = 0;

    D3D12_RESOURCE_DESC ds_resource_desc;
    ds_resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ds_resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ds_resource_desc.Width              = m_Window->Size().w;
    ds_resource_desc.Height             = m_Window->Size().h;
    ds_resource_desc.DepthOrArraySize   = 1;
    ds_resource_desc.MipLevels          = 1;
    ds_resource_desc.Format             = DXGI_FORMAT_D32_FLOAT;
    ds_resource_desc.SampleDesc.Count   = 1;
    ds_resource_desc.SampleDesc.Quality = 0;
    ds_resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ds_resource_desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE ds_clear_value;
    ds_clear_value.Format               = DXGI_FORMAT_D32_FLOAT;
    ds_clear_value.DepthStencil.Depth   = 1.0f;
    ds_clear_value.DepthStencil.Stencil = 0;

    m_Error = m_Device->CreateCommittedResource(&ds_heap_properties,
                                                D3D12_HEAP_FLAG_SHARED,
                                                &ds_resource_desc,
                                                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                &ds_clear_value,
                                                IID_PPV_ARGS(&m_DSBuffer));
    Check(SUCCEEDED(m_Error));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    dsv_desc.Format             = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags              = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    dsv_desc.Texture2D.MipSlice = 1;

    m_Device->CreateDepthStencilView(m_DSBuffer, &dsv_desc, m_DSVCPUDescHandle);

    m_Flags |= FLAGS::FIRST_FRAME;
}

void D3D12GPUManager::ResizeTarget()
{
    DXGI_MODE_DESC mode_desc;
    mode_desc.Width                   = m_Window->Size().w;
    mode_desc.Height                  = m_Window->Size().h;
    mode_desc.RefreshRate.Numerator   = 0;
    mode_desc.RefreshRate.Denominator = 1;
    mode_desc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    mode_desc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    mode_desc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;

    m_Error = m_SwapChain->ResizeTarget(&mode_desc);
    Check(SUCCEEDED(m_Error));
}

void D3D12GPUManager::StartFrame()
{
    ID3D12CommandAllocator    *graphics_allocator = m_GraphicsAllocators[m_CurrentBuffer];
    ID3D12GraphicsCommandList *graphics_list      = m_GraphicsLists[m_CurrentBuffer];

    WaitForGPU();

    m_Error = graphics_allocator->Reset();
    Check(SUCCEEDED(m_Error));
    m_Error = graphics_list->Reset(graphics_allocator, null);
    Check(SUCCEEDED(m_Error));

    // Set viewport
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width    = cast<f32>(m_Window->Size().w);
    viewport.Height   = cast<f32>(m_Window->Size().h);
    viewport.MinDepth = -1.0f;
    viewport.MaxDepth =  1.0f;

    graphics_list->RSSetViewports(1, &viewport);

    // Set scissor rect (optimization)
    D3D12_RECT scissor_rect;
    scissor_rect.left   = 0;
    scissor_rect.top    = 0;
    scissor_rect.right  = m_Window->Size().w;
    scissor_rect.bottom = m_Window->Size().h;

    graphics_list->RSSetScissorRects(1, &scissor_rect);

    // Set RTV And DSV
    graphics_list->OMSetRenderTargets(1,
                                      &m_RTVCPUDescHandle,
                                      false,
                                      &m_DSVCPUDescHandle);

    // Set Resource Barriers
    D3D12_RESOURCE_BARRIER rt_barrier;
    rt_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    rt_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    rt_barrier.Transition.pResource   = m_RTBuffers[m_CurrentBuffer];
    rt_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rt_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    rt_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

    if ((m_Flags & FLAGS::FIRST_FRAME) != FLAGS::NONE)
    {
        graphics_list->ResourceBarrier(1, &rt_barrier);
    }
    else
    {
        D3D12_RESOURCE_BARRIER ds_barrier;
        ds_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        ds_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        ds_barrier.Transition.pResource   = m_DSBuffer;
        ds_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        ds_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        ds_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;

        D3D12_RESOURCE_BARRIER begin_barriers[] = { rt_barrier, ds_barrier };
        graphics_list->ResourceBarrier(cast<UINT>(ArrayCount(begin_barriers)), begin_barriers);
    }

    // Clear
    f32 clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    graphics_list->ClearRenderTargetView(m_RTVCPUDescHandle,
                                         clear_color,
                                         0, null);
    graphics_list->ClearDepthStencilView(m_DSVCPUDescHandle,
                                         D3D12_CLEAR_FLAG_DEPTH,
                                         1.0f, 0,
                                         0, null);
}

void D3D12GPUManager::EndFrame()
{
    ID3D12GraphicsCommandList *graphics_list = m_GraphicsLists[m_CurrentBuffer];

    // Set Resource Barriers
    D3D12_RESOURCE_BARRIER rt_barrier;
    rt_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    rt_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    rt_barrier.Transition.pResource   = m_RTBuffers[m_CurrentBuffer];
    rt_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rt_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    rt_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

    D3D12_RESOURCE_BARRIER ds_barrier;
    ds_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ds_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    ds_barrier.Transition.pResource   = m_DSBuffer;
    ds_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ds_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    ds_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    D3D12_RESOURCE_BARRIER end_barriers[] = { rt_barrier, ds_barrier };
    graphics_list->ResourceBarrier(cast<UINT>(ArrayCount(end_barriers)), end_barriers);

    // Close and execute lists
    m_Error = graphics_list->Close();
#if DEBUG
    // @TODO(Roman): ...
    // LogDirectXMessages();
#endif
    Check(SUCCEEDED(m_Error));

    ID3D12CommandList *command_lists[] =
    {
        graphics_list,
    };
    m_GraphicsQueue->ExecuteCommandLists(cast<UINT>(ArrayCount(command_lists)), command_lists);
    Check(SUCCEEDED(m_Error));

#if DEBUG
    // @TODO(Roman): ...
    // LogDirectXMessages(engine);
#endif

    // Present
    u32 vsync         = 1;
    u32 present_flags = 0;

    if ((m_Flags & FLAGS::VSYNC_ENABLED) == FLAGS::NONE)
    {
        vsync          = 0;
        present_flags |= DXGI_PRESENT_DO_NOT_WAIT;

        if ((m_Flags & FLAGS::TEARING_SUPPORTED) != FLAGS::NONE)
        {
            present_flags |= DXGI_PRESENT_ALLOW_TEARING;
        }
    }
    if ((m_Flags & FLAGS::FIRST_FRAME) != FLAGS::NONE)
    {
        m_Flags &= ~FLAGS::FIRST_FRAME;
    }
    else
    {
        present_flags |= DXGI_PRESENT_DO_NOT_SEQUENCE;
    }

    m_Error = m_SwapChain->Present(vsync, present_flags);
    Check(SUCCEEDED(m_Error));

#if DEBUG
    // @TODO(Roman): ...
    // LogDirectXMessages(engine);
#endif

    // Swap buffers
    m_CurrentBuffer = m_SwapChain->GetCurrentBackBufferIndex();

    m_RTVCPUDescHandle      = m_RTVHeapDesc->GetCPUDescriptorHandleForHeapStart();
    m_RTVCPUDescHandle.ptr += m_CurrentBuffer * m_RTVDescSize;
}

D3D12GPUManager& D3D12GPUManager::operator=(D3D12GPUManager&& other) noexcept
{
    if (this != &other)
    {
        *cast<u64 *>(this)   = *cast<u64 *>(&other);
        *cast<u64 *>(&other) = 0;

        m_Logger           = RTTI::move(other.m_Logger);
        m_Window           = other.m_Window;
    #if DEBUG
        m_Debug            = other.m_Debug;
        m_DXGIDebug        = other.m_DXGIDebug;
        m_InfoQueue        = other.m_InfoQueue;
    #endif
        m_Factory          = other.m_Factory;
        m_Adapter          = other.m_Adapter;
        m_Device           = other.m_Device;
        m_GraphicsQueue    = other.m_GraphicsQueue;
        m_SwapChain        = other.m_SwapChain;
        m_RTVHeapDesc      = other.m_RTVHeapDesc;
        m_RTVCPUDescHandle = other.m_RTVCPUDescHandle;
        m_RTVDescSize      = other.m_RTVDescSize;
        m_CurrentBuffer    = other.m_CurrentBuffer;
        m_DSVHeapDesc      = other.m_DSVHeapDesc;
        m_DSBuffer         = other.m_DSBuffer;
        m_DSVCPUDescHandle = other.m_DSVCPUDescHandle;
        m_FenceEvent       = other.m_FenceEvent;
        m_Flags            = other.m_Flags;
        m_Features         = other.m_Features;
        m_Error            = other.m_Error;

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            m_GraphicsAllocators[i] = other.m_GraphicsAllocators[i];
            m_GraphicsLists[i]      = other.m_GraphicsLists[i];
    #if DEBUG
            m_DebugGraphicsLists[i] = other.m_DebugGraphicsLists[i];
    #endif
            m_RTBuffers[i]          = other.m_RTBuffers[i];
            m_Fences[i]             = other.m_Fences[i];
            m_FencesValues[i]       = other.m_FencesValues[i];

            other.m_GraphicsAllocators[i] = null;
            other.m_GraphicsLists[i]      = null;
    #if DEBUG
            other.m_DebugGraphicsLists[i] = null;
    #endif
            other.m_RTBuffers[i]          = null;
            other.m_Fences[i]             = null;
        }

        other.m_Window        = null;
    #if DEBUG
        other.m_Debug         = null;
        other.m_DXGIDebug     = null;
        other.m_InfoQueue     = null;
    #endif
        other.m_Factory       = null;
        other.m_Adapter       = null;
        other.m_Device        = null;
        other.m_GraphicsQueue = null;
        
        other.m_SwapChain     = null;
        other.m_RTVHeapDesc   = null;
        other.m_DSVHeapDesc   = null;
        other.m_DSBuffer      = null;
        other.m_FenceEvent    = null;
    }
    return *this;
}

void D3D12GPUManager::CreateDebugLayer()
{
#if DEBUG
    m_Error = D3D12GetDebugInterface(IID_PPV_ARGS(&m_Debug));
    Check(SUCCEEDED(m_Error));

    m_Debug->EnableDebugLayer();
    // m_Debug->SetEnableGPUBasedValidation(true);
    m_Debug->SetEnableSynchronizedCommandQueueValidation(true);

    m_Error = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_DXGIDebug));
    Check(SUCCEEDED(m_Error));

    m_DXGIDebug->EnableLeakTrackingForThread();

    m_Error = m_DXGIDebug->QueryInterface(&m_InfoQueue);
    Check(SUCCEEDED(m_Error));
#endif
}

void D3D12GPUManager::CreateFactory()
{
    UINT factory_create_flags = 0;
#if DEBUG
    factory_create_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    m_Error = CreateDXGIFactory2(factory_create_flags, IID_PPV_ARGS(&m_Factory));
    Check(SUCCEEDED(m_Error));
}

void D3D12GPUManager::CreateAdapterAndDevice()
{
    SIZE_T             max_vram      = 0;
    UINT               adapter_index = 0;
    DXGI_ADAPTER_DESC1 adapter_desc  = {0};

    for (UINT i = 0; m_Factory->EnumAdapters1(i, &m_Adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        m_Error = m_Adapter->GetDesc1(&adapter_desc);
        Check(SUCCEEDED(m_Error));

        m_Error = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device));
        if (SUCCEEDED(m_Error) && max_vram < adapter_desc.DedicatedVideoMemory && !(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            max_vram      = adapter_desc.DedicatedVideoMemory;
            adapter_index = i;
            SafeRelease(m_Device);
        }
        SafeRelease(m_Adapter);
    }

    if (max_vram)
    {
        m_Error = m_Factory->EnumAdapters1(adapter_index, &m_Adapter);
        Check(SUCCEEDED(m_Error));

        m_Error = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device));
        Check(SUCCEEDED(m_Error));

        m_Error = m_Adapter->GetDesc1(&adapter_desc);
        Check(SUCCEEDED(m_Error));

        m_Logger.LogInfo("GPU Adapter: %S, Feature Level: %s", adapter_desc.Description, CSTR(D3D_FEATURE_LEVEL_12_1));
    }
    else
    {
        for (UINT i = 0; m_Factory->EnumAdapters1(i, &m_Adapter) != D3D12_ERROR_ADAPTER_NOT_FOUND; ++i)
        {
            m_Error = m_Adapter->GetDesc1(&adapter_desc);
            Check(SUCCEEDED(m_Error));

            m_Error = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));
            if (SUCCEEDED(m_Error) && max_vram < adapter_desc.DedicatedVideoMemory && !(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
            {
                max_vram      = adapter_desc.DedicatedVideoMemory;
                adapter_index = i;
                SafeRelease(m_Device);
            }
            SafeRelease(m_Adapter);
        }

        CheckM(max_vram, "Direct3D 12 is not supported by your hardware");

        m_Error = m_Factory->EnumAdapters1(adapter_index, &m_Adapter);
        Check(SUCCEEDED(m_Error));

        m_Error = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));
        Check(SUCCEEDED(m_Error));

        m_Error = m_Adapter->GetDesc1(&adapter_desc);
        Check(SUCCEEDED(m_Error));

        m_Logger.LogInfo("GPU Adapter: %S, Feature Level: %s", adapter_desc.Description, CSTR(D3D_FEATURE_LEVEL_12_0));
    }
}

void D3D12GPUManager::GetInfoAboutFeatures()
{
    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &m_Features.options, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
    Check(SUCCEEDED(m_Error));

    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &m_Features.options1, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS1));
    Check(SUCCEEDED(m_Error));

    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &m_Features.options2, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS2));
    Check(SUCCEEDED(m_Error));

    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &m_Features.options3, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3));
    Check(SUCCEEDED(m_Error));

    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &m_Features.options4, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS4));
    Check(SUCCEEDED(m_Error));

    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &m_Features.options5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
    Check(SUCCEEDED(m_Error));

    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &m_Features.options6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
    Check(SUCCEEDED(m_Error));

    m_Features.architecture.NodeIndex = 0;
    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &m_Features.architecture, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE1));
    Check(SUCCEEDED(m_Error));

    m_Features.root_signature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &m_Features.root_signature, sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE));
    Check(SUCCEEDED(m_Error));

    m_Features.shader_model.HighestShaderModel = D3D_SHADER_MODEL_6_5;
    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &m_Features.shader_model, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL));
    Check(SUCCEEDED(m_Error));

    m_Error = m_Device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &m_Features.virtual_address, sizeof(D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT));
    Check(SUCCEEDED(m_Error));
}

void D3D12GPUManager::CreateGraphicsQueueAllocatorsAndLists()
{
    D3D12_COMMAND_QUEUE_DESC graphics_queue_desc;
    graphics_queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    graphics_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME ?
    graphics_queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    graphics_queue_desc.NodeMask = 0;

    m_Error = m_Device->CreateCommandQueue(&graphics_queue_desc, IID_PPV_ARGS(&m_GraphicsQueue));
    Check(SUCCEEDED(m_Error));
    
    m_Error = m_GraphicsQueue->SetPrivateData(WKPDID_D3DDebugObjectName, CSTRLEN("Graphics Queue"), "Graphics Queue");
    Check(SUCCEEDED(m_Error));

    char name[64];
    s32  name_len = 0;

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12CommandAllocator    **graphics_allocator = m_GraphicsAllocators + i;
        ID3D12GraphicsCommandList **graphics_list      = m_GraphicsLists      + i;

        m_Error = m_Device->CreateCommandAllocator(graphics_queue_desc.Type, IID_PPV_ARGS(graphics_allocator));
        Check(SUCCEEDED(m_Error));

        name_len = sprintf(name, "Graphics Allocator #%I32u", i);
        m_Error = (*graphics_allocator)->SetPrivateData(WKPDID_D3DDebugObjectName, name_len, name);
        Check(SUCCEEDED(m_Error));

        m_Error = m_Device->CreateCommandList(graphics_queue_desc.NodeMask,
                                              graphics_queue_desc.Type,
                                              *graphics_allocator,
                                              null,
                                              IID_PPV_ARGS(graphics_list));
        Check(SUCCEEDED(m_Error));

        name_len = sprintf(name, "Graphics List #%I32u", i);
        m_Error = (*graphics_list)->SetPrivateData(WKPDID_D3DDebugObjectName, name_len, name);
        Check(SUCCEEDED(m_Error));

        m_Error = (*graphics_list)->Close();
        Check(SUCCEEDED(m_Error));

    #if DEBUG
        m_Error = (*graphics_list)->QueryInterface(m_DebugGraphicsLists + i);
        Check(SUCCEEDED(m_Error));
    #endif
    }
}

void D3D12GPUManager::CreateSwapChain()
{
    IDXGIFactory5 *factory5 = null;
    m_Error = m_Factory->QueryInterface(&factory5);
    Check(SUCCEEDED(m_Error));

    b32 tearing_supported = false;
    m_Error = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing_supported, sizeof(b32));
    Check(SUCCEEDED(m_Error));

    SafeRelease(factory5);

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1;
    swap_chain_desc1.Width              = m_Window->Size().w;
    swap_chain_desc1.Height             = m_Window->Size().h;
    swap_chain_desc1.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc1.Stereo             = false;
    swap_chain_desc1.SampleDesc.Count   = 1;
    swap_chain_desc1.SampleDesc.Quality = 0;
    swap_chain_desc1.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc1.BufferCount        = SWAP_CHAIN_BUFFERS_COUNT;
    swap_chain_desc1.Scaling            = DXGI_SCALING_STRETCH;
    swap_chain_desc1.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc1.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc1.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if (tearing_supported)
    {
        m_Flags                |= FLAGS::TEARING_SUPPORTED;
        swap_chain_desc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_desc;
    swap_chain_fullscreen_desc.RefreshRate.Numerator   = 0;
    swap_chain_fullscreen_desc.RefreshRate.Denominator = 1;
    swap_chain_fullscreen_desc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_fullscreen_desc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
    swap_chain_fullscreen_desc.Windowed                = true;

    IDXGISwapChain1 *swap_chain1 = null;
    m_Error = m_Factory->CreateSwapChainForHwnd(m_GraphicsQueue,
                                                m_Window->m_Handle,
                                                &swap_chain_desc1,
                                                &swap_chain_fullscreen_desc,
                                                null,
                                                &swap_chain1);
    Check(SUCCEEDED(m_Error));

    m_Error = swap_chain1->QueryInterface(&m_SwapChain);
    Check(SUCCEEDED(m_Error));

    SafeRelease(swap_chain1);

#if 0 // We'are already suppressing Alt+Enter in WindowProc
    m_Error = m_Factory->MakeWindowAssociation(m_Window->m_Handle, DXGI_MWA_NO_ALT_ENTER);
    Check(SUCCEEDED(m_Error));
#endif
}

void D3D12GPUManager::GetSwapChainRenderTargets()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc;
    rtv_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_desc_heap_desc.NumDescriptors = SWAP_CHAIN_BUFFERS_COUNT;
    rtv_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_desc_heap_desc.NodeMask       = 0;

    m_Error = m_Device->CreateDescriptorHeap(&rtv_desc_heap_desc, IID_PPV_ARGS(&m_RTVHeapDesc));
    Check(SUCCEEDED(m_Error));

    m_RTVDescSize      = m_Device->GetDescriptorHandleIncrementSize(rtv_desc_heap_desc.Type);
    m_RTVCPUDescHandle = m_RTVHeapDesc->GetCPUDescriptorHandleForHeapStart();

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12Resource **rt_buffer = m_RTBuffers + i;
        m_Error = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(rt_buffer));
        Check(SUCCEEDED(m_Error));

        m_Device->CreateRenderTargetView(m_RTBuffers[i], null, m_RTVCPUDescHandle);

        m_RTVCPUDescHandle.ptr += m_RTVDescSize;
    }

    m_RTVCPUDescHandle.ptr -= SWAP_CHAIN_BUFFERS_COUNT * m_RTVDescSize;
    m_CurrentBuffer         = m_SwapChain->GetCurrentBackBufferIndex();
}

void D3D12GPUManager::CreateDepthBuffer()
{
    D3D12_DESCRIPTOR_HEAP_DESC dsv_desc_heap_desc;
    dsv_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_desc_heap_desc.NumDescriptors = 1;
    dsv_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_desc_heap_desc.NodeMask       = 0;

    m_Error = m_Device->CreateDescriptorHeap(&dsv_desc_heap_desc, IID_PPV_ARGS(&m_DSVHeapDesc));
    Check(SUCCEEDED(m_Error));

    m_DSVCPUDescHandle = m_DSVHeapDesc->GetCPUDescriptorHandleForHeapStart();

    D3D12_HEAP_PROPERTIES ds_heap_properties;
    ds_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    ds_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    ds_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    ds_heap_properties.CreationNodeMask     = 0;
    ds_heap_properties.VisibleNodeMask      = 0;

    D3D12_RESOURCE_DESC ds_resource_desc;
    ds_resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ds_resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ds_resource_desc.Width              = m_Window->Size().w;
    ds_resource_desc.Height             = m_Window->Size().h;
    ds_resource_desc.DepthOrArraySize   = 1;
    ds_resource_desc.MipLevels          = 0;
    ds_resource_desc.Format             = DXGI_FORMAT_D32_FLOAT;
    ds_resource_desc.SampleDesc.Count   = 1;
    ds_resource_desc.SampleDesc.Quality = 0;
    ds_resource_desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ds_resource_desc.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE ds_clear_value;
    ds_clear_value.Format               = DXGI_FORMAT_D32_FLOAT;
    ds_clear_value.DepthStencil.Depth   = 1.0f;
    ds_clear_value.DepthStencil.Stencil = 0;

    m_Error = m_Device->CreateCommittedResource(&ds_heap_properties,
                                                D3D12_HEAP_FLAG_SHARED,
                                                &ds_resource_desc,
                                                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                &ds_clear_value,
                                                IID_PPV_ARGS(&m_DSBuffer));
    Check(SUCCEEDED(m_Error));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    dsv_desc.Format             = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags              = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    dsv_desc.Texture2D.MipSlice = 0;

    m_Device->CreateDepthStencilView(m_DSBuffer, &dsv_desc, m_DSVCPUDescHandle);
}

void D3D12GPUManager::CreateFences()
{
    for (u64 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12Fence **fence = m_Fences + i;

        m_Error = m_Device->CreateFence(m_FencesValues[i], D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(fence));
        Check(SUCCEEDED(m_Error));

        ++m_FencesValues[i];
    }

    DebugResult(m_FenceEvent = CreateEventExA(null, "Fence Event", 0, EVENT_ALL_ACCESS));
}
