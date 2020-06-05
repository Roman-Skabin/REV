//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "cengine.h"
#include "core/id.h"
#include "math/color.h"

#pragma warning(push)
#pragma warning(disable: 4133)

//
// Input
//

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(XInputGetStateProc);
internal XInputGetStateProc *XInputGetState_;

global HMODULE gXinput;

internal INLINE void DigitalButtonPull(DigitalButton *button, b32 down)
{
    b32 was_down = button->down;

    button->down     =  down;
    button->pressed  = !was_down &&  down;
    button->released =  was_down && !down;
}

internal INLINE void AnalogButtonPull(AnalogButton *button, f32 value)
{
    b32 was_down = button->down;

    button->value    =  value;
    button->down     =  button->value >= button->threshold;
    button->pressed  = !was_down &&  button->down;
    button->released =  was_down && !button->down;
}

internal INLINE void StickPull(Stick *stick, f32 x, f32 y)
{
    #define ABSF(val) ((val) < 0.0f ? (-(val)) : (val))
    stick->offset.x = ABSF(x) <= stick->deadzone ? x : 0;
    stick->offset.y = ABSF(y) <= stick->deadzone ? y : 0;
    #undef ABSF
}

internal void GamepadPull(Engine *engine)
{
    XINPUT_STATE xinput_state = {0};

    b32 was_connected = engine->input.gamepad.connected;
    engine->input.gamepad.connected = XInputGetState_(0, &xinput_state) == ERROR_SUCCESS;

    /**/ if (!was_connected &&  engine->input.gamepad.connected) Log(&engine->logger, "gamepad was connected");
    else if ( was_connected && !engine->input.gamepad.connected) Log(&engine->logger, "gamepad was disconnected");

    if (engine->input.gamepad.connected)
    {
        #define IS_DIGITAL_DOWN(button) ((xinput_state.Gamepad.wButtons & (button)) != 0)

        DigitalButtonPull(&engine->input.gamepad.button_a, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_A));
        DigitalButtonPull(&engine->input.gamepad.button_b, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_B));
        DigitalButtonPull(&engine->input.gamepad.button_x, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_X));
        DigitalButtonPull(&engine->input.gamepad.button_y, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_Y));

        DigitalButtonPull(&engine->input.gamepad.left_shoulder, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_SHOULDER));
        DigitalButtonPull(&engine->input.gamepad.right_shoulder, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_SHOULDER));

        DigitalButtonPull(&engine->input.gamepad.left_stick.button, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_THUMB));
        DigitalButtonPull(&engine->input.gamepad.right_stick.button, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_THUMB));
        
        DigitalButtonPull(&engine->input.gamepad.button_start, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_START));
        DigitalButtonPull(&engine->input.gamepad.button_back, IS_DIGITAL_DOWN(XINPUT_GAMEPAD_BACK));

        #undef IS_DIGITAL_DOWN

        AnalogButtonPull(&engine->input.gamepad.left_trigger, xinput_state.Gamepad.bLeftTrigger / cast(f32, MAXBYTE));
        AnalogButtonPull(&engine->input.gamepad.right_trigger, xinput_state.Gamepad.bRightTrigger / cast(f32, MAXBYTE));

        #define STICK_OFFSET(stick_dir) (2.0f * (((stick_dir) + 0x8000) / cast(f32, MAXWORD) - 0.5f))

        StickPull(&engine->input.gamepad.left_stick, STICK_OFFSET(xinput_state.Gamepad.sThumbLX), STICK_OFFSET(xinput_state.Gamepad.sThumbLY));
        StickPull(&engine->input.gamepad.right_stick, STICK_OFFSET(xinput_state.Gamepad.sThumbRX), STICK_OFFSET(xinput_state.Gamepad.sThumbRY));

        #undef STICK_OFFSET
    }
}

internal void InputCreate(Engine *engine)
{
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.dwFlags     = 0;
    rid.hwndTarget  = engine->window.handle;

    if (RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        Success(&engine->logger, "Mouse was created");

        POINT screen_pos = {0};
        RECT  window_rect = {0};
        if (GetCursorPos(&screen_pos)
        &&  GetWindowRect(engine->window.handle, &window_rect))
        {
            engine->input.mouse.pos.x = screen_pos.x - window_rect.left;
            engine->input.mouse.pos.y = window_rect.bottom - (screen_pos.y - window_rect.top);
        }
    }
    else
    {
        Error(&engine->logger, "Mouse was not created");
        FailedM("Mouse creation failure");
    }

    if (!gXinput)
    {
        gXinput = LoadLibraryA(XINPUT_DLL_A);
        Check(gXinput);

        XInputGetState_ = cast(XInputGetStateProc *, GetProcAddress(gXinput, "XInputGetState"));
        Check(XInputGetState_);
    }

    engine->input.gamepad.left_trigger.threshold  = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / cast(f32, MAXBYTE);
    engine->input.gamepad.right_trigger.threshold = engine->input.gamepad.left_trigger.threshold;

    engine->input.gamepad.left_stick.deadzone  = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  / cast(f32, MAXSHORT);
    engine->input.gamepad.right_stick.deadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / cast(f32, MAXSHORT);

    Success(&engine->logger, "Gamepad created");

    XINPUT_STATE xinput_state = {0};
    if (engine->input.gamepad.connected = (XInputGetState_(0, &xinput_state) == ERROR_SUCCESS))
    {
        Log(&engine->logger, "Gamepad is connected");
    }
    else
    {
        Log(&engine->logger, "Gamepad is not connected");
    }
}

internal void InputDestroy(Engine *engine)
{
    if (gXinput) FreeLibrary(gXinput);
    Log(&engine->logger, "Input was destroyed");
}

internal void InputReset(Engine *engine)
{
    engine->input.mouse.dpos.x = 0;
    engine->input.mouse.dpos.y = 0;
    engine->input.mouse.dwheel = 0;

    engine->input.mouse.left_button.pressed    = false;
    engine->input.mouse.left_button.released   = false;

    engine->input.mouse.middle_button.pressed  = false;
    engine->input.mouse.middle_button.released = false;

    engine->input.mouse.right_button.pressed   = false;
    engine->input.mouse.right_button.released  = false;

    engine->input.mouse.x1_button.pressed      = false;
    engine->input.mouse.x1_button.released     = false;

    engine->input.mouse.x2_button.pressed      = false;
    engine->input.mouse.x2_button.released     = false;
}

