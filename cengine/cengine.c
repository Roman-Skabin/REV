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
// GPUManager
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
         engine->gpu_manager.factory->lpVtbl->EnumAdapters1(engine->gpu_manager.factory, i, &test_adapter) != DXGI_ERROR_NOT_FOUND;
         ++i)
    {
        engine->gpu_manager.error = D3D12CreateDevice(test_adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, &test_device);
        if (SUCCEEDED(engine->gpu_manager.error))
        {
            DXGI_ADAPTER_DESC1 ad1;
            engine->gpu_manager.error = test_adapter->lpVtbl->GetDesc1(test_adapter, &ad1);
            if (SUCCEEDED(engine->gpu_manager.error) && max_vram < ad1.DedicatedVideoMemory)
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
        engine->gpu_manager.error = engine->gpu_manager.factory->lpVtbl->EnumAdapters1(engine->gpu_manager.factory, adapter_index, &engine->gpu_manager.adapter);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = D3D12CreateDevice(engine->gpu_manager.adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, &engine->gpu_manager.device);
        Check(SUCCEEDED(engine->gpu_manager.error));
    }
    else
    {
        for (UINT i = 0;
             engine->gpu_manager.factory->lpVtbl->EnumAdapters1(engine->gpu_manager.factory, i, &test_adapter) != D3D12_ERROR_ADAPTER_NOT_FOUND;
             ++i)
        {
            engine->gpu_manager.error = D3D12CreateDevice(test_adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &test_device);
            if (SUCCEEDED(engine->gpu_manager.error))
            {
                DXGI_ADAPTER_DESC1 ad1;
                engine->gpu_manager.error = test_adapter->lpVtbl->GetDesc1(test_adapter, &ad1);
                if (SUCCEEDED(engine->gpu_manager.error) && max_vram < ad1.DedicatedVideoMemory)
                {
                    max_vram      = ad1.DedicatedVideoMemory;
                    adapter_index = i;
                }

                SafeRelease(test_device);
            }

            SafeRelease(test_adapter);
        }

        CheckM(max_vram, "Direct3D 12 is not supported by your hardware");

        engine->gpu_manager.error = engine->gpu_manager.factory->lpVtbl->EnumAdapters1(engine->gpu_manager.factory, adapter_index, &engine->gpu_manager.adapter);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = D3D12CreateDevice(engine->gpu_manager.adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &engine->gpu_manager.device);
        Check(SUCCEEDED(engine->gpu_manager.error));
    }
}

