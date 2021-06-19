//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/pch.h"
#include "core/settings.h"
#include "platform/d3d12/d3d12_renderer.h"
#include "platform/d3d12/d3d12_common.h"

namespace REV::D3D12 {

Renderer::Renderer(Window *window, const Logger& logger)
    : m_Logger(logger, "Renderer logger", Logger::TARGET::FILE | Logger::TARGET::CONSOLE),
      m_Window(window),
      m_RTSize(Settings::Get()->render_target_wh),
      m_ActualRTSize(m_RTSize),
#if REV_DEBUG
      m_Debug(null),
      m_DXGIDebug(null),
      m_InfoQueue(null),
#endif
      m_Factory(null),
      m_Adapter(null),
      m_Device(null),
      m_GraphicsQueue(null),
      m_GraphicsAllocators{},
      m_GraphicsLists{},
      m_SwapChain(null),
      m_WaitableObject(null),
      m_RTVHeapDesc(null),
      m_RTBuffers{},
      m_RTVCPUDescHandle(),
      m_RTVDescSize(0),
      m_CurrentBuffer(0),
      m_DSVHeapDesc(null),
      m_DSBuffer(null),
      m_DSVCPUDescHandle(),
      m_Fence(null),
      m_FenceEvent(null),
      m_VsyncEnabled(false),
      m_FirstFrame(true),
      m_TearingSupported(false),
      m_Fullscreen(false),
      m_FrameStarted(false),
      m_Features{}
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
    m_Logger.LogSuccess("Renderer has been created");
}

Renderer::~Renderer()
{
    if (m_Device)
    {
        HRESULT error = S_OK;

        if (m_Fullscreen)
        {
            error = m_SwapChain->SetFullscreenState(false, null);
            REV_CHECK(CheckResultAndPrintMessages(error, this));
        }

        WaitForGPU();

        if (m_FenceEvent) REV_DEBUG_RESULT(CloseHandle(m_FenceEvent));
        SafeRelease(m_Fence);

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            SafeRelease(m_RTBuffers[i]);
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

    #if REV_DEBUG
        if (m_DXGIDebug)
        {
            error = m_DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, cast<DXGI_DEBUG_RLO_FLAGS>(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            REV_CHECK(CheckResultAndPrintMessages(error, this));

            m_DXGIDebug->DisableLeakTrackingForThread();
        }

        SafeRelease(m_InfoQueue);
        SafeRelease(m_DXGIDebug);
        SafeRelease(m_Debug);
    #endif

        m_Logger.LogInfo("Renderer has been destroyed");
    }
}

void Renderer::StartFrame()
{
    ID3D12CommandAllocator    *graphics_allocator = m_GraphicsAllocators[m_CurrentBuffer];
    ID3D12GraphicsCommandList *graphics_list      = m_GraphicsLists[m_CurrentBuffer];

    WaitForGPU();

    while (WaitForSingleObjectEx(m_WaitableObject, INFINITE, true) != WAIT_OBJECT_0)
    {
    }

    HRESULT error = graphics_allocator->Reset();
    REV_CHECK(CheckResultAndPrintMessages(error, this));
    error = graphics_list->Reset(graphics_allocator, null);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    // Set viewport
    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width    = cast<f32>(m_ActualRTSize.w);
    viewport.Height   = cast<f32>(m_ActualRTSize.h);
    viewport.MinDepth = -1.0f;
    viewport.MaxDepth =  1.0f;

    graphics_list->RSSetViewports(1, &viewport);

    // Set scissor rect (optimization)
    D3D12_RECT scissor_rect;
    scissor_rect.left   = 0;
    scissor_rect.top    = 0;
    scissor_rect.right  = m_ActualRTSize.w;
    scissor_rect.bottom = m_ActualRTSize.h;

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

    D3D12_RESOURCE_BARRIER ds_barrier;
    ds_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ds_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    ds_barrier.Transition.pResource   = m_DSBuffer;
    ds_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ds_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    ds_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    D3D12_RESOURCE_BARRIER begin_barriers[] = { rt_barrier, ds_barrier };
    graphics_list->ResourceBarrier(cast<UINT>(ArrayCount(begin_barriers)), begin_barriers);

    // Clear
    f32 clear_color[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    graphics_list->ClearRenderTargetView(m_RTVCPUDescHandle,
                                         clear_color,
                                         0, null);
    graphics_list->ClearDepthStencilView(m_DSVCPUDescHandle,
                                         D3D12_CLEAR_FLAG_DEPTH,
                                         1.0f, 0,
                                         0, null);

    m_FrameStarted = true;
}

void Renderer::EndFrame()
{
    m_FrameStarted = false;

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
    HRESULT error = graphics_list->Close();
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    ID3D12CommandList *command_lists[] =
    {
        graphics_list,
    };
    m_GraphicsQueue->ExecuteCommandLists(cast<UINT>(ArrayCount(command_lists)), command_lists);

    // Present
    u32 present_flags = DXGI_PRESENT_DO_NOT_WAIT;

    if (!m_VsyncEnabled && m_TearingSupported && !m_Window->Fullscreened())
    {
        present_flags |= DXGI_PRESENT_ALLOW_TEARING;
    }

    if (m_FirstFrame)
    {
        m_FirstFrame = false;
    }
    else if (m_VsyncEnabled)
    {
        present_flags |= DXGI_PRESENT_DO_NOT_SEQUENCE;
    }

    error = m_SwapChain->Present(0, present_flags);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    // Swap buffers
    m_CurrentBuffer = m_SwapChain->GetCurrentBackBufferIndex();

    m_RTVCPUDescHandle      = m_RTVHeapDesc->GetCPUDescriptorHandleForHeapStart();
    m_RTVCPUDescHandle.ptr += m_CurrentBuffer * m_RTVDescSize;
}

void Renderer::WaitForGPU()
{
    u64 current_fence_value = m_Fence->GetCompletedValue() + 1;

    HRESULT error = m_GraphicsQueue->Signal(m_Fence, current_fence_value);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    if (m_Fence->GetCompletedValue() < current_fence_value)
    {
        error = m_Fence->SetEventOnCompletion(current_fence_value, m_FenceEvent);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        while (WaitForSingleObjectEx(m_FenceEvent, INFINITE, true) != WAIT_OBJECT_0)
        {
        }
    }
}

void Renderer::CreateDebugLayer()
{
#if REV_DEBUG
    HRESULT error = D3D12GetDebugInterface(IID_PPV_ARGS(&m_Debug));
    REV_CHECK(Succeeded(error));

    m_Debug->EnableDebugLayer();
//  m_Debug->SetEnableGPUBasedValidation(true);
    m_Debug->SetEnableSynchronizedCommandQueueValidation(true);

    error = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&m_DXGIDebug));
    REV_CHECK(Succeeded(error));

    m_DXGIDebug->EnableLeakTrackingForThread();

    error = m_DXGIDebug->QueryInterface(&m_InfoQueue);
    REV_CHECK(CheckResultAndPrintMessages(error, this));
#endif
}