internal INLINE void InputPull(Engine *engine)
{
    BYTE keyboard_state[256];
    GetKeyboardState(keyboard_state);

    for (s16 key = KEY_FIRST; key < KEY_MAX; ++key)
    {
        DigitalButtonPull(engine->input.keys + key, keyboard_state[key] >> 7);
    }

    GamepadPull(engine);
}

//
// Timer
//

#define QPF(s64_val) QueryPerformanceFrequency(cast(LARGE_INTEGER *, &(s64_val)))
#define QPC(s64_val) QueryPerformanceCounter(cast(LARGE_INTEGER *, &(s64_val)))

internal void TimerCreate(Engine *engine)
{
    if (QPF(engine->timer.ticks_per_second) && QPC(engine->timer.initial_ticks))
    {
        Success(&engine->logger, "Timer was created");
    }
    else
    {
        Error(&engine->logger, "Timer was not created");
        FailedM("Timer creation failure");
    }

    engine->timer.stopped    = true;
    engine->timer.stop_begin = engine->timer.initial_ticks;
}

internal void TimerPull(Engine *engine)
{
    s64 cur_ticks;
    QPC(cur_ticks);
    cur_ticks -= engine->timer.initial_ticks + engine->timer.stop_duration;

    if (!engine->timer.stopped)
    {
        engine->timer.delta_ticks = cur_ticks - engine->timer.ticks;
        engine->timer.ticks       = cur_ticks;

        engine->timer.delta_seconds = engine->timer.delta_ticks / cast(f32, engine->timer.ticks_per_second);
        engine->timer.seconds       = engine->timer.ticks       / cast(f32, engine->timer.ticks_per_second);
    }

    engine->timer.total_seconds = (cur_ticks + engine->timer.stop_duration) / cast(f32, engine->timer.ticks_per_second);
}

void TimerStart(Engine *engine)
{
    if (engine->timer.stopped)
    {
        s64 stop_end;
        QPC(stop_end);
        engine->timer.stop_last_duration  = stop_end - engine->timer.stop_begin;
        engine->timer.stop_duration      += engine->timer.stop_last_duration;
        engine->timer.stopped             = false;

        Log(&engine->logger, "Timer was started");
    }
}

void TimerStop(Engine *engine)
{
    if (!engine->timer.stopped)
    {
        QPC(engine->timer.stop_begin);
        engine->timer.stopped = true;

        Log(&engine->logger, "Timer was stopped");
    }
}

//
// CoreRenderer
//

#define SafeRelease(directx_interface)                           \
{                                                                \
    if (directx_interface)                                       \
    {                                                            \
        (directx_interface)->lpVtbl->Release(directx_interface); \
        (directx_interface) = 0;                                 \
    }                                                            \
}

internal void CreateAdapterAndDevice(Engine *engine)
{
    IDXGIAdapter1 *test_adapter  = 0;
    ID3D12Device  *test_device   = 0;
    SIZE_T         max_vram      = 0;
    UINT           adapter_index = 0;

    for (UINT i = 0;
         engine->core_renderer.factory->lpVtbl->EnumAdapters1(engine->core_renderer.factory, i, &test_adapter) != DXGI_ERROR_NOT_FOUND;
         ++i)
    {
        engine->core_renderer.error = D3D12CreateDevice(test_adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, &test_device);
        if (SUCCEEDED(engine->core_renderer.error))
        {
            DXGI_ADAPTER_DESC1 ad1;
            engine->core_renderer.error = test_adapter->lpVtbl->GetDesc1(test_adapter, &ad1);
            if (SUCCEEDED(engine->core_renderer.error) && max_vram < ad1.DedicatedVideoMemory)
            {
                max_vram      = ad1.DedicatedVideoMemory;
                adapter_index = i;
            }

            SafeRelease(test_device);
        }

        SafeRelease(test_adapter);
    }

    if (max_vram)
    {
        engine->core_renderer.error = engine->core_renderer.factory->lpVtbl->EnumAdapters1(engine->core_renderer.factory, adapter_index, &engine->core_renderer.adapter);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.error = D3D12CreateDevice(engine->core_renderer.adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, &engine->core_renderer.device);
        Check(SUCCEEDED(engine->core_renderer.error));
    }
    else
    {
        for (UINT i = 0;
             engine->core_renderer.factory->lpVtbl->EnumAdapters1(engine->core_renderer.factory, i, &test_adapter) != D3D12_ERROR_ADAPTER_NOT_FOUND;
             ++i)
        {
            engine->core_renderer.error = D3D12CreateDevice(test_adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &test_device);
            if (SUCCEEDED(engine->core_renderer.error))
            {
                DXGI_ADAPTER_DESC1 ad1;
                engine->core_renderer.error = test_adapter->lpVtbl->GetDesc1(test_adapter, &ad1);
                if (SUCCEEDED(engine->core_renderer.error) && max_vram < ad1.DedicatedVideoMemory)
                {
                    max_vram      = ad1.DedicatedVideoMemory;
                    adapter_index = i;
                }

                SafeRelease(test_device);
            }

            SafeRelease(test_adapter);
        }

        CheckM(max_vram, "Direct3D 12 is not supported by your hardware");

        engine->core_renderer.error = engine->core_renderer.factory->lpVtbl->EnumAdapters1(engine->core_renderer.factory, adapter_index, &engine->core_renderer.adapter);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.error = D3D12CreateDevice(engine->core_renderer.adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &engine->core_renderer.device);
        Check(SUCCEEDED(engine->core_renderer.error));
    }
}