internal void CreateGPUManager(Engine *engine)
{
#if DEBUG
    engine->gpu_manager.error = D3D12GetDebugInterface(&IID_ID3D12Debug, &engine->gpu_manager.debug);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.debug->lpVtbl->EnableDebugLayer(engine->gpu_manager.debug);

    engine->gpu_manager.error = DXGIGetDebugInterface1(0, &IID_IDXGIDebug1, &engine->gpu_manager.dxgi_debug);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.dxgi_debug->lpVtbl->EnableLeakTrackingForThread(engine->gpu_manager.dxgi_debug);
#endif

    UINT factory_create_flags = 0;
#if DEBUG
    factory_create_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    engine->gpu_manager.error = CreateDXGIFactory2(factory_create_flags, &IID_IDXGIFactory, &engine->gpu_manager.factory);
    Check(SUCCEEDED(engine->gpu_manager.error));

    CreateAdapterAndDevice(engine);

    D3D12_COMMAND_QUEUE_DESC graphics_queue_desc;
    graphics_queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    graphics_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME ?
    graphics_queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    graphics_queue_desc.NodeMask = 1;

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandQueue(engine->gpu_manager.device,
                                                                                       &graphics_queue_desc,
                                                                                       &IID_ID3D12CommandQueue,
                                                                                       &engine->gpu_manager.graphics_queue);
    Check(SUCCEEDED(engine->gpu_manager.error));

    D3D12_COMMAND_QUEUE_DESC compute_queue_desc;
    compute_queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    compute_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME ?
    compute_queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    compute_queue_desc.NodeMask = 1; // @TODO(Roman): Set to 2 if we have 2 adapters

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandQueue(engine->gpu_manager.device,
                                                                                       &compute_queue_desc,
                                                                                       &IID_ID3D12CommandQueue,
                                                                                       &engine->gpu_manager.compute_queue);
    Check(SUCCEEDED(engine->gpu_manager.error));

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandAllocator(engine->gpu_manager.device,
                                                                                               graphics_queue_desc.Type,
                                                                                               &IID_ID3D12CommandAllocator,
                                                                                               engine->gpu_manager.graphics_allocators + i);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandList(engine->gpu_manager.device,
                                                                                          graphics_queue_desc.NodeMask,
                                                                                          graphics_queue_desc.Type,
                                                                                          engine->gpu_manager.graphics_allocators[i],
                                                                                          0,
                                                                                          &IID_ID3D12GraphicsCommandList,
                                                                                          engine->gpu_manager.graphics_lists + i);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = engine->gpu_manager.graphics_lists[i]->lpVtbl->Close(engine->gpu_manager.graphics_lists[i]);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandAllocator(engine->gpu_manager.device,
                                                                                               compute_queue_desc.Type,
                                                                                               &IID_ID3D12CommandAllocator,
                                                                                               engine->gpu_manager.compute_allocators + i);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandList(engine->gpu_manager.device,
                                                                                          compute_queue_desc.NodeMask,
                                                                                          compute_queue_desc.Type,
                                                                                          engine->gpu_manager.compute_allocators[i],
                                                                                          0,
                                                                                          &IID_ID3D12GraphicsCommandList,
                                                                                          engine->gpu_manager.compute_lists + i);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = engine->gpu_manager.compute_lists[i]->lpVtbl->Close(engine->gpu_manager.compute_lists[i]);
        Check(SUCCEEDED(engine->gpu_manager.error));
    }

    IDXGIFactory5 *factory5 = 0;
    engine->gpu_manager.error = engine->gpu_manager.factory->lpVtbl->QueryInterface(engine->gpu_manager.factory,
                                                                                    &IID_IDXGIFactory5,
                                                                                    &factory5);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = factory5->lpVtbl->CheckFeatureSupport(factory5,
                                                                      DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                                      &engine->gpu_manager.tearing_supported,
                                                                      sizeof(b32));
    Check(SUCCEEDED(engine->gpu_manager.error));

    SafeRelease(factory5);

    DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1;
    swap_chain_desc1.Width              = engine->window.size.w;
    swap_chain_desc1.Height             = engine->window.size.h;
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
    if (engine->gpu_manager.tearing_supported)
    {
        swap_chain_desc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swap_chain_fullscreen_desc;
    swap_chain_fullscreen_desc.RefreshRate.Numerator   = 0;
    swap_chain_fullscreen_desc.RefreshRate.Denominator = 1;
    swap_chain_fullscreen_desc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_fullscreen_desc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
    swap_chain_fullscreen_desc.Windowed                = true;

    IDXGISwapChain1 *swap_chain1 = 0;
    engine->gpu_manager.error = engine->gpu_manager.factory->lpVtbl->CreateSwapChainForHwnd(engine->gpu_manager.factory,
                                                                                            engine->gpu_manager.graphics_queue,
                                                                                            engine->window.handle,
                                                                                            &swap_chain_desc1,
                                                                                            &swap_chain_fullscreen_desc,
                                                                                            0,
                                                                                            &swap_chain1);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = swap_chain1->lpVtbl->QueryInterface(swap_chain1,
                                                                    &IID_IDXGISwapChain4,
                                                                    &engine->gpu_manager.swap_chain);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = engine->gpu_manager.factory->lpVtbl->MakeWindowAssociation(engine->gpu_manager.factory,
                                                                                           engine->window.handle,
                                                                                           DXGI_MWA_NO_ALT_ENTER);
    Check(SUCCEEDED(engine->gpu_manager.error));

    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc;
    rtv_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_desc_heap_desc.NumDescriptors = SWAP_CHAIN_BUFFERS_COUNT;
    rtv_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_desc_heap_desc.NodeMask       = 1;

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateDescriptorHeap(engine->gpu_manager.device,
                                                                                         &rtv_desc_heap_desc,
                                                                                         &IID_ID3D12DescriptorHeap,
                                                                                         &engine->gpu_manager.rtv_heap_desc);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.rtv_desc_size = engine->gpu_manager.device->lpVtbl->GetDescriptorHandleIncrementSize(engine->gpu_manager.device,
                                                                                                             rtv_desc_heap_desc.Type);

    #pragma warning(suppress: 4020) // I know what I'm doing.
    engine->gpu_manager.rtv_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->gpu_manager.rtv_heap_desc, &engine->gpu_manager.rtv_cpu_desc_handle);

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->gpu_manager.error = engine->gpu_manager.swap_chain->lpVtbl->GetBuffer(engine->gpu_manager.swap_chain,
                                                                                      i,
                                                                                      &IID_ID3D12Resource,
                                                                                      engine->gpu_manager.rt_buffers + i);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.device->lpVtbl->CreateRenderTargetView(engine->gpu_manager.device,
                                                                   engine->gpu_manager.rt_buffers[i],
                                                                   0,
                                                                   engine->gpu_manager.rtv_cpu_desc_handle);

        engine->gpu_manager.rtv_cpu_desc_handle.ptr += engine->gpu_manager.rtv_desc_size;
    }

    engine->gpu_manager.rtv_cpu_desc_handle.ptr -= SWAP_CHAIN_BUFFERS_COUNT * engine->gpu_manager.rtv_desc_size;
    engine->gpu_manager.current_buffer           = engine->gpu_manager.swap_chain->lpVtbl->GetCurrentBackBufferIndex(engine->gpu_manager.swap_chain);

    D3D12_DESCRIPTOR_HEAP_DESC dsv_desc_heap_desc;
    dsv_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv_desc_heap_desc.NumDescriptors = 1;
    dsv_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsv_desc_heap_desc.NodeMask       = 1;

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateDescriptorHeap(engine->gpu_manager.device,
                                                                                         &dsv_desc_heap_desc,
                                                                                         &IID_ID3D12DescriptorHeap,
                                                                                         &engine->gpu_manager.ds_heap_desc);
    Check(SUCCEEDED(engine->gpu_manager.error));

    D3D12_HEAP_PROPERTIES ds_heap_properties;
    ds_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    ds_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    ds_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    ds_heap_properties.CreationNodeMask     = 1;
    ds_heap_properties.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC ds_resource_desc;
    ds_resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ds_resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ds_resource_desc.Width              = engine->window.size.w;
    ds_resource_desc.Height             = engine->window.size.h;
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

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommittedResource(engine->gpu_manager.device,
                                                                                            &ds_heap_properties,
                                                                                            D3D12_HEAP_FLAG_NONE,
                                                                                            &ds_resource_desc,
                                                                                            D3D12_RESOURCE_STATE_PRESENT,
                                                                                            &ds_clear_value,
                                                                                            &IID_ID3D12Resource,
                                                                                            &engine->gpu_manager.ds_buffer);
    Check(SUCCEEDED(engine->gpu_manager.error));

    #pragma warning(suppress: 4020) // I know what I'm doing.
    engine->gpu_manager.ds_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->gpu_manager.ds_heap_desc, &engine->gpu_manager.dsv_cpu_desc_handle);

    D3D12_DEPTH_STENCIL_VIEW_DESC ds_view_desc = {0};
    ds_view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
    ds_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    engine->gpu_manager.device->lpVtbl->CreateDepthStencilView(engine->gpu_manager.device,
                                                               engine->gpu_manager.ds_buffer,
                                                               &ds_view_desc,
                                                               engine->gpu_manager.dsv_cpu_desc_handle);

    for (u64 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateFence(engine->gpu_manager.device,
                                                                                    engine->gpu_manager.graphics_fences_values[i],
                                                                                    D3D12_FENCE_FLAG_SHARED,
                                                                                    &IID_ID3D12Fence,
                                                                                    engine->gpu_manager.graphics_fences + i);
        Check(SUCCEEDED(engine->gpu_manager.error));
        ++engine->gpu_manager.graphics_fences_values[i];

        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateFence(engine->gpu_manager.device,
                                                                                    engine->gpu_manager.compute_fences_values[i],
                                                                                    D3D12_FENCE_FLAG_SHARED,
                                                                                    &IID_ID3D12Fence,
                                                                                    engine->gpu_manager.compute_fences + i);
        Check(SUCCEEDED(engine->gpu_manager.error));
        ++engine->gpu_manager.graphics_fences_values[i];
    }

    engine->gpu_manager.graphics_fence_event = CreateEventExA(0, "Graphics Fence Event", 0, EVENT_ALL_ACCESS);
    Check(engine->gpu_manager.graphics_fence_event);

    engine->gpu_manager.compute_fence_event = CreateEventExA(0, "Compute Fence Event", 0, EVENT_ALL_ACCESS);
    Check(engine->gpu_manager.compute_fence_event);

    engine->gpu_manager.first_frame = true;

    Success(&engine->logger, "Renderer was created");
}