void Renderer::CreateFactory()
{
#if REV_DEBUG
    HRESULT error = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_Factory));
#else
    HRESULT error = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_Factory));
#endif
    REV_CHECK(CheckResultAndPrintMessages(error, this));
}

void Renderer::CreateAdapterAndDevice()
{
    SIZE_T             max_vram      = 0;
    UINT               adapter_index = 0;
    DXGI_ADAPTER_DESC1 adapter_desc  = {0};
    HRESULT            error         = S_OK;

    for (UINT i = 0; m_Factory->EnumAdapters1(i, &m_Adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        error = m_Adapter->GetDesc1(&adapter_desc);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        error = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device));
        if (Succeeded(error) && max_vram < adapter_desc.DedicatedVideoMemory && !(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            max_vram      = adapter_desc.DedicatedVideoMemory;
            adapter_index = i;
            SafeRelease(m_Device);
        }
        SafeRelease(m_Adapter);
    }

    if (max_vram)
    {
        error = m_Factory->EnumAdapters1(adapter_index, &m_Adapter);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        error = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_Device));
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        error = m_Adapter->GetDesc1(&adapter_desc);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        m_Logger.LogInfo("GPU Adapter: %S, Feature Level: D3D_FEATURE_LEVEL_12_1", adapter_desc.Description);
    }
    else
    {
        for (UINT i = 0; m_Factory->EnumAdapters1(i, &m_Adapter) != D3D12_ERROR_ADAPTER_NOT_FOUND; ++i)
        {
            error = m_Adapter->GetDesc1(&adapter_desc);
            REV_CHECK(CheckResultAndPrintMessages(error, this));

            error = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));
            if (Succeeded(error) && max_vram < adapter_desc.DedicatedVideoMemory && !(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
            {
                max_vram      = adapter_desc.DedicatedVideoMemory;
                adapter_index = i;
                SafeRelease(m_Device);
            }
            SafeRelease(m_Adapter);
        }

        REV_CHECK_M(max_vram, "Direct3D 12 is not supported by your hardware");

        error = m_Factory->EnumAdapters1(adapter_index, &m_Adapter);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        error = D3D12CreateDevice(m_Adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        error = m_Adapter->GetDesc1(&adapter_desc);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        m_Logger.LogInfo("GPU Adapter: %S, Feature Level: D3D_FEATURE_LEVEL_12_0", adapter_desc.Description);
    }
}