internal void CreateCoreRenderer(Engine *engine)
{
#if DEBUG
    engine->core_renderer.error = D3D12GetDebugInterface(&IID_ID3D12Debug, &engine->core_renderer.debug);
    Check(SUCCEEDED(engine->core_renderer.error));

    engine->core_renderer.debug->lpVtbl->EnableDebugLayer(engine->core_renderer.debug);
#endif

    UINT factory_create_flags = 0;
#if DEBUG
    factory_create_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    engine->core_renderer.error = CreateDXGIFactory2(factory_create_flags, &IID_IDXGIFactory, &engine->core_renderer.factory);
    Check(SUCCEEDED(engine->core_renderer.error));

    CreateAdapterAndDevice(engine);

    D3D12_COMMAND_QUEUE_DESC cqd;
    cqd.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cqd.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cqd.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cqd.NodeMask = 0;

    engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateCommandQueue(engine->core_renderer.device,
                                                                                           &cqd,
                                                                                           &IID_ID3D12CommandQueue,
                                                                                           &engine->core_renderer.queue);
    Check(SUCCEEDED(engine->core_renderer.error));

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateCommandAllocator(engine->core_renderer.device,
                                                                                                   cqd.Type,
                                                                                                   &IID_ID3D12CommandAllocator,
                                                                                                   engine->core_renderer.graphics_allocators + i);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateCommandList(engine->core_renderer.device,
                                                                                              cqd.NodeMask,
                                                                                              cqd.Type,
                                                                                              engine->core_renderer.graphics_allocators[i],
                                                                                              0,
                                                                                              &IID_ID3D12GraphicsCommandList,
                                                                                              engine->core_renderer.graphics_lists + i);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.error = engine->core_renderer.graphics_lists[i]->lpVtbl->Close(engine->core_renderer.graphics_lists[i]);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateCommandAllocator(engine->core_renderer.device,
                                                                                                   cqd.Type,
                                                                                                   &IID_ID3D12CommandAllocator,
                                                                                                   engine->core_renderer.compute_allocators + i);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateCommandList(engine->core_renderer.device,
                                                                                              cqd.NodeMask,
                                                                                              cqd.Type,
                                                                                              engine->core_renderer.compute_allocators[i],
                                                                                              0,
                                                                                              &IID_ID3D12GraphicsCommandList,
                                                                                              engine->core_renderer.compute_lists + i);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.error = engine->core_renderer.compute_lists[i]->lpVtbl->Close(engine->core_renderer.compute_lists[i]);
        Check(SUCCEEDED(engine->core_renderer.error));
    }

    IDXGIFactory5 *factory5 = 0;
    engine->core_renderer.error = engine->core_renderer.factory->lpVtbl->QueryInterface(engine->core_renderer.factory,
                                                                                        &IID_IDXGIFactory5,
                                                                                        &factory5);
    Check(SUCCEEDED(engine->core_renderer.error));

    engine->core_renderer.error = factory5->lpVtbl->CheckFeatureSupport(factory5, DXGI_FEATURE_PRESENT_ALLOW_TEARING, &engine->core_renderer.tearing_supported, sizeof(b32));
    Check(SUCCEEDED(engine->core_renderer.error));

    SafeRelease(factory5);

    DXGI_SWAP_CHAIN_DESC1 scd1;
    scd1.Width              = engine->window.size.w;
    scd1.Height             = engine->window.size.h;
    scd1.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd1.Stereo             = false;
    scd1.SampleDesc.Count   = 1;
    scd1.SampleDesc.Quality = 0;
    scd1.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT; // DXGI_USAGE_SHARED ?
    scd1.BufferCount        = SWAP_CHAIN_BUFFERS_COUNT;
    scd1.Scaling            = DXGI_SCALING_STRETCH;
    scd1.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd1.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    scd1.Flags              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER ?

    if (engine->core_renderer.tearing_supported) scd1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC scfd;
    scfd.RefreshRate.Numerator   = 0;
    scfd.RefreshRate.Denominator = 1;
    scfd.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    scfd.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
    scfd.Windowed                = true;

    IDXGISwapChain1 *swap_chain1 = 0;
    engine->core_renderer.error = engine->core_renderer.factory->lpVtbl->CreateSwapChainForHwnd(engine->core_renderer.factory,
                                                                                                engine->core_renderer.queue,
                                                                                                engine->window.handle,
                                                                                                &scd1,
                                                                                                &scfd,
                                                                                                0,
                                                                                                &swap_chain1);
    Check(SUCCEEDED(engine->core_renderer.error));

    engine->core_renderer.error = swap_chain1->lpVtbl->QueryInterface(swap_chain1,
                                                                      &IID_IDXGISwapChain4,
                                                                      &engine->core_renderer.swap_chain);
    Check(SUCCEEDED(engine->core_renderer.error));

    engine->core_renderer.error = engine->core_renderer.factory->lpVtbl->MakeWindowAssociation(engine->core_renderer.factory,
                                                                                               engine->window.handle,
                                                                                               DXGI_MWA_NO_ALT_ENTER);
    Check(SUCCEEDED(engine->core_renderer.error));

    D3D12_DESCRIPTOR_HEAP_DESC rtv_dhd;
    rtv_dhd.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_dhd.NumDescriptors = SWAP_CHAIN_BUFFERS_COUNT;
    rtv_dhd.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_dhd.NodeMask       = 0;

    engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateDescriptorHeap(engine->core_renderer.device,
                                                                                             &rtv_dhd,
                                                                                             &IID_ID3D12DescriptorHeap,
                                                                                             &engine->core_renderer.rtv_heap_desc);
    Check(SUCCEEDED(engine->core_renderer.error));

    engine->core_renderer.rtv_desc_size = engine->core_renderer.device->lpVtbl->GetDescriptorHandleIncrementSize(engine->core_renderer.device, rtv_dhd.Type);

    #pragma warning(suppress: 4020) // I know what I'm doing.
    engine->core_renderer.rtv_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->core_renderer.rtv_heap_desc, &engine->core_renderer.rtv_cpu_desc_handle);

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->core_renderer.error = engine->core_renderer.swap_chain->lpVtbl->GetBuffer(engine->core_renderer.swap_chain,
                                                                                          i,
                                                                                          &IID_ID3D12Resource,
                                                                                          engine->core_renderer.rt_buffers + i);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.device->lpVtbl->CreateRenderTargetView(engine->core_renderer.device,
                                                                     engine->core_renderer.rt_buffers[i],
                                                                     0,
                                                                     engine->core_renderer.rtv_cpu_desc_handle);

        engine->core_renderer.rtv_cpu_desc_handle.ptr += engine->core_renderer.rtv_desc_size;
    }

    engine->core_renderer.rtv_cpu_desc_handle.ptr -= SWAP_CHAIN_BUFFERS_COUNT * engine->core_renderer.rtv_desc_size;
    engine->core_renderer.current_buffer = engine->core_renderer.swap_chain->lpVtbl->GetCurrentBackBufferIndex(engine->core_renderer.swap_chain);

    D3D12_DESCRIPTOR_HEAP_DESC dsv_dhd;
    dsv_dhd.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_dhd.NumDescriptors = 1;
    dsv_dhd.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_dhd.NodeMask       = 0;

    engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateDescriptorHeap(engine->core_renderer.device,
                                                                                             &dsv_dhd,
                                                                                             &IID_ID3D12DescriptorHeap,
                                                                                             &engine->core_renderer.ds_heap_desc);
    Check(SUCCEEDED(engine->core_renderer.error));

    D3D12_HEAP_PROPERTIES ds_hp;
    ds_hp.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    ds_hp.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    ds_hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    ds_hp.CreationNodeMask     = 0;
    ds_hp.VisibleNodeMask      = 0;

    D3D12_RESOURCE_DESC ds_rd;
    ds_rd.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ds_rd.Alignment          = 0;
    ds_rd.Width              = engine->window.size.w;
    ds_rd.Height             = engine->window.size.h;
    ds_rd.DepthOrArraySize   = 1;
    ds_rd.MipLevels          = 0;
    ds_rd.Format             = DXGI_FORMAT_D32_FLOAT;
    ds_rd.SampleDesc.Count   = 1;
    ds_rd.SampleDesc.Quality = 0;
    ds_rd.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ds_rd.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE ds_cv;
    ds_cv.Format               = DXGI_FORMAT_D32_FLOAT;
    ds_cv.DepthStencil.Depth   = 1.0f;
    ds_cv.DepthStencil.Stencil = 0;

    engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateCommittedResource(engine->core_renderer.device,
                                                                                                &ds_hp,
                                                                                                D3D12_HEAP_FLAG_SHARED,
                                                                                                &ds_rd,
                                                                                                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                                                                &ds_cv,
                                                                                                &IID_ID3D12Resource,
                                                                                                &engine->core_renderer.ds_buffer);
    Check(SUCCEEDED(engine->core_renderer.error));

    #pragma warning(suppress: 4020) // I know what I'm doing.
    engine->core_renderer.ds_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->core_renderer.ds_heap_desc, &engine->core_renderer.dsv_cpu_desc_handle);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = {0};
    dsvd.Format        = DXGI_FORMAT_D32_FLOAT;
    dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    engine->core_renderer.device->lpVtbl->CreateDepthStencilView(engine->core_renderer.device,
                                                                 engine->core_renderer.ds_buffer,
                                                                 &dsvd,
                                                                 engine->core_renderer.dsv_cpu_desc_handle);

    for (u64 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateFence(engine->core_renderer.device,
                                                                                        engine->core_renderer.fences_values[i],
                                                                                        D3D12_FENCE_FLAG_NONE, // D3D12_FENCE_FLAG_SHARED ?
                                                                                        &IID_ID3D12Fence,
                                                                                        engine->core_renderer.fences + i);
        Check(SUCCEEDED(engine->core_renderer.error));
        ++engine->core_renderer.fences_values[i];
    }

    engine->core_renderer.fence_event = CreateEventExA(0, "Fence Event", 0, EVENT_ALL_ACCESS);
    Check(engine->core_renderer.fence_event);

    engine->core_renderer.first_frame = true;

    Success(&engine->logger, "Renderer was created");
}