internal WORK_QUEUE_ENTRY_PROC(WaitForGraphicsWork)
{
    Engine *engine = cast(Engine *, arg);

    ID3D12Fence *current_fence       = engine->gpu_manager.graphics_fences[engine->gpu_manager.current_buffer];
    u64          current_fence_value = engine->gpu_manager.graphics_fences_values[engine->gpu_manager.current_buffer];

    engine->gpu_manager.graphics_queue->lpVtbl->Signal(engine->gpu_manager.graphics_queue,
                                                       current_fence,
                                                       current_fence_value);

    u64 value = current_fence->lpVtbl->GetCompletedValue(current_fence);
    if (value < current_fence_value)
    {
        engine->gpu_manager.error = current_fence->lpVtbl->SetEventOnCompletion(current_fence,
                                                                                current_fence_value,
                                                                                engine->gpu_manager.graphics_fence_event);
        Check(SUCCEEDED(engine->gpu_manager.error));
        while (WaitForSingleObjectEx(engine->gpu_manager.graphics_fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }
    }

    engine->gpu_manager.graphics_fences_values[engine->gpu_manager.current_buffer] = current_fence_value + 1;
}

internal WORK_QUEUE_ENTRY_PROC(WaitForComputeWork)
{
    Engine *engine = cast(Engine *, arg);

    ID3D12Fence *current_fence       = engine->gpu_manager.compute_fences[engine->gpu_manager.current_buffer];
    u64          current_fence_value = engine->gpu_manager.compute_fences_values[engine->gpu_manager.current_buffer];

    engine->gpu_manager.compute_queue->lpVtbl->Signal(engine->gpu_manager.compute_queue,
                                                      current_fence,
                                                      current_fence_value);

    u64 value = current_fence->lpVtbl->GetCompletedValue(current_fence);
    if (value < current_fence_value)
    {
        engine->gpu_manager.error = current_fence->lpVtbl->SetEventOnCompletion(current_fence,
                                                                                current_fence_value,
                                                                                engine->gpu_manager.compute_fence_event);
        Check(SUCCEEDED(engine->gpu_manager.error));
        while (WaitForSingleObjectEx(engine->gpu_manager.compute_fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }
    }

    engine->gpu_manager.compute_fences_values[engine->gpu_manager.current_buffer] = current_fence_value + 1;
}

internal INLINE void WaitForGPU(Engine *engine)
{
    // @TODO(Roman): Wait in several work threads
    WaitForGraphicsWork(0, engine);
    WaitForComputeWork(0, engine);
}

internal WORK_QUEUE_ENTRY_PROC(FlushGraphicsWork)
{
    Engine *engine = cast(Engine *, arg);

    for (u32 buffer_index = 0; buffer_index < SWAP_CHAIN_BUFFERS_COUNT; ++buffer_index)
    {
        ID3D12Fence *fence       = engine->gpu_manager.graphics_fences[buffer_index];
        u64          fence_value = engine->gpu_manager.graphics_fences_values[buffer_index];

        engine->gpu_manager.graphics_queue->lpVtbl->Signal(engine->gpu_manager.graphics_queue,
                                                           fence,
                                                           fence_value);

        engine->gpu_manager.error = fence->lpVtbl->SetEventOnCompletion(fence,
                                                                        fence_value,
                                                                        engine->gpu_manager.graphics_fence_event);
        Check(SUCCEEDED(engine->gpu_manager.error));
        while (WaitForSingleObjectEx(engine->gpu_manager.graphics_fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }

        ++engine->gpu_manager.graphics_fences_values[buffer_index];
    }
}

internal WORK_QUEUE_ENTRY_PROC(FlushComputeWork)
{
    Engine *engine = cast(Engine *, arg);

    for (u32 buffer_index = 0; buffer_index < SWAP_CHAIN_BUFFERS_COUNT; ++buffer_index)
    {
        ID3D12Fence *fence       = engine->gpu_manager.compute_fences[buffer_index];
        u64          fence_value = engine->gpu_manager.compute_fences_values[buffer_index];

        engine->gpu_manager.compute_queue->lpVtbl->Signal(engine->gpu_manager.compute_queue,
                                                          fence,
                                                          fence_value);

        engine->gpu_manager.error = fence->lpVtbl->SetEventOnCompletion(fence,
                                                                        fence_value,
                                                                        engine->gpu_manager.compute_fence_event);
        Check(SUCCEEDED(engine->gpu_manager.error));
        while (WaitForSingleObjectEx(engine->gpu_manager.compute_fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }

        ++engine->gpu_manager.compute_fences_values[buffer_index];
    }
}

internal INLINE void FlushGPU(Engine *engine)
{
    // @TODO(Roman): Flush in several work threads
    FlushGraphicsWork(0, engine);
    FlushComputeWork(0, engine);
}

internal void DestroyGPUManager(Engine *engine)
{
    SafeRelease(engine->gpu_manager.ds_buffer);
    SafeRelease(engine->gpu_manager.ds_heap_desc);
    DebugResult(CloseHandle(engine->gpu_manager.compute_fence_event));
    DebugResult(CloseHandle(engine->gpu_manager.graphics_fence_event));
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(engine->gpu_manager.compute_fences[i]);
        SafeRelease(engine->gpu_manager.graphics_fences[i]);
        SafeRelease(engine->gpu_manager.rt_buffers[i]);
        SafeRelease(engine->gpu_manager.compute_lists[i]);
        SafeRelease(engine->gpu_manager.compute_allocators[i]);
        SafeRelease(engine->gpu_manager.graphics_lists[i]);
        SafeRelease(engine->gpu_manager.graphics_allocators[i]);
    }
    SafeRelease(engine->gpu_manager.rtv_heap_desc);
    SafeRelease(engine->gpu_manager.swap_chain);
    SafeRelease(engine->gpu_manager.compute_queue);
    SafeRelease(engine->gpu_manager.graphics_queue);
    SafeRelease(engine->gpu_manager.device);
    SafeRelease(engine->gpu_manager.adapter);
    SafeRelease(engine->gpu_manager.factory);
#if DEBUG
    SafeRelease(engine->gpu_manager.debug_list);
    SafeRelease(engine->gpu_manager.debug_queue);
    SafeRelease(engine->gpu_manager.debug);

    engine->gpu_manager.error = engine->gpu_manager.dxgi_debug->lpVtbl->ReportLiveObjects(engine->gpu_manager.dxgi_debug,
                                                                                          DXGI_DEBUG_DX,
                                                                                          DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    Check(SUCCEEDED(engine->gpu_manager.error));
    engine->gpu_manager.error = engine->gpu_manager.dxgi_debug->lpVtbl->ReportLiveObjects(engine->gpu_manager.dxgi_debug,
                                                                                          DXGI_DEBUG_DXGI,
                                                                                          DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    Check(SUCCEEDED(engine->gpu_manager.error));
    engine->gpu_manager.dxgi_debug->lpVtbl->DisableLeakTrackingForThread(engine->gpu_manager.dxgi_debug);
    SafeRelease(engine->gpu_manager.dxgi_debug);
#endif
}

internal void ResizeGPUBuffers(Engine *engine)
{
    // Wait for the GPU, release swap chain buffers buffers
    FlushGPU(engine);
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(engine->gpu_manager.rt_buffers[i]);
    }
    SafeRelease(engine->gpu_manager.ds_buffer);

    // Resize buffers
    u32 flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if (engine->gpu_manager.tearing_supported)
    {
        flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }

    u32                 node_masks[]     = { 1, 1 };
    ID3D12CommandQueue *present_queues[] = { engine->gpu_manager.graphics_queue, engine->gpu_manager.graphics_queue };

    engine->gpu_manager.error = engine->gpu_manager.swap_chain->lpVtbl->ResizeBuffers1(engine->gpu_manager.swap_chain,
                                                                                       SWAP_CHAIN_BUFFERS_COUNT,
                                                                                       engine->window.size.w,
                                                                                       engine->window.size.h,
                                                                                       DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                                       flags,
                                                                                       node_masks,
                                                                                       present_queues);
    Check(SUCCEEDED(engine->gpu_manager.error));

    // Recreate buffers
    #pragma warning(suppress: 4020) // I know what I'm doing.
    engine->gpu_manager.rtv_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->gpu_manager.rtv_heap_desc, &engine->gpu_manager.rtv_cpu_desc_handle);

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->gpu_manager.error = engine->gpu_manager.swap_chain->lpVtbl->GetBuffer(engine->gpu_manager.swap_chain,
                                                                                      i,
                                                                                      &IID_ID3D12Resource,
                                                                                      engine->gpu_manager.rt_buffers + i);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.device->lpVtbl->CreateRenderTargetView(engine->gpu_manager.device,
                                                                   engine->gpu_manager.rt_buffers[i],
                                                                   0,
                                                                   engine->gpu_manager.rtv_cpu_desc_handle);

        engine->gpu_manager.rtv_cpu_desc_handle.ptr += engine->gpu_manager.rtv_desc_size;
    }

    engine->gpu_manager.rtv_cpu_desc_handle.ptr -= SWAP_CHAIN_BUFFERS_COUNT * engine->gpu_manager.rtv_desc_size;
    engine->gpu_manager.current_buffer           = engine->gpu_manager.swap_chain->lpVtbl->GetCurrentBackBufferIndex(engine->gpu_manager.swap_chain);
    engine->gpu_manager.rtv_cpu_desc_handle.ptr += engine->gpu_manager.current_buffer * engine->gpu_manager.rtv_desc_size;

    // Recreate DS buffer & DSV
    D3D12_HEAP_PROPERTIES ds_heap_properties;
    ds_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    ds_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    ds_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    ds_heap_properties.CreationNodeMask     = 1;
    ds_heap_properties.VisibleNodeMask      = 1;

    D3D12_RESOURCE_DESC ds_resource_desc;
    ds_resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ds_resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ds_resource_desc.Width              = engine->window.size.w;
    ds_resource_desc.Height             = engine->window.size.h;
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

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommittedResource(engine->gpu_manager.device,
                                                                                            &ds_heap_properties,
                                                                                            D3D12_HEAP_FLAG_NONE,
                                                                                            &ds_resource_desc,
                                                                                            D3D12_RESOURCE_STATE_PRESENT,
                                                                                            &ds_clear_value,
                                                                                            &IID_ID3D12Resource,
                                                                                            &engine->gpu_manager.ds_buffer);
    Check(SUCCEEDED(engine->gpu_manager.error));

    D3D12_DEPTH_STENCIL_VIEW_DESC ds_view_desc = {0};
    ds_view_desc.Format        = DXGI_FORMAT_D32_FLOAT;
    ds_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

    engine->gpu_manager.device->lpVtbl->CreateDepthStencilView(engine->gpu_manager.device,
                                                               engine->gpu_manager.ds_buffer,
                                                               &ds_view_desc,
                                                               engine->gpu_manager.dsv_cpu_desc_handle);

    engine->gpu_manager.first_frame = true;
}

internal void StartFrame(Engine *engine)
{
    ID3D12CommandAllocator    *graphics_allocator = engine->gpu_manager.graphics_allocators[engine->gpu_manager.current_buffer];
    ID3D12GraphicsCommandList *graphics_list      = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];
    ID3D12CommandAllocator    *compute_allocator  = engine->gpu_manager.compute_allocators[engine->gpu_manager.current_buffer];
    ID3D12GraphicsCommandList *compute_list       = engine->gpu_manager.compute_lists[engine->gpu_manager.current_buffer];

    // Reinitialize all stuff was allocated on transient memory
    // ...

    // Pull
    InputPull(engine);
    TimerPull(engine);

    // Delayed SetFullscreen if we need to.
    SetFullscreen(engine, engine->window.fullscreened);

    WaitForGPU(engine);
    ClearGPUMemory(engine, GPU_MEMORY_COPY);

    engine->gpu_manager.error = graphics_allocator->lpVtbl->Reset(graphics_allocator);
    Check(SUCCEEDED(engine->gpu_manager.error));
    engine->gpu_manager.error = graphics_list->lpVtbl->Reset(graphics_list, graphics_allocator, 0);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = compute_allocator->lpVtbl->Reset(compute_allocator);
    Check(SUCCEEDED(engine->gpu_manager.error));
    engine->gpu_manager.error = compute_list->lpVtbl->Reset(compute_list, compute_allocator, 0);
    Check(SUCCEEDED(engine->gpu_manager.error));

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
                                              &engine->gpu_manager.rtv_cpu_desc_handle,
                                              false,
                                              &engine->gpu_manager.dsv_cpu_desc_handle);

    // Set Resource Barriers
    D3D12_RESOURCE_BARRIER rt_barrier;
    rt_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    rt_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    rt_barrier.Transition.pResource   = engine->gpu_manager.rt_buffers[engine->gpu_manager.current_buffer];
    rt_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rt_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    rt_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

    D3D12_RESOURCE_BARRIER ds_barrier;
    ds_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ds_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    ds_barrier.Transition.pResource   = engine->gpu_manager.ds_buffer;
    ds_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ds_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    ds_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    D3D12_RESOURCE_BARRIER begin_barriers[] = { rt_barrier, ds_barrier };
    graphics_list->lpVtbl->ResourceBarrier(graphics_list, ArrayCount(begin_barriers), begin_barriers);

    // Clear
    f32 clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    graphics_list->lpVtbl->ClearRenderTargetView(graphics_list,
                                                 engine->gpu_manager.rtv_cpu_desc_handle,
                                                 clear_color,
                                                 0, 0);
    graphics_list->lpVtbl->ClearDepthStencilView(graphics_list,
                                                 engine->gpu_manager.dsv_cpu_desc_handle,
                                                 D3D12_CLEAR_FLAG_DEPTH,
                                                 1.0f, 0,
                                                 0, 0);
}