void Renderer::GetInfoAboutFeatures()
{
    HRESULT error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &m_Features.options, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &m_Features.options1, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS1));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &m_Features.options2, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS2));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &m_Features.options3, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &m_Features.options4, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS4));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &m_Features.options5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &m_Features.options6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    m_Features.architecture.NodeIndex = 0;
    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &m_Features.architecture, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE1));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    m_Features.root_signature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &m_Features.root_signature, sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    m_Features.shader_model.HighestShaderModel = D3D_SHADER_MODEL_6_5;
    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &m_Features.shader_model, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = m_Device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &m_Features.virtual_address, sizeof(D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT));
    REV_CHECK(CheckResultAndPrintMessages(error, this));
}

void Renderer::CreateGraphicsQueueAllocatorsAndLists()
{
    D3D12_COMMAND_QUEUE_DESC graphics_queue_desc;
    graphics_queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    graphics_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME ?
    graphics_queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    graphics_queue_desc.NodeMask = 0;

    HRESULT error = m_Device->CreateCommandQueue(&graphics_queue_desc, IID_PPV_ARGS(&m_GraphicsQueue));
    REV_CHECK(CheckResultAndPrintMessages(error, this));
    
    error = m_GraphicsQueue->SetPrivateData(WKPDID_D3DDebugObjectName, REV_CSTRLEN("Graphics Queue"), "Graphics Queue");
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    char name[64];
    s32  name_len = 0;

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12CommandAllocator    **graphics_allocator = m_GraphicsAllocators + i;
        ID3D12GraphicsCommandList **graphics_list      = m_GraphicsLists      + i;

        error = m_Device->CreateCommandAllocator(graphics_queue_desc.Type, IID_PPV_ARGS(graphics_allocator));
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        name_len = sprintf(name, "Graphics Allocator #%I32u", i);
        error = (*graphics_allocator)->SetPrivateData(WKPDID_D3DDebugObjectName, name_len, name);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        error = m_Device->CreateCommandList(graphics_queue_desc.NodeMask,
                                              graphics_queue_desc.Type,
                                              *graphics_allocator,
                                              null,
                                              IID_PPV_ARGS(graphics_list));
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        name_len = sprintf(name, "Graphics List #%I32u", i);
        error = (*graphics_list)->SetPrivateData(WKPDID_D3DDebugObjectName, name_len, name);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        error = (*graphics_list)->Close();
        REV_CHECK(CheckResultAndPrintMessages(error, this));
    }
}

void Renderer::CreateSwapChain()
{
    IDXGIFactory5 *factory5 = null;
    HRESULT        error    = m_Factory->QueryInterface(&factory5);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    b32 tearing_supported = false;
    error = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing_supported, sizeof(b32));
    m_TearingSupported = tearing_supported;
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    SafeRelease(factory5);

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1;
    swap_chain_desc1.Width              = m_ActualRTSize.w;
    swap_chain_desc1.Height             = m_ActualRTSize.h;
    swap_chain_desc1.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc1.Stereo             = false;
    swap_chain_desc1.SampleDesc.Count   = 1;
    swap_chain_desc1.SampleDesc.Quality = 0;
    swap_chain_desc1.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc1.BufferCount        = SWAP_CHAIN_BUFFERS_COUNT;
    swap_chain_desc1.Scaling            = DXGI_SCALING_STRETCH;
    swap_chain_desc1.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_chain_desc1.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_chain_desc1.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
                                        | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    if (m_TearingSupported)
    {
        swap_chain_desc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_desc;
    swap_chain_fullscreen_desc.RefreshRate.Numerator   = 0;
    swap_chain_fullscreen_desc.RefreshRate.Denominator = 1;
    swap_chain_fullscreen_desc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_fullscreen_desc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
    swap_chain_fullscreen_desc.Windowed                = true;

    IDXGISwapChain1 *swap_chain1 = null;
    error = m_Factory->CreateSwapChainForHwnd(m_GraphicsQueue,
                                              m_Window->Handle(),
                                              &swap_chain_desc1,
                                              &swap_chain_fullscreen_desc,
                                              null,
                                              &swap_chain1);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = swap_chain1->QueryInterface(&m_SwapChain);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    SafeRelease(swap_chain1);

    error = m_Factory->MakeWindowAssociation(m_Window->Handle(), DXGI_MWA_NO_ALT_ENTER);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    error = m_SwapChain->SetMaximumFrameLatency(1);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    REV_DEBUG_RESULT(m_WaitableObject = m_SwapChain->GetFrameLatencyWaitableObject());
}