internal void WaitForGPU(Engine *engine)
{
    u64 current_fence_value = engine->core_renderer.fences_values[engine->core_renderer.current_buffer];
    engine->core_renderer.queue->lpVtbl->Signal(engine->core_renderer.queue,
                                                engine->core_renderer.fences[engine->core_renderer.current_buffer],
                                                current_fence_value);

    u64 value = engine->core_renderer.fences[engine->core_renderer.current_buffer]->lpVtbl->GetCompletedValue(engine->core_renderer.fences[engine->core_renderer.current_buffer]);
    if (value < engine->core_renderer.fences_values[engine->core_renderer.current_buffer])
    {
        engine->core_renderer.error = engine->core_renderer.fences[engine->core_renderer.current_buffer]->lpVtbl->SetEventOnCompletion(engine->core_renderer.fences[engine->core_renderer.current_buffer],
                                                                                                                                       engine->core_renderer.fences_values[engine->core_renderer.current_buffer],
                                                                                                                                       engine->core_renderer.fence_event);
        Check(SUCCEEDED(engine->core_renderer.error));
        while (WaitForSingleObjectEx(engine->core_renderer.fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }
    }

    engine->core_renderer.fences_values[engine->core_renderer.current_buffer] = current_fence_value + 1;
}

internal void FlushGPU(Engine *engine)
{
    for (u32 buffer_index = 0; buffer_index < SWAP_CHAIN_BUFFERS_COUNT; ++buffer_index)
    {
        engine->core_renderer.queue->lpVtbl->Signal(engine->core_renderer.queue,
                                                    engine->core_renderer.fences[buffer_index],
                                                    engine->core_renderer.fences_values[buffer_index]);

        engine->core_renderer.error = engine->core_renderer.fences[buffer_index]->lpVtbl->SetEventOnCompletion(engine->core_renderer.fences[buffer_index],
                                                                                                               engine->core_renderer.fences_values[buffer_index],
                                                                                                               engine->core_renderer.fence_event);
        Check(SUCCEEDED(engine->core_renderer.error));
        while (WaitForSingleObjectEx(engine->core_renderer.fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }

        ++engine->core_renderer.fences_values[buffer_index];
    }
}

internal void DestroyCoreRenderer(Engine *engine)
{
    SafeRelease(engine->core_renderer.ds_buffer);
    SafeRelease(engine->core_renderer.ds_heap_desc);
    DebugResult(CloseHandle(engine->core_renderer.fence_event));
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(engine->core_renderer.fences[i]);
        SafeRelease(engine->core_renderer.rt_buffers[i]);
        SafeRelease(engine->core_renderer.compute_lists[i]);
        SafeRelease(engine->core_renderer.compute_allocators[i]);
        SafeRelease(engine->core_renderer.graphics_lists[i]);
        SafeRelease(engine->core_renderer.graphics_allocators[i]);
    }
    SafeRelease(engine->core_renderer.rtv_heap_desc);
    SafeRelease(engine->core_renderer.swap_chain);
    SafeRelease(engine->core_renderer.queue);
    SafeRelease(engine->core_renderer.device);
    SafeRelease(engine->core_renderer.adapter);
    SafeRelease(engine->core_renderer.factory);
#if DEBUG
    SafeRelease(engine->core_renderer.debug);
#endif
}

internal void ResizeBuffers(Engine *engine)
{
    // Wait for the GPU, reset memory and release buffers
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->core_renderer.queue->lpVtbl->Signal(engine->core_renderer.queue,
                                                    engine->core_renderer.fences[i],
                                                    engine->core_renderer.fences_values[i]);

        engine->core_renderer.error = engine->core_renderer.fences[i]->lpVtbl->SetEventOnCompletion(engine->core_renderer.fences[i],
                                                                                                    engine->core_renderer.fences_values[i],
                                                                                                    engine->core_renderer.fence_event);
        Check(SUCCEEDED(engine->core_renderer.error));
        while (WaitForSingleObjectEx(engine->core_renderer.fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }

        ++engine->core_renderer.fences_values[i];

        SafeRelease(engine->core_renderer.rt_buffers[i]);
    }

    SafeRelease(engine->core_renderer.ds_buffer);

    // Resize buffers
    u32 flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER ?
    if (engine->core_renderer.tearing_supported) flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    engine->core_renderer.error = engine->core_renderer.swap_chain->lpVtbl->ResizeBuffers(engine->core_renderer.swap_chain,
                                                                                          SWAP_CHAIN_BUFFERS_COUNT,
                                                                                          engine->window.size.w,
                                                                                          engine->window.size.h,
                                                                                          DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                                          flags);
    Check(SUCCEEDED(engine->core_renderer.error));

    // Recreate buffers
    #pragma warning(suppress: 4020) // I know what I'm doing.
    engine->core_renderer.rtv_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->core_renderer.rtv_heap_desc, &engine->core_renderer.rtv_cpu_desc_handle);

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->core_renderer.error = engine->core_renderer.swap_chain->lpVtbl->GetBuffer(engine->core_renderer.swap_chain,
                                                                                          i,
                                                                                          &IID_ID3D12Resource,
                                                                                          engine->core_renderer.rt_buffers + i);
        Check(SUCCEEDED(engine->core_renderer.error));

        engine->core_renderer.device->lpVtbl->CreateRenderTargetView(engine->core_renderer.device,
                                                                     engine->core_renderer.rt_buffers[i],
                                                                     0,
                                                                     engine->core_renderer.rtv_cpu_desc_handle);

        engine->core_renderer.rtv_cpu_desc_handle.ptr += engine->core_renderer.rtv_desc_size;
    }

    engine->core_renderer.rtv_cpu_desc_handle.ptr -= SWAP_CHAIN_BUFFERS_COUNT * engine->core_renderer.rtv_desc_size;
    engine->core_renderer.current_buffer           = engine->core_renderer.swap_chain->lpVtbl->GetCurrentBackBufferIndex(engine->core_renderer.swap_chain);
    engine->core_renderer.rtv_cpu_desc_handle.ptr += engine->core_renderer.current_buffer * engine->core_renderer.rtv_desc_size;

    // Recreate DS buffer & DSV
    D3D12_HEAP_PROPERTIES ds_hp;
    ds_hp.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    ds_hp.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    ds_hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    ds_hp.CreationNodeMask     = 0;
    ds_hp.VisibleNodeMask      = 0;

    D3D12_RESOURCE_DESC ds_rd;
    ds_rd.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ds_rd.Alignment          = 0;
    ds_rd.Width              = engine->window.size.w;
    ds_rd.Height             = engine->window.size.h;
    ds_rd.DepthOrArraySize   = 1;
    ds_rd.MipLevels          = 0;
    ds_rd.Format             = DXGI_FORMAT_D32_FLOAT;
    ds_rd.SampleDesc.Count   = 1;
    ds_rd.SampleDesc.Quality = 0;
    ds_rd.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ds_rd.Flags              = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE ds_cv;
    ds_cv.Format               = DXGI_FORMAT_D32_FLOAT;
    ds_cv.DepthStencil.Depth   = 1.0f;
    ds_cv.DepthStencil.Stencil = 0;

    engine->core_renderer.error = engine->core_renderer.device->lpVtbl->CreateCommittedResource(engine->core_renderer.device,
                                                                                                &ds_hp,
                                                                                                D3D12_HEAP_FLAG_SHARED,
                                                                                                &ds_rd,
                                                                                                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                                                                &ds_cv,
                                                                                                &IID_ID3D12Resource,
                                                                                                &engine->core_renderer.ds_buffer);
    Check(SUCCEEDED(engine->core_renderer.error));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = {0};
    dsvd.Format        = DXGI_FORMAT_D32_FLOAT;
    dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    engine->core_renderer.device->lpVtbl->CreateDepthStencilView(engine->core_renderer.device,
                                                                 engine->core_renderer.ds_buffer,
                                                                 &dsvd,
                                                                 engine->core_renderer.dsv_cpu_desc_handle);

    engine->core_renderer.first_frame = true;
}