// @TODO(Roman): EndFrame in several work threads
internal void EndFrame(Engine *engine)
{
    ID3D12GraphicsCommandList *compute_list  = engine->gpu_manager.compute_lists[engine->gpu_manager.current_buffer];
    ID3D12GraphicsCommandList *graphics_list = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    // Set Resource Barriers
    D3D12_RESOURCE_BARRIER rt_barrier;
    rt_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    rt_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    rt_barrier.Transition.pResource   = engine->gpu_manager.rt_buffers[engine->gpu_manager.current_buffer];
    rt_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    rt_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    rt_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

    D3D12_RESOURCE_BARRIER ds_barrier;
    ds_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ds_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    ds_barrier.Transition.pResource   = engine->gpu_manager.ds_buffer;
    ds_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    ds_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    ds_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;

    D3D12_RESOURCE_BARRIER end_barriers[] = { rt_barrier, ds_barrier };
    graphics_list->lpVtbl->ResourceBarrier(graphics_list, ArrayCount(end_barriers), end_barriers);

    // Close and execute lists
    engine->gpu_manager.error = compute_list->lpVtbl->Close(compute_list);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = graphics_list->lpVtbl->Close(graphics_list);
    Check(SUCCEEDED(engine->gpu_manager.error));

    ID3D12CommandList *compute_lists[] = { compute_list };
    engine->gpu_manager.compute_queue->lpVtbl->ExecuteCommandLists(engine->gpu_manager.compute_queue,
                                                                   ArrayCount(compute_lists),
                                                                   compute_lists);
    Check(SUCCEEDED(engine->gpu_manager.error));

    ID3D12CommandList *graphics_lists[] = { graphics_list };
    engine->gpu_manager.graphics_queue->lpVtbl->ExecuteCommandLists(engine->gpu_manager.graphics_queue,
                                                                    ArrayCount(graphics_lists),
                                                                    graphics_lists);
    Check(SUCCEEDED(engine->gpu_manager.error));

    // Present
    u32 present_flags = 0;
    if (!engine->gpu_manager.vsync)
    {
        present_flags |= DXGI_PRESENT_DO_NOT_WAIT;
        if (engine->gpu_manager.tearing_supported)
        {
            present_flags |= DXGI_PRESENT_ALLOW_TEARING;
        }
    }
    if (engine->gpu_manager.first_frame)
    {
        engine->gpu_manager.first_frame = false;
    }
    else
    {
        present_flags |= DXGI_PRESENT_DO_NOT_SEQUENCE;
    }

    engine->gpu_manager.error = engine->gpu_manager.swap_chain->lpVtbl->Present(engine->gpu_manager.swap_chain,
                                                                                engine->gpu_manager.vsync,
                                                                                present_flags);
    Check(SUCCEEDED(engine->gpu_manager.error));

    // Swap buffers
    engine->gpu_manager.current_buffer = engine->gpu_manager.swap_chain->lpVtbl->GetCurrentBackBufferIndex(engine->gpu_manager.swap_chain);

    #pragma warning(suppress: 4020) // I know what I'm doing.
    engine->gpu_manager.rtv_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->gpu_manager.rtv_heap_desc, &engine->gpu_manager.rtv_cpu_desc_handle);
    engine->gpu_manager.rtv_cpu_desc_handle.ptr += engine->gpu_manager.current_buffer * engine->gpu_manager.rtv_desc_size;
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
// GPUMemoryManager
//