void Renderer::GetSwapChainRenderTargets()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc;
    rtv_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_desc_heap_desc.NumDescriptors = SWAP_CHAIN_BUFFERS_COUNT;
    rtv_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_desc_heap_desc.NodeMask       = 0;

    HRESULT error = m_Device->CreateDescriptorHeap(&rtv_desc_heap_desc, IID_PPV_ARGS(&m_RTVHeapDesc));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    m_RTVDescSize      = m_Device->GetDescriptorHandleIncrementSize(rtv_desc_heap_desc.Type);
    m_RTVCPUDescHandle = m_RTVHeapDesc->GetCPUDescriptorHandleForHeapStart();

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12Resource **rt_buffer = m_RTBuffers + i;
        error = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(rt_buffer));
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        m_Device->CreateRenderTargetView(m_RTBuffers[i], null, m_RTVCPUDescHandle);

        m_RTVCPUDescHandle.ptr += m_RTVDescSize;
    }

    m_RTVCPUDescHandle.ptr -= SWAP_CHAIN_BUFFERS_COUNT * m_RTVDescSize;
    m_CurrentBuffer         = m_SwapChain->GetCurrentBackBufferIndex();
}

void Renderer::CreateDepthBuffer()
{
    D3D12_DESCRIPTOR_HEAP_DESC dsv_desc_heap_desc;
    dsv_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_desc_heap_desc.NumDescriptors = 1;
    dsv_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_desc_heap_desc.NodeMask       = 0;

    HRESULT error = m_Device->CreateDescriptorHeap(&dsv_desc_heap_desc, IID_PPV_ARGS(&m_DSVHeapDesc));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

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
    ds_resource_desc.Width              = m_ActualRTSize.w;
    ds_resource_desc.Height             = m_ActualRTSize.h;
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

    error = m_Device->CreateCommittedResource(&ds_heap_properties,
                                                D3D12_HEAP_FLAG_SHARED,
                                                &ds_resource_desc,
                                                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                &ds_clear_value,
                                                IID_PPV_ARGS(&m_DSBuffer));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    dsv_desc.Format             = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags              = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    dsv_desc.Texture2D.MipSlice = 0;

    m_Device->CreateDepthStencilView(m_DSBuffer, &dsv_desc, m_DSVCPUDescHandle);
}

void Renderer::CreateFences()
{
    HRESULT error = m_Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_Fence));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    REV_DEBUG_RESULT(m_FenceEvent = CreateEventExA(null, "Fence Event", 0, EVENT_ALL_ACCESS));
}

void Renderer::ResizeBuffers()
{
    // Wait for the GPU, release swap chain buffers and DS buffer
    WaitForGPU();
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(m_RTBuffers[i]);
    }
    SafeRelease(m_DSBuffer);

    // Resize buffers
    u32 flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
              | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    if (m_TearingSupported)
    {
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    HRESULT error = m_SwapChain->ResizeBuffers(SWAP_CHAIN_BUFFERS_COUNT,
                                               m_ActualRTSize.w,
                                               m_ActualRTSize.h,
                                               DXGI_FORMAT_R8G8B8A8_UNORM,
                                               flags);
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    // Recreate RT buffer & RTV
    m_RTVCPUDescHandle = m_RTVHeapDesc->GetCPUDescriptorHandleForHeapStart();

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12Resource **rt_buffer = m_RTBuffers + i;
        error = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(rt_buffer));
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        m_Device->CreateRenderTargetView(m_RTBuffers[i], null, m_RTVCPUDescHandle);

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
    ds_resource_desc.Width              = m_ActualRTSize.w;
    ds_resource_desc.Height             = m_ActualRTSize.h;
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

    error = m_Device->CreateCommittedResource(&ds_heap_properties,
                                                D3D12_HEAP_FLAG_SHARED,
                                                &ds_resource_desc,
                                                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                &ds_clear_value,
                                                IID_PPV_ARGS(&m_DSBuffer));
    REV_CHECK(CheckResultAndPrintMessages(error, this));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    dsv_desc.Format             = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags              = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    dsv_desc.Texture2D.MipSlice = 0;

    m_Device->CreateDepthStencilView(m_DSBuffer, &dsv_desc, m_DSVCPUDescHandle);

    m_FirstFrame = true;
}

void Renderer::SetFullscreenMode(bool set)
{
    if (set != m_Fullscreen)
    {
        HRESULT error = m_SwapChain->SetFullscreenState(set, null);
        REV_CHECK(CheckResultAndPrintMessages(error, this));

        if (set)
        {
            m_ActualRTSize = m_Window->Size();
            ResizeBuffers();
            m_Fullscreen = true;
        }
        else
        {
            m_ActualRTSize = m_RTSize;
            ResizeBuffers();
            m_Fullscreen = false;
        }
    }
}

}