internal void Present(Engine *engine)
{
    ID3D12GraphicsCommandList *graphics_list = engine->core_renderer.graphics_lists[engine->core_renderer.current_buffer];
    ID3D12GraphicsCommandList *compute_list  = engine->core_renderer.compute_lists[engine->core_renderer.current_buffer];

    // Complete stage
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = engine->core_renderer.rt_buffers[engine->core_renderer.current_buffer];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

    graphics_list->lpVtbl->ResourceBarrier(graphics_list, 1, &barrier);

    engine->core_renderer.error = graphics_list->lpVtbl->Close(graphics_list);
    Check(SUCCEEDED(engine->core_renderer.error));

    engine->core_renderer.error = compute_list->lpVtbl->Close(compute_list);
    Check(SUCCEEDED(engine->core_renderer.error));

    ID3D12CommandList *lists[] =
    {
        graphics_list,
        compute_list
    };
    engine->core_renderer.queue->lpVtbl->ExecuteCommandLists(engine->core_renderer.queue, ArrayCount(lists), lists);
    Check(SUCCEEDED(engine->core_renderer.error));

    // Present
    u32 present_flags = 0;
    if (!engine->core_renderer.vsync)
    {
        present_flags |= DXGI_PRESENT_DO_NOT_WAIT;
        if (engine->core_renderer.tearing_supported)
        {
            present_flags |= DXGI_PRESENT_ALLOW_TEARING;
        }
    }
    if (engine->core_renderer.first_frame)
    {
        engine->core_renderer.first_frame = false;
    }
    else
    {
        present_flags |= DXGI_PRESENT_DO_NOT_SEQUENCE;
    }

    engine->core_renderer.error = engine->core_renderer.swap_chain->lpVtbl->Present(engine->core_renderer.swap_chain,
                                                                                    engine->core_renderer.vsync,
                                                                                    present_flags);
    Check(SUCCEEDED(engine->core_renderer.error));

    // Swap buffers
    engine->core_renderer.current_buffer = engine->core_renderer.swap_chain->lpVtbl->GetCurrentBackBufferIndex(engine->core_renderer.swap_chain);

    #pragma warning(suppress: 4020)
    engine->core_renderer.rtv_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->core_renderer.rtv_heap_desc, &engine->core_renderer.rtv_cpu_desc_handle);
    engine->core_renderer.rtv_cpu_desc_handle.ptr += engine->core_renderer.current_buffer * engine->core_renderer.rtv_desc_size;
}