internal void CreateGPUMemoryManager(
    Engine *engine,
    u64     vb_memory_capacity,
    u64     ib_memory_capacity,
    u64     cbv_memory_capacity,
    u64     srv_memory_capacity,
    u64     uav_memory_capacity,
    u64     sampler_memory_capacity,
    u64     copy_memory_capacity)
{
    Check(engine);
    CheckM(vb_memory_capacity, "VB memory capacity can't be 0");
    CheckM(ib_memory_capacity, "IB memory capacity can't be 0");
    CheckM(cbv_memory_capacity, "CBV memory capacity can't be 0");
    CheckM(srv_memory_capacity, "SRV memory capacity can't be 0");
    CheckM(uav_memory_capacity, "UAV memory capacity can't be 0");
    CheckM(sampler_memory_capacity, "Sampler memory capacity can't be 0");
    CheckM(copy_memory_capacity, "Copy memory capacity can't be 0");

    {
        engine->gpu_memory_manager.vb_memory.capacity = ALIGN_UP(vb_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        D3D12_HEAP_DESC heap_desc;
        heap_desc.SizeInBytes                     = engine->gpu_memory_manager.vb_memory.capacity;
        heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_desc.Properties.CreationNodeMask     = 1;
        heap_desc.Properties.VisibleNodeMask      = 1;
        heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                          &heap_desc,
                                                                                          &IID_ID3D12Heap,
                                                                                          &engine->gpu_memory_manager.vb_memory.heap);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Vertex Buffer Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.error = engine->gpu_memory_manager.vb_memory.heap->lpVtbl->SetName(engine->gpu_memory_manager.vb_memory.heap, L"Vertex Buffer Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }

    {
        engine->gpu_memory_manager.ib_memory.capacity = ALIGN_UP(ib_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        D3D12_HEAP_DESC heap_desc;
        heap_desc.SizeInBytes                     = engine->gpu_memory_manager.ib_memory.capacity;
        heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_desc.Properties.CreationNodeMask     = 1;
        heap_desc.Properties.VisibleNodeMask      = 1;
        heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                          &heap_desc,
                                                                                          &IID_ID3D12Heap,
                                                                                          &engine->gpu_memory_manager.ib_memory.heap);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Index Buffer Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.error = engine->gpu_memory_manager.ib_memory.heap->lpVtbl->SetName(engine->gpu_memory_manager.ib_memory.heap, L"Index Buffer Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }

    {
        engine->gpu_memory_manager.cbv_memory.capacity = ALIGN_UP(cbv_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        D3D12_HEAP_DESC heap_desc;
        heap_desc.SizeInBytes                     = engine->gpu_memory_manager.cbv_memory.capacity;
        heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_desc.Properties.CreationNodeMask     = 1;
        heap_desc.Properties.VisibleNodeMask      = 1;
        heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS; // D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES ?

        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                          &heap_desc,
                                                                                          &IID_ID3D12Heap,
                                                                                          &engine->gpu_memory_manager.cbv_memory.heap);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Constant Buffer Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.error = engine->gpu_memory_manager.cbv_memory.heap->lpVtbl->SetName(engine->gpu_memory_manager.cbv_memory.heap, L"Constant Buffer Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }

    {
        engine->gpu_memory_manager.srv_memory.capacity = ALIGN_UP(srv_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        D3D12_HEAP_DESC heap_desc;
        heap_desc.SizeInBytes                     = engine->gpu_memory_manager.srv_memory.capacity;
        heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_desc.Properties.CreationNodeMask     = 1;
        heap_desc.Properties.VisibleNodeMask      = 1;
        heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                          &heap_desc,
                                                                                          &IID_ID3D12Heap,
                                                                                          &engine->gpu_memory_manager.srv_memory.heap);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Shader Resource Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.error = engine->gpu_memory_manager.srv_memory.heap->lpVtbl->SetName(engine->gpu_memory_manager.srv_memory.heap, L"Shader Resource Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }

    {
        engine->gpu_memory_manager.uav_memory.capacity = ALIGN_UP(uav_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        D3D12_HEAP_DESC heap_desc;
        heap_desc.SizeInBytes                     = engine->gpu_memory_manager.uav_memory.capacity;
        heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_desc.Properties.CreationNodeMask     = 1;
        heap_desc.Properties.VisibleNodeMask      = 1;
        heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                          &heap_desc,
                                                                                          &IID_ID3D12Heap,
                                                                                          &engine->gpu_memory_manager.uav_memory.heap);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Unordered Access Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.error = engine->gpu_memory_manager.uav_memory.heap->lpVtbl->SetName(engine->gpu_memory_manager.uav_memory.heap, L"Unordered Access Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }

    {
        engine->gpu_memory_manager.sampler_memory.capacity = ALIGN_UP(sampler_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        D3D12_HEAP_DESC heap_desc;
        heap_desc.SizeInBytes                     = engine->gpu_memory_manager.sampler_memory.capacity;
        heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_desc.Properties.CreationNodeMask     = 1;
        heap_desc.Properties.VisibleNodeMask      = 1;
        heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                          &heap_desc,
                                                                                          &IID_ID3D12Heap,
                                                                                          &engine->gpu_memory_manager.sampler_memory.heap);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Sampler Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        engine->gpu_memory_manager.error = engine->gpu_memory_manager.sampler_memory.heap->lpVtbl->SetName(engine->gpu_memory_manager.sampler_memory.heap, L"Sampler Memory");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }

    {
        copy_memory_capacity = ALIGN_UP(copy_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

        D3D12_HEAP_DESC heap_desc;
        heap_desc.SizeInBytes                     = copy_memory_capacity;
        heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_CUSTOM;
        heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
        heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
        heap_desc.Properties.CreationNodeMask     = 1;
        heap_desc.Properties.VisibleNodeMask      = 1;
        heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

        for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
        {
            GPUMemory *current_copy_memory = engine->gpu_memory_manager.copy_memory + i;

            current_copy_memory->capacity = copy_memory_capacity;

            engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                              &heap_desc,
                                                                                              &IID_ID3D12Heap,
                                                                                              &current_copy_memory->heap);
            CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Copy Memory");
            Check(SUCCEEDED(engine->gpu_memory_manager.error));

            wchar_t heap_name[32];
            _swprintf(heap_name, L"Copy Memory%I32u", i);

            engine->gpu_memory_manager.error = current_copy_memory->heap->lpVtbl->SetName(current_copy_memory->heap, heap_name);
            Check(SUCCEEDED(engine->gpu_memory_manager.error));
        }
    }
}

internal void DestroyGPUMemoryManager(Engine *engine)
{
    Check(engine);

    ClearGPUMemory(engine, GPU_MEMORY_ALL);

    SafeRelease(engine->gpu_memory_manager.vb_memory.heap);
    SafeRelease(engine->gpu_memory_manager.ib_memory.heap);
    SafeRelease(engine->gpu_memory_manager.cbv_memory.heap);
    SafeRelease(engine->gpu_memory_manager.srv_memory.heap);
    SafeRelease(engine->gpu_memory_manager.uav_memory.heap);
    SafeRelease(engine->gpu_memory_manager.sampler_memory.heap);
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(engine->gpu_memory_manager.copy_memory[i].heap);
    }
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
            if ((engine->window.size.w != saved_size.w || engine->window.size.h != saved_size.h) && wparam != SIZE_MINIMIZED)
            {
            #if 0
                ResizeGPUBuffers(engine);
            #else
                DXGI_MODE_DESC mode_desc;
                mode_desc.Width                   = engine->window.size.w;
                mode_desc.Height                  = engine->window.size.h;
                mode_desc.RefreshRate.Numerator   = 0;
                mode_desc.RefreshRate.Denominator = 1;
                mode_desc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
                mode_desc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                mode_desc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
                engine->gpu_manager.error = engine->gpu_manager.swap_chain->lpVtbl->ResizeTarget(engine->gpu_manager.swap_chain, &mode_desc);
                Check(SUCCEEDED(engine->gpu_manager.error));
            #endif
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
    IN       UserCallback  *OnUpdateAndRender,
    IN       SoundCallback *OnSound)
{
    DebugResult(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST));

    Memory engine_memory;
    CreateMemory(&engine_memory, GB(1ui64), GB(2ui64));

    Engine *engine = PushToPA(Engine, &engine_memory, 1);
    engine->memory = &engine_memory;

    DebugResult(engine->user_callbacks.OnInit            = OnInit);
    DebugResult(engine->user_callbacks.OnDestroy         = OnDestroy);
    DebugResult(engine->user_callbacks.OnUpdateAndRender = OnUpdateAndRender);
    DebugResult(engine->user_callbacks.OnSound           = OnSound);

    u64 allocator_cap  = GB(1ui64);
    void *base_address = PushToPermanentArea(engine->memory, allocator_cap);
    CreateAllocator(&engine->allocator, base_address, allocator_cap, false);

    CreateLogger(&engine->logger, "Engine logger", "../log/cengine.log", LOG_TO_FILE);
    WindowCreate(engine, "CEngine", v2s_1(960, 540), instance);
    InputCreate(engine);
    TimerCreate(engine);
    CreateGPUManager(engine);
    CreateGPUMemoryManager(engine, MB(64ui64), MB(64ui64), MB(256ui64), MB(256ui64), MB(128ui64), MB(128ui64), MB(64ui64));
    CreateSound(engine);
    engine->work_queue = CreateWorkQueue(engine);

    StartFrame(engine);
    engine->user_callbacks.OnInit(engine);
    EndFrame(engine);

    TimerStart(engine);

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
            StartFrame(engine);
            engine->user_callbacks.OnUpdateAndRender(engine);
            EndFrame(engine);
        }
    }

    TimerStop(engine);
    DestroySound(engine);
    FlushGPU(engine);

    engine->user_callbacks.OnDestroy(engine);

    DestroyGPUMemoryManager(engine);
    DestroyGPUManager(engine);
    InputDestroy(engine);
    WindowDestroy(engine, instance);
    DestroyLogger(&engine->logger);
    DestroyAllocator(&engine->allocator);
    DestroyMemory(engine->memory);

    return 0;
}

#pragma warning(pop)
