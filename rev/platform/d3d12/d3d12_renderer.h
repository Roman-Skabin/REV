//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "graphics/renderer.h"
#include "core/memory.h"
#include "core/window.h"
#include "tools/event.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

namespace REV::D3D12
{
    // @TODO(Roman): Various number of back buffers?
    enum
    {
        SWAP_CHAIN_BUFFERS_COUNT = 2,
    };

    class REV_API Renderer final
    {
    public:
        Renderer(Window *window, const Logger& logger, Math::v2s rt_size);
        ~Renderer();

        void StartFrame();
        void EndFrame();

        bool VSyncEnabled()        { return m_VsyncEnabled;   }
        void SetVSync(bool enable) { m_VsyncEnabled = enable; }

        void WaitForGPU();

        constexpr const ID3D12Device              *Device()              const { return m_Device;                         }
        constexpr const ID3D12CommandQueue        *GraphicsQueue()       const { return m_GraphicsQueue;                  }
        constexpr const ID3D12GraphicsCommandList *CurrentGraphicsList() const { return m_GraphicsLists[m_CurrentBuffer]; }
        constexpr const Logger&                    GetLogger()           const { return m_Logger;                         }

        constexpr ID3D12Device              *Device()              { return m_Device;                         }
        constexpr ID3D12CommandQueue        *GraphicsQueue()       { return m_GraphicsQueue;                  }
        constexpr ID3D12GraphicsCommandList *CurrentGraphicsList() { return m_GraphicsLists[m_CurrentBuffer]; }
        constexpr Logger&                    GetLogger()           { return m_Logger;                         }

        constexpr bool                       HalfPrecisionSupported()      const { return m_Features.options.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT; }
        constexpr D3D_ROOT_SIGNATURE_VERSION HighestRootSignatureVersion() const { return m_Features.root_signature.HighestVersion; }
        constexpr u32                        CurrentBuffer()               const { return m_CurrentBuffer; }
        constexpr D3D12_RESOURCE_HEAP_TIER   ResourceHeapTier()            const { return m_Features.options.ResourceHeapTier; }
        constexpr D3D_SHADER_MODEL           HighestShaderModel()          const { return m_Features.shader_model.HighestShaderModel; }

    #if REV_DEBUG
        constexpr const IDXGIInfoQueue *InfoQueue() const { return m_InfoQueue; }
        constexpr       IDXGIInfoQueue *InfoQueue()       { return m_InfoQueue; }
    #endif

        constexpr bool FirstFrame()       const { return m_FirstFrame;       }
        constexpr bool TearingSupported() const { return m_TearingSupported; }
        constexpr bool InFullscreenMode() const { return m_Fullscreen;       }

        Renderer& operator=(Renderer&& other) noexcept;

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

        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&)      = delete;

        Renderer& operator=(const Renderer&) = delete;

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

        friend class ::REV::Renderer;
    };
}
