//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "renderer/renderer.h"
#include "core/memory.h"
#include "core/window.h"
#include "tools/event.h"

namespace D3D12
{
    // @TODO(Roman): Various number of back buffers?
    enum
    {
        SWAP_CHAIN_BUFFERS_COUNT = 2,
    };

    class ENGINE_IMPEXP Renderer final : public IRenderer
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
        Renderer(Window *window, const Logger& logger);
        ~Renderer();

        virtual void Destroy() override;

        virtual void ResizeBuffers() override;
        virtual void ResizeTarget()  override;

        virtual void StartFrame() override;
        virtual void EndFrame()   override;

        virtual bool VSyncEnabled() override;
        virtual void SetVSync(bool enable) override;

        void *operator new(size_t) { return Memory::Get()->PushToPA<Renderer>(); }
        void  operator delete(void *) {}

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

        void WaitForGPU();
        void FlushGPU();

        friend u32 WINAPI LogInfoQueueMessages(void *arg);

        Renderer(const Renderer&) = delete;
        Renderer(Renderer&&)      = delete;

        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&)      = delete;

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
        // @Issue(Roman): Useless??
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
        HRESULT                      m_Error;

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

        // @Issue(Roman): Ok, what about creating a manager with another API? Kill thread? Store thread handle then?
    #if DEBUG
        Event                        m_LoggingEvent;
        bool                         m_StopLogging;
    #endif

        // @TODO(Roman): Deal with all these friend classes. This is not normal.
        friend class GPUMemoryManager;
        friend class GPUResourceMemory;
        friend class GPUResource;
        friend class GPUDescHeapMemory;
    };

    ENUM_CLASS_OPERATORS(Renderer::FLAGS);

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_base_of_v<IUnknown, T>>>
    INLINE void SafeRelease(T *& unknown)
    {
        if (unknown)
        {
            unknown->Release();
            unknown = null;
        }
    }
};