//
// Window
//

internal LRESULT WINAPI WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

internal void WindowCreate(Engine *engine, const char *title, v2s size, HINSTANCE instance)
{
    WNDCLASSA wca     = {0};
    wca.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wca.lpfnWndProc   = WindowProc;
    wca.hInstance     = instance;
    wca.hCursor       = LoadCursorA(0, IDC_ARROW);
    wca.lpszClassName = "cengine_window_class";
    DebugResult(RegisterClassA(&wca));

    engine->window.size = size;

    s32 width  = engine->window.size.w;
    s32 height = engine->window.size.h;

    RECT wr = { 0, 0, width, height };
    if (AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false))
    {
        width  = wr.right  - wr.left;
        height = wr.bottom - wr.top;
    }

    engine->window.handle = CreateWindowA(wca.lpszClassName, title, WS_OVERLAPPEDWINDOW, 10, 10, width, height, 0, 0, wca.hInstance, 0);
    Check(engine->window.handle);

    engine->window.context = GetDC(engine->window.handle);
    Check(engine->window.context);

    Success(&engine->logger, "Window was created");

    engine->monitor.handle = MonitorFromWindow(engine->window.handle, MONITOR_DEFAULTTONEAREST);
    Check(engine->monitor.handle);

    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    DebugResult(GetMonitorInfoA(engine->monitor.handle, &info));

    engine->monitor.pos  = v2s_1(info.rcMonitor.left, info.rcMonitor.top);
    engine->monitor.size = v2s_1(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);

    Success(&engine->logger, "Monitor was initialized");

    SetWindowLongPtrA(engine->window.handle, GWLP_USERDATA, cast(LONG_PTR, engine));
}

internal void WindowDestroy(Engine *engine, HINSTANCE instance)
{
    DebugResult(UnregisterClassA("cengine_window_class", instance));
    Log(&engine->logger, "Window was destroyed");
}

