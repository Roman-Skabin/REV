// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/window.h"

namespace REV::GPU
{
    class DeviceContext;
    enum TEXTURE_FORMAT : u32;
}

namespace REV::D3D12
{
    // @TODO(Roman): Various number of back buffers?
    enum
    {
        SWAP_CHAIN_BUFFERS_COUNT = 2,
    };

    class DeviceContext final
    {
    public:
        DeviceContext(Window *window, const Logger& logger);
        ~DeviceContext();

        void StartFrame();
        void EndFrame();

        bool VSyncEnabled()        { return m_VsyncEnabled;   }
        void SetVSync(bool enable) { m_VsyncEnabled = enable; }

        void WaitForGPU();

        u8 GetFormatPlanesCount(DXGI_FORMAT format);
        u8 GetFormatPlanesCount(GPU::TEXTURE_FORMAT format);

        REV_INLINE ID3D12Device                *Device()                      { return m_Device; }
        REV_INLINE ID3D12CommandQueue          *GraphicsQueue()               { return m_GraphicsQueue; }
        REV_INLINE ID3D12GraphicsCommandList   *CurrentGraphicsList()         { return m_GraphicsLists[m_CurrentBuffer]; }
        REV_INLINE Logger&                      GetLogger()                   { return m_Logger; }
        REV_INLINE bool                         HalfPrecisionSupported()      { return m_Features.options.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT; }
        REV_INLINE D3D_ROOT_SIGNATURE_VERSION   HighestRootSignatureVersion() { return m_Features.root_signature.HighestVersion; }
        REV_INLINE u32                          CurrentBuffer()               { return m_CurrentBuffer; }
        REV_INLINE D3D12_RESOURCE_HEAP_TIER     ResourceHeapTier()            { return m_Features.options.ResourceHeapTier; }
        REV_INLINE D3D_SHADER_MODEL             HighestShaderModel()          { return m_Features.shader_model.HighestShaderModel; }
        REV_INLINE D3D12_CPU_DESCRIPTOR_HANDLE  CurrentRTCPUDescHeap()        { return m_RTVCPUDescHandle; }
        REV_INLINE ID3D12Resource              *CurrentRenderTarget()         { return m_RTBuffers[m_CurrentBuffer]; }
        REV_INLINE D3D12_CPU_DESCRIPTOR_HANDLE  DSCPUDescHeap()               { return m_DSVCPUDescHandle; }
        REV_INLINE u32                          RTVDescSize()                 { return m_RTVDescSize; }
        REV_INLINE Window                      *GetWindow()                   { return m_Window; }
        REV_INLINE Math::v2s REV_VECTORCALL     RTSize()                      { return m_ActualRTSize; }

    #if REV_DEBUG
        REV_INLINE IDXGIInfoQueue *InfoQueue() { return m_InfoQueue; }
    #endif

        REV_INLINE bool FirstFrame()       const { return m_FirstFrame;       }
        REV_INLINE bool TearingSupported() const { return m_TearingSupported; }
        REV_INLINE bool InFullscreenMode() const { return m_Fullscreen;       }
        REV_INLINE bool FrameStarted()     const { return m_FrameStarted;     }

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

        void ResizeBuffers();

        void SetFullscreenMode(bool set);

        REV_DELETE_CONSTRS_AND_OPS(DeviceContext);

    private:
        Logger                       m_Logger;
        Window                      *m_Window;
        Math::v2s                    m_RTSize;
        Math::v2s                    m_ActualRTSize; // @NOTE(Roman): In windowed mode = m_RTSize, in fullscreen mode = m_Window->Size().

    #if REV_DEBUG
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

        IDXGISwapChain4             *m_SwapChain;
        HANDLE                       m_WaitableObject;

        ID3D12DescriptorHeap        *m_RTVHeapDesc;
        ID3D12Resource              *m_RTBuffers[SWAP_CHAIN_BUFFERS_COUNT];
        D3D12_CPU_DESCRIPTOR_HANDLE  m_RTVCPUDescHandle;
        u32                          m_RTVDescSize;
        u32                          m_CurrentBuffer;

        ID3D12DescriptorHeap        *m_DSVHeapDesc;
        ID3D12Resource              *m_DSBuffer;
        D3D12_CPU_DESCRIPTOR_HANDLE  m_DSVCPUDescHandle;

        ID3D12Fence                 *m_Fence;
        HANDLE                       m_FenceEvent;

        // Flags
        u32 m_VsyncEnabled     : 1;
        u32 m_FirstFrame       : 1;
        u32 m_TearingSupported : 1;
        u32 m_Fullscreen       : 1;
        u32 m_FrameStarted     : 1;

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

        friend class ::REV::GPU::DeviceContext;
    };
}
