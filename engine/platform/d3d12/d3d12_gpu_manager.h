//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "renderer/gpu_manager.h"

// @TODO(Roman): Various number of back buffers?
enum GPU_MANAGER_CONSTANTS
{
    SWAP_CHAIN_BUFFERS_COUNT = 2,
};

class ENGINE_IMPEXP D3D12GPUManager : public IGPUManager
{
public:
    enum class FLAGS
    {
        NONE              = 0,
        VSYNC_ENABLED     = BIT(0),
        FIRST_FRAME       = BIT(1),
        TEARING_SUPPORTED = BIT(2),
    };

public:
    D3D12GPUManager(Window *window, const Logger& logger);
    D3D12GPUManager(D3D12GPUManager&& other) noexcept;

    ~D3D12GPUManager();

    virtual void Destroy() override;

    virtual void WaitForGPU() override;
    virtual void FlushGPU()   override;

    virtual void ResizeBuffers() override;
    virtual void ResizeTarget()  override;

    virtual void StartFrame() override;
    virtual void EndFrame()   override;

    D3D12GPUManager& operator=(D3D12GPUManager&& other) noexcept;

private:
    void CreateDebugLayer();
    void CreateFactory();
    void CreateAdapterAndDevice();
    void GetInfoAboutFeatures();
    void CreateGraphicsQueueAllocatorsAndLists();
    void CreateSwapChain();
    void GetSwapChainRenderTargets();
    void CreateDepthBuffer();
    void CreateFences();

    D3D12GPUManager(const D3D12GPUManager&)            = delete;
    D3D12GPUManager& operator=(const D3D12GPUManager&) = delete;

private:
    Logger                       m_Logger;
    Window                      *m_Window;

#if DEBUG
    ID3D12Debug1                *m_Debug;
    IDXGIDebug1                 *m_DXGIDebug;
    IDXGIInfoQueue              *m_InfoQueue;
#endif

    IDXGIFactory2               *m_Factory;
    IDXGIAdapter1               *m_Adapter;
    ID3D12Device                *m_Device;

    ID3D12CommandQueue          *m_GraphicsQueue;
    ID3D12CommandAllocator      *m_GraphicsAllocators[SWAP_CHAIN_BUFFERS_COUNT];
    ID3D12GraphicsCommandList   *m_GraphicsLists[SWAP_CHAIN_BUFFERS_COUNT];
#if DEBUG
    // @Issue(Roman): Useles??
    ID3D12DebugCommandList      *m_DebugGraphicsLists[SWAP_CHAIN_BUFFERS_COUNT];
#endif

    IDXGISwapChain4             *m_SwapChain;

    ID3D12DescriptorHeap        *m_RTVHeapDesc;
    ID3D12Resource              *m_RTBuffers[SWAP_CHAIN_BUFFERS_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE  m_RTVCPUDescHandle;
    u32                          m_RTVDescSize;
    u32                          m_CurrentBuffer;

    ID3D12DescriptorHeap        *m_DSVHeapDesc;
    ID3D12Resource              *m_DSBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE  m_DSVCPUDescHandle;

    ID3D12Fence                 *m_Fences[SWAP_CHAIN_BUFFERS_COUNT];
    u64                          m_FencesValues[SWAP_CHAIN_BUFFERS_COUNT];
    HANDLE                       m_FenceEvent;

    FLAGS                        m_Flags;

    // @TODO(Roman): Do we need it all???
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
    } m_Features;

    HRESULT                      m_Error;
};

ENUM_CLASS_OPERATORS(D3D12GPUManager::FLAGS);

template<typename T, typename = RTTI::enable_if_t<RTTI::is_base_of_v<IUnknown, T>>>
INLINE void SafeRelease(T *& unknown)
{
    if (unknown)
    {
        unknown->Release();
        unknown = null;
    }
}