void SetFullscreen(Engine *engine, b32 set)
{
    local RECT wr;

    if (engine->window.fullscreened == BIT(31))
    {
        DebugResult(GetWindowRect(engine->window.handle, &wr));
        SetWindowLongPtrA(engine->window.handle, GWL_STYLE,
                          GetWindowLongPtrA(engine->window.handle, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
        DebugResult(SetWindowPos(engine->window.handle, HWND_TOP,
                                 engine->monitor.pos.x, engine->monitor.pos.y,
                                 engine->monitor.size.w, engine->monitor.size.h,
                                 SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED | SWP_DEFERERASE | SWP_NOCOPYBITS | SWP_NOREDRAW));
        engine->window.fullscreened = true;
    }
    else if (engine->window.fullscreened == BIT(30))
    {
        SetWindowLongPtrA(engine->window.handle, GWL_STYLE,
                          GetWindowLongPtrA(engine->window.handle, GWL_STYLE) | WS_OVERLAPPEDWINDOW);
        DebugResult(SetWindowPos(engine->window.handle, HWND_TOP,
                                 wr.left, wr.top, wr.right - wr.left, wr.bottom - wr.top,
                                 SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED | SWP_DEFERERASE | SWP_NOCOPYBITS | SWP_NOREDRAW));
        engine->window.fullscreened = false;
    }
    else if (set == 1 && engine->window.fullscreened != 1)
    {
        engine->window.fullscreened = BIT(31);
    }
    else if (set == 0 && engine->window.fullscreened != 0)
    {
        engine->window.fullscreened = BIT(30);
    }
}

//
// Sound
//

internal DWORD WINAPI SoundThreadProc(LPVOID arg)
{
    Engine *engine = cast(Engine *, arg);

    HANDLE event       = CreateEventExA(0, "EngineSoundEvent", 0, EVENT_ALL_ACCESS);
    engine->sound.error = engine->sound.client->lpVtbl->SetEventHandle(engine->sound.client, event);
    Check(SUCCEEDED(engine->sound.error));

    u32 buffer_size = 0;
    engine->sound.error = engine->sound.client->lpVtbl->GetBufferSize(engine->sound.client, &buffer_size);
    Check(SUCCEEDED(engine->sound.error));

    SoundBuffer buffer        = {0};
    buffer.samples_per_second = engine->sound.wave_format.Format.nSamplesPerSec;
    buffer.channels_count     = engine->sound.wave_format.Format.nChannels;

    engine->sound.error = engine->sound.client->lpVtbl->Start(engine->sound.client);
    Check(SUCCEEDED(engine->sound.error));

    while (true)
    {
        while (WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0)
        {
        }

        if (!engine->sound.pause)
        {
            u32 padding = 0;
            engine->sound.error = engine->sound.client->lpVtbl->GetCurrentPadding(engine->sound.client, &padding);
            Check(SUCCEEDED(engine->sound.error));

            buffer.samples_count = buffer_size - padding;
            buffer.samples       = 0;

            engine->sound.error = engine->sound.renderer->lpVtbl->GetBuffer(engine->sound.renderer, buffer.samples_count, cast(u8 **, &buffer.samples));
            Check(SUCCEEDED(engine->sound.error));

            engine->user_callbacks.OnSound(engine, &buffer);

            engine->sound.error = engine->sound.renderer->lpVtbl->ReleaseBuffer(engine->sound.renderer, buffer.samples_count, 0);
            Check(SUCCEEDED(engine->sound.error));
        }
    }

    return 0;
}

internal void CreateSound(Engine *engine)
{
    engine->sound.error = CoInitializeEx(0, COINIT_MULTITHREADED);
    Check(SUCCEEDED(engine->sound.error));

    IMMDeviceEnumerator *mmdevice_enumerator = 0;
    engine->sound.error = CoCreateInstance(&CLSID_MMDeviceEnumerator, 0, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, &mmdevice_enumerator);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = mmdevice_enumerator->lpVtbl->GetDefaultAudioEndpoint(mmdevice_enumerator, eRender, eConsole, &engine->sound.device);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = engine->sound.device->lpVtbl->Activate(engine->sound.device, &IID_IAudioClient, CLSCTX_INPROC_SERVER | CLSCTX_ACTIVATE_64_BIT_SERVER, 0, &engine->sound.client);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.wave_format.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    engine->sound.wave_format.Format.nChannels            = 2;
    engine->sound.wave_format.Format.nSamplesPerSec       = 48000;
    engine->sound.wave_format.Format.wBitsPerSample       = 32;
    engine->sound.wave_format.Format.nBlockAlign          = engine->sound.wave_format.Format.nChannels * engine->sound.wave_format.Format.wBitsPerSample / 8;
    engine->sound.wave_format.Format.nAvgBytesPerSec      = engine->sound.wave_format.Format.nSamplesPerSec * engine->sound.wave_format.Format.nBlockAlign;
    engine->sound.wave_format.Format.cbSize               = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    engine->sound.wave_format.Samples.wValidBitsPerSample = engine->sound.wave_format.Format.wBitsPerSample;
    engine->sound.wave_format.dwChannelMask               = KSAUDIO_SPEAKER_STEREO;
    engine->sound.wave_format.SubFormat                   = (GUID){ STATIC_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT };

    DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK
                       | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
                       | AUDCLNT_STREAMFLAGS_RATEADJUST;

    engine->sound.error = engine->sound.client->lpVtbl->Initialize(engine->sound.client, AUDCLNT_SHAREMODE_SHARED, stream_flags, 0, 0, cast(WAVEFORMATEX *, &engine->sound.wave_format), 0);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = engine->sound.client->lpVtbl->GetService(engine->sound.client, &IID_IAudioRenderClient, &engine->sound.renderer);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = engine->sound.client->lpVtbl->GetService(engine->sound.client, &IID_ISimpleAudioVolume, &engine->sound.volume);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.pause = true;

    DebugResult(CloseHandle(CreateThread(0, 0, SoundThreadProc, engine, 0, 0)));

    mmdevice_enumerator->lpVtbl->Release(mmdevice_enumerator);

    engine->sound.error = MFStartup(MF_VERSION, MFSTARTUP_LITE);
    Check(SUCCEEDED(engine->sound.error));
    
    Success(&engine->logger, "Sound was initialized");
}

internal void DestroySound(Engine *engine)
{
    engine->sound.pause = true;

    engine->sound.error = engine->sound.client->lpVtbl->Stop(engine->sound.client);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.volume->lpVtbl->Release(engine->sound.volume);
    engine->sound.renderer->lpVtbl->Release(engine->sound.renderer);
    engine->sound.client->lpVtbl->Release(engine->sound.client);
    engine->sound.device->lpVtbl->Release(engine->sound.device);
    
    Success(&engine->logger, "Sound was destroyed");
}

//
// Main
//

internal LRESULT WINAPI WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;
    Engine *engine = cast(Engine *, GetWindowLongPtrA(window, GWLP_USERDATA));

    switch (message)
    {
        case WM_DESTROY: // 0x0002
        {
            engine->window.closed = true;
        } break;

        case WM_SIZE: // 0x0005
        {
            v2s saved_size      = engine->window.size;
            engine->window.size = v2s_1(cast(s32, lparam & 0xFFFF), cast(s32, lparam >> 16));
            if ((engine->window.size.x != saved_size.x || engine->window.size.y != saved_size.y) && wparam != SIZE_MINIMIZED)
            {
                ResizeBuffers(engine);
                engine->window.resized = true;
                Log(&engine->logger, "Window was resized");
            }
        } break;

        case WM_DISPLAYCHANGE: // 0x007E
        {
            // CloseHandle(engine->monitor.handle);
            engine->monitor.handle = MonitorFromWindow(engine->window.handle, MONITOR_DEFAULTTONEAREST);
            Check(engine->monitor.handle);

            MONITORINFO info;
            info.cbSize = sizeof(MONITORINFO);
            DebugResult(GetMonitorInfoA(engine->monitor.handle, &info));

            engine->monitor.pos  = v2s_1(info.rcMonitor.left, info.rcMonitor.top);
            engine->monitor.size = v2s_1(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);

            Log(&engine->logger, "Monitor was changed");
        } break;

        case WM_INPUT: // 0x00FF
        {
            HRAWINPUT raw_input_handle = cast(HRAWINPUT, lparam);
            UINT size = 0;
            GetRawInputData(raw_input_handle, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER));

            RAWINPUT *ri = cast(RAWINPUT *, PushToTransientArea(engine->memory, size));
            if (GetRawInputData(raw_input_handle, RID_INPUT, ri, &size, sizeof(RAWINPUTHEADER)) == size
            &&  ri->header.dwType == RIM_TYPEMOUSE)
            {
                engine->input.mouse.dpos.x =  ri->data.mouse.lLastX;
                engine->input.mouse.dpos.y = -ri->data.mouse.lLastY;

                engine->input.mouse.pos.x += engine->input.mouse.dpos.x;
                engine->input.mouse.pos.y += engine->input.mouse.dpos.y;

                if (ri->data.mouse.ulButtons & RI_MOUSE_WHEEL)
                {
                    engine->input.mouse.dwheel  = cast(s16, ri->data.mouse.usButtonData) / WHEEL_DELTA;
                    engine->input.mouse.wheel  += engine->input.mouse.dwheel;
                }

                b32 down = engine->input.mouse.left_button.down;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) down = true;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP  ) down = false;
                DigitalButtonPull(&engine->input.mouse.left_button, down);

                down = engine->input.mouse.right_button.down;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) down = true;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP  ) down = false;
                DigitalButtonPull(&engine->input.mouse.right_button, down);

                down = engine->input.mouse.middle_button.down;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) down = true;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP  ) down = false;
                DigitalButtonPull(&engine->input.mouse.middle_button, down);

                down = engine->input.mouse.x1_button.down;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) down = true;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP  ) down = false;
                DigitalButtonPull(&engine->input.mouse.x1_button, down);

                down = engine->input.mouse.x2_button.down;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) down = true;
                if (ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP  ) down = false;
                DigitalButtonPull(&engine->input.mouse.x2_button, down);
            }

            result = DefWindowProcA(window, message, wparam, lparam);
        } break;

        case WM_MENUCHAR: // 0x0100
        {
            // @NOTE(Roman): suppress bip sound on alt+enter
            result = MNC_CLOSE << 16;
        } break;

        default:
        {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }

    return result;
}

int EngineRun(
    OPTIONAL HINSTANCE      instance,
    IN       UserCallback  *OnInit,
    IN       UserCallback  *OnDestroy,
    IN       UserCallback  *OnUpdate,
    IN       UserCallback  *OnRender,
    IN       SoundCallback *OnSound)
{
    DebugResult(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST));

    Memory engine_memory;
    CreateMemory(&engine_memory, GB(1ui64), GB(2ui64));

    Engine *engine = PushToPA(Engine, &engine_memory, 1);
    engine->memory = &engine_memory;

    DebugResult(engine->user_callbacks.OnInit    = OnInit);
    DebugResult(engine->user_callbacks.OnDestroy = OnDestroy);
    DebugResult(engine->user_callbacks.OnUpdate  = OnUpdate);
    DebugResult(engine->user_callbacks.OnRender  = OnRender);
    DebugResult(engine->user_callbacks.OnSound   = OnSound);

    u64 allocator_cap  = GB(1ui64);
    void *base_address = PushToPermanentArea(engine->memory, allocator_cap);
    CreateAllocator(&engine->allocator, base_address, allocator_cap, false);

    CreateLogger(&engine->logger, "Engine logger", "../log/cengine.log", LOG_TO_FILE);
    WindowCreate(engine, "CEngine", v2s_1(960, 540), instance);
    InputCreate(engine);
    TimerCreate(engine);
    CreateCoreRenderer(engine);
    CreateSound(engine);
    // engine->queue = CreateWorkQueue(engine);

    engine->user_callbacks.OnInit(engine);

    TimerStart(engine);

    D3D12_RESOURCE_BARRIER rtv_barrier;
    rtv_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    rtv_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    rtv_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    D3D12_RESOURCE_BARRIER dsv_barrier;
    dsv_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    dsv_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    dsv_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    MSG msg = {0};
    ShowWindow(engine->window.handle, SW_SHOW);
    while (!engine->window.closed)
    {
        // Reset
    #if DEBUG
        gAllocationsPerFrame   = 0;
        gReAllocationsPerFrame = 0;
        gDeAllocationsPerFrame = 0;
    #endif
        ResetTransientArea(engine->memory);
        engine->window.resized = false;
        InputReset(engine);

        while (PeekMessageA(&msg, engine->window.handle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (!engine->window.closed)
        {
            // Reinitialize all stuff was allocated on transient memory
            // ...

            // Pull
            InputPull(engine);
            TimerPull(engine);

            // Delayed SetFullscreen if we need to.
            SetFullscreen(engine, engine->window.fullscreened);

            // Frame start
            WaitForGPU(engine);

            ID3D12CommandAllocator    *graphics_allocator = engine->core_renderer.graphics_allocators[engine->core_renderer.current_buffer];
            ID3D12GraphicsCommandList *graphics_list      = engine->core_renderer.graphics_lists[engine->core_renderer.current_buffer];
            ID3D12CommandAllocator    *compute_allocator  = engine->core_renderer.compute_allocators[engine->core_renderer.current_buffer];
            ID3D12GraphicsCommandList *compute_list       = engine->core_renderer.compute_lists[engine->core_renderer.current_buffer];

            engine->core_renderer.error = graphics_allocator->lpVtbl->Reset(graphics_allocator);
            Check(SUCCEEDED(engine->core_renderer.error));
            engine->core_renderer.error = graphics_list->lpVtbl->Reset(graphics_list, graphics_allocator, 0);
            Check(SUCCEEDED(engine->core_renderer.error));

            engine->core_renderer.error = compute_allocator->lpVtbl->Reset(compute_allocator);
            Check(SUCCEEDED(engine->core_renderer.error));
            engine->core_renderer.error = compute_list->lpVtbl->Reset(compute_list, compute_allocator, 0);
            Check(SUCCEEDED(engine->core_renderer.error));

            // Set viewport
            D3D12_VIEWPORT viewport;
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width    = cast(f32, engine->window.size.w);
            viewport.Height   = cast(f32, engine->window.size.h);
            viewport.MinDepth = -1.0f;
            viewport.MaxDepth =  1.0f;

            graphics_list->lpVtbl->RSSetViewports(graphics_list, 1, &viewport);

            // Set scissor rect (optimization)
            D3D12_RECT scissor_rect;
            scissor_rect.left   = 0;
            scissor_rect.top    = 0;
            scissor_rect.right  = engine->window.size.w;
            scissor_rect.bottom = engine->window.size.h;

            graphics_list->lpVtbl->RSSetScissorRects(graphics_list, 1, &scissor_rect);

            // Set RTV And DSV
            graphics_list->lpVtbl->OMSetRenderTargets(graphics_list,
                                                      1,
                                                      &engine->core_renderer.rtv_cpu_desc_handle,
                                                      false,
                                                      &engine->core_renderer.dsv_cpu_desc_handle);

            // Set Resource Barrier
            rtv_barrier.Transition.pResource   = engine->core_renderer.rt_buffers[engine->core_renderer.current_buffer];
            rtv_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
            rtv_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

            graphics_list->lpVtbl->ResourceBarrier(graphics_list, 1, &rtv_barrier);

            if (!engine->core_renderer.first_frame)
            {
                dsv_barrier.Transition.pResource   = engine->core_renderer.ds_buffer;
                dsv_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                dsv_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;

                graphics_list->lpVtbl->ResourceBarrier(graphics_list, 1, &dsv_barrier);
            }

            // Update
            engine->user_callbacks.OnUpdate(engine);

            // Clear
            f32 clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            graphics_list->lpVtbl->ClearRenderTargetView(graphics_list,
                                                         engine->core_renderer.rtv_cpu_desc_handle,
                                                         clear_color,
                                                         0, 0);
            graphics_list->lpVtbl->ClearDepthStencilView(graphics_list,
                                                         engine->core_renderer.dsv_cpu_desc_handle,
                                                         D3D12_CLEAR_FLAG_DEPTH,
                                                         1.0f, 0,
                                                         0, 0);

            // Render
            engine->user_callbacks.OnRender(engine);

            dsv_barrier.Transition.pResource   = engine->core_renderer.ds_buffer;
            dsv_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
            dsv_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

            graphics_list->lpVtbl->ResourceBarrier(graphics_list, 1, &dsv_barrier);

            // Present
            Present(engine);
        }
    }

    TimerStop(engine);
    DestroySound(engine);
    FlushGPU(engine);

    engine->user_callbacks.OnDestroy(engine);

    DestroyCoreRenderer(engine);
    InputDestroy(engine);
    WindowDestroy(engine, instance);
    DestroyLogger(&engine->logger);
    DestroyAllocator(&engine->allocator);
    DestroyMemory(engine->memory);

    return 0;
}

#pragma warning(pop)
