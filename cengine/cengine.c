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

    /**/ if (!was_connected &&  engine->input.gamepad.connected) LogInfo(&engine->logger, "gamepad was connected");
    else if ( was_connected && !engine->input.gamepad.connected) LogInfo(&engine->logger, "gamepad was disconnected");

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
        LogSuccess(&engine->logger, "Mouse was created");

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
        LogError(&engine->logger, "Mouse was not created");
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

    LogSuccess(&engine->logger, "Gamepad created");

    XINPUT_STATE xinput_state = {0};
    if (engine->input.gamepad.connected = (XInputGetState_(0, &xinput_state) == ERROR_SUCCESS))
    {
        LogInfo(&engine->logger, "Gamepad is connected");
    }
    else
    {
        LogInfo(&engine->logger, "Gamepad is not connected");
    }
}

internal void InputDestroy(Engine *engine)
{
    if (gXinput) FreeLibrary(gXinput);
    LogInfo(&engine->logger, "Input was destroyed");
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
        LogSuccess(&engine->logger, "Timer was created");
    }
    else
    {
        LogError(&engine->logger, "Timer was not created");
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

        LogInfo(&engine->logger, "Timer was started");
    }
}

void TimerStop(Engine *engine)
{
    if (!engine->timer.stopped)
    {
        QPC(engine->timer.stop_begin);
        engine->timer.stopped = true;

        LogInfo(&engine->logger, "Timer was stopped");
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
        (directx_interface) = null;                              \
    }                                                            \
}

internal void CreateAdapterAndDevice(Engine *engine)
{
    SIZE_T             max_vram      = 0;
    UINT               adapter_index = 0;
    DXGI_ADAPTER_DESC1 adapter_desc  = {0};

    for (UINT i = 0;
         engine->gpu_manager.factory->lpVtbl->EnumAdapters1(engine->gpu_manager.factory, i, &engine->gpu_manager.adapter) != DXGI_ERROR_NOT_FOUND;
         ++i)
    {
        engine->gpu_manager.error = engine->gpu_manager.adapter->lpVtbl->GetDesc1(engine->gpu_manager.adapter, &adapter_desc);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = D3D12CreateDevice(engine->gpu_manager.adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, &engine->gpu_manager.device);
        if (SUCCEEDED(engine->gpu_manager.error)
        &&  max_vram < adapter_desc.DedicatedVideoMemory
        &&  !(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
        {
            max_vram      = adapter_desc.DedicatedVideoMemory;
            adapter_index = i;
            SafeRelease(engine->gpu_manager.device);
        }
        SafeRelease(engine->gpu_manager.adapter);
    }

    if (max_vram)
    {
        engine->gpu_manager.error = engine->gpu_manager.factory->lpVtbl->EnumAdapters1(engine->gpu_manager.factory, adapter_index, &engine->gpu_manager.adapter);
        Check(SUCCEEDED(engine->gpu_manager.error));
        
        engine->gpu_manager.error = D3D12CreateDevice(engine->gpu_manager.adapter, D3D_FEATURE_LEVEL_12_1, &IID_ID3D12Device, &engine->gpu_manager.device);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = engine->gpu_manager.adapter->lpVtbl->GetDesc1(engine->gpu_manager.adapter, &adapter_desc);
        Check(SUCCEEDED(engine->gpu_manager.error));

        LogInfo(&engine->gpu_manager.logger, "GPU Adapter: %S, Feature Level: %s", adapter_desc.Description, CSTR(D3D_FEATURE_LEVEL_12_1));
    }
    else
    {
        for (UINT i = 0;
             engine->gpu_manager.factory->lpVtbl->EnumAdapters1(engine->gpu_manager.factory, i, &engine->gpu_manager.adapter) != D3D12_ERROR_ADAPTER_NOT_FOUND;
             ++i)
        {
            engine->gpu_manager.error = engine->gpu_manager.adapter->lpVtbl->GetDesc1(engine->gpu_manager.adapter, &adapter_desc);
            Check(SUCCEEDED(engine->gpu_manager.error));

            engine->gpu_manager.error = D3D12CreateDevice(engine->gpu_manager.adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &engine->gpu_manager.device);
            if (SUCCEEDED(engine->gpu_manager.error)
            &&  max_vram < adapter_desc.DedicatedVideoMemory
            &&  !(adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
            {
                max_vram      = adapter_desc.DedicatedVideoMemory;
                adapter_index = i;
                SafeRelease(engine->gpu_manager.device);
            }
            SafeRelease(engine->gpu_manager.adapter);
        }

        CheckM(max_vram, "Direct3D 12 is not supported by your hardware");

        engine->gpu_manager.error = engine->gpu_manager.factory->lpVtbl->EnumAdapters1(engine->gpu_manager.factory, adapter_index, &engine->gpu_manager.adapter);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = D3D12CreateDevice(engine->gpu_manager.adapter, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, &engine->gpu_manager.device);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = engine->gpu_manager.adapter->lpVtbl->GetDesc1(engine->gpu_manager.adapter, &adapter_desc);
        Check(SUCCEEDED(engine->gpu_manager.error));

        LogInfo(&engine->gpu_manager.logger, "GPU Adapter: %S, Feature Level: %s", adapter_desc.Description, CSTR(D3D_FEATURE_LEVEL_12_0));
    }
}

internal void CreateGPUManager(Engine *engine)
{
    DuplicateLogger("GPU Manager Logger",
                    LOG_TO_FILE | LOG_TO_CONSOLE,
                    &engine->logger,
                    &engine->gpu_manager.logger);
#if DEBUG
    engine->gpu_manager.error = D3D12GetDebugInterface(&IID_ID3D12Debug1,
                                                       &engine->gpu_manager.debug);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.debug->lpVtbl->EnableDebugLayer(engine->gpu_manager.debug);
    // engine->gpu_manager.debug->lpVtbl->SetEnableGPUBasedValidation(engine->gpu_manager.debug, true);
    engine->gpu_manager.debug->lpVtbl->SetEnableSynchronizedCommandQueueValidation(engine->gpu_manager.debug, true);

    engine->gpu_manager.error = DXGIGetDebugInterface1(0,
                                                       &IID_IDXGIDebug1,
                                                       &engine->gpu_manager.dxgi_debug);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.dxgi_debug->lpVtbl->EnableLeakTrackingForThread(engine->gpu_manager.dxgi_debug);

    engine->gpu_manager.error = engine->gpu_manager.dxgi_debug->lpVtbl->QueryInterface(engine->gpu_manager.dxgi_debug,
                                                                                       &IID_IDXGIInfoQueue,
                                                                                       &engine->gpu_manager.dxgi_info_queue);
    Check(SUCCEEDED(engine->gpu_manager.error));
#endif

    UINT factory_create_flags = 0;
#if DEBUG
    factory_create_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    engine->gpu_manager.error = CreateDXGIFactory2(factory_create_flags,
                                                   &IID_IDXGIFactory,
                                                   &engine->gpu_manager.factory);
    Check(SUCCEEDED(engine->gpu_manager.error));

    CreateAdapterAndDevice(engine);

#if DEBUG
    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->QueryInterface(engine->gpu_manager.device,
                                                                                  &IID_ID3D12InfoQueue,
                                                                                  &engine->gpu_manager.info_queue);
    Check(SUCCEEDED(engine->gpu_manager.error));
#endif

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_D3D12_OPTIONS,
                                                                                        &engine->gpu_manager.features.options,
                                                                                        sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_D3D12_OPTIONS1,
                                                                                        &engine->gpu_manager.features.options1,
                                                                                        sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS1));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_D3D12_OPTIONS2,
                                                                                        &engine->gpu_manager.features.options2,
                                                                                        sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS2));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_D3D12_OPTIONS3,
                                                                                        &engine->gpu_manager.features.options3,
                                                                                        sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS3));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_D3D12_OPTIONS4,
                                                                                        &engine->gpu_manager.features.options4,
                                                                                        sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS4));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_D3D12_OPTIONS5,
                                                                                        &engine->gpu_manager.features.options5,
                                                                                        sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_D3D12_OPTIONS6,
                                                                                        &engine->gpu_manager.features.options6,
                                                                                        sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.features.architecture.NodeIndex = 0;
    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_ARCHITECTURE1,
                                                                                        &engine->gpu_manager.features.architecture,
                                                                                        sizeof(D3D12_FEATURE_DATA_ARCHITECTURE1));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.features.root_signature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_ROOT_SIGNATURE,
                                                                                        &engine->gpu_manager.features.root_signature,
                                                                                        sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.features.shader_model.HighestShaderModel = D3D_SHADER_MODEL_6_5;
    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_SHADER_MODEL,
                                                                                        &engine->gpu_manager.features.shader_model,
                                                                                        sizeof(D3D12_FEATURE_DATA_SHADER_MODEL));
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CheckFeatureSupport(engine->gpu_manager.device,
                                                                                        D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT,
                                                                                        &engine->gpu_manager.features.virtual_address,
                                                                                        sizeof(D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT));
    Check(SUCCEEDED(engine->gpu_manager.error));

    D3D12_COMMAND_QUEUE_DESC graphics_queue_desc;
    graphics_queue_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    graphics_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME ?
    graphics_queue_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    graphics_queue_desc.NodeMask = 0;

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandQueue(engine->gpu_manager.device,
                                                                                       &graphics_queue_desc,
                                                                                       &IID_ID3D12CommandQueue,
                                                                                       &engine->gpu_manager.graphics_queue);
    Check(SUCCEEDED(engine->gpu_manager.error));
    
    engine->gpu_manager.error = engine->gpu_manager.graphics_queue->lpVtbl->SetPrivateData(engine->gpu_manager.graphics_queue,
                                                                                           &WKPDID_D3DDebugObjectName,
                                                                                           CSTRLEN("Graphics Queue"),
                                                                                           "Graphics Queue");
    Check(SUCCEEDED(engine->gpu_manager.error));

    char name[64];
    s32  name_len = 0;

    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        ID3D12CommandAllocator    **graphics_allocator = engine->gpu_manager.graphics_allocators + i;
        ID3D12GraphicsCommandList **graphics_list      = engine->gpu_manager.graphics_lists      + i;

        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandAllocator(engine->gpu_manager.device,
                                                                                               graphics_queue_desc.Type,
                                                                                               &IID_ID3D12CommandAllocator,
                                                                                               graphics_allocator);
        Check(SUCCEEDED(engine->gpu_manager.error));

        name_len = sprintf(name, "Graphics Allocator #%I32u", i);
        engine->gpu_manager.error = (*graphics_allocator)->lpVtbl->SetPrivateData(*graphics_allocator,
                                                                                  &WKPDID_D3DDebugObjectName,
                                                                                  name_len,
                                                                                  name);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommandList(engine->gpu_manager.device,
                                                                                          graphics_queue_desc.NodeMask,
                                                                                          graphics_queue_desc.Type,
                                                                                          *graphics_allocator,
                                                                                          0,
                                                                                          &IID_ID3D12GraphicsCommandList,
                                                                                          graphics_list);
        Check(SUCCEEDED(engine->gpu_manager.error));

        name_len = sprintf(name, "Graphics List #%I32u", i);
        engine->gpu_manager.error = (*graphics_list)->lpVtbl->SetPrivateData(*graphics_list,
                                                                             &WKPDID_D3DDebugObjectName,
                                                                             name_len,
                                                                             name);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = (*graphics_list)->lpVtbl->Close(*graphics_list);
        Check(SUCCEEDED(engine->gpu_manager.error));

    #if DEBUG
        engine->gpu_manager.error = (*graphics_list)->lpVtbl->QueryInterface(*graphics_list,
                                                                             &IID_ID3D12DebugCommandList,
                                                                             engine->gpu_manager.debug_graphics_lists + i);
        Check(SUCCEEDED(engine->gpu_manager.error));
    #endif
    }

    IDXGIFactory5 *factory5 = null;
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

    IDXGISwapChain1 *swap_chain1 = null;
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
    SafeRelease(swap_chain1);

#if 0 // We'are already suppressing Alt+Enter in WindowProc
    engine->gpu_manager.error = engine->gpu_manager.factory->lpVtbl->MakeWindowAssociation(engine->gpu_manager.factory,
                                                                                           engine->window.handle,
                                                                                           DXGI_MWA_NO_ALT_ENTER);
    Check(SUCCEEDED(engine->gpu_manager.error));
#endif

    D3D12_DESCRIPTOR_HEAP_DESC rtv_desc_heap_desc;
    rtv_desc_heap_desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_desc_heap_desc.NumDescriptors = SWAP_CHAIN_BUFFERS_COUNT;
    rtv_desc_heap_desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_desc_heap_desc.NodeMask       = 0;

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateDescriptorHeap(engine->gpu_manager.device,
                                                                                         &rtv_desc_heap_desc,
                                                                                         &IID_ID3D12DescriptorHeap,
                                                                                         &engine->gpu_manager.rtv_heap_desc);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.rtv_desc_size = engine->gpu_manager.device->lpVtbl->GetDescriptorHandleIncrementSize(engine->gpu_manager.device,
                                                                                                             rtv_desc_heap_desc.Type);

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
    dsv_desc_heap_desc.NodeMask       = 0;

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateDescriptorHeap(engine->gpu_manager.device,
                                                                                         &dsv_desc_heap_desc,
                                                                                         &IID_ID3D12DescriptorHeap,
                                                                                         &engine->gpu_manager.dsv_heap_desc);
    Check(SUCCEEDED(engine->gpu_manager.error));

    engine->gpu_manager.dsv_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->gpu_manager.dsv_heap_desc, &engine->gpu_manager.dsv_cpu_desc_handle);

    D3D12_HEAP_PROPERTIES ds_heap_properties;
    ds_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    ds_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    ds_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    ds_heap_properties.CreationNodeMask     = 0;
    ds_heap_properties.VisibleNodeMask      = 0;

    D3D12_RESOURCE_DESC ds_resource_desc;
    ds_resource_desc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ds_resource_desc.Alignment          = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ds_resource_desc.Width              = engine->window.size.w;
    ds_resource_desc.Height             = engine->window.size.h;
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

    engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateCommittedResource(engine->gpu_manager.device,
                                                                                            &ds_heap_properties,
                                                                                            D3D12_HEAP_FLAG_SHARED,
                                                                                            &ds_resource_desc,
                                                                                            D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                                                            &ds_clear_value,
                                                                                            &IID_ID3D12Resource,
                                                                                            &engine->gpu_manager.ds_buffer);
    Check(SUCCEEDED(engine->gpu_manager.error));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    dsv_desc.Format             = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags              = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    dsv_desc.Texture2D.MipSlice = 0;

    engine->gpu_manager.device->lpVtbl->CreateDepthStencilView(engine->gpu_manager.device,
                                                               engine->gpu_manager.ds_buffer,
                                                               &dsv_desc,
                                                               engine->gpu_manager.dsv_cpu_desc_handle);

    for (u64 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->gpu_manager.error = engine->gpu_manager.device->lpVtbl->CreateFence(engine->gpu_manager.device,
                                                                                    engine->gpu_manager.fences_values[i],
                                                                                    D3D12_FENCE_FLAG_SHARED,
                                                                                    &IID_ID3D12Fence,
                                                                                    engine->gpu_manager.fences + i);
        Check(SUCCEEDED(engine->gpu_manager.error));
        ++engine->gpu_manager.fences_values[i];
    }

    engine->gpu_manager.fence_event = CreateEventExA(0, "Fence Event", 0, EVENT_ALL_ACCESS);
    Check(engine->gpu_manager.fence_event);

    engine->gpu_manager.first_frame = true;

    LogSuccess(&engine->logger, "GPUManager was created");
}

internal void WaitForGPU(Engine *engine)
{
    ID3D12Fence *current_fence       = engine->gpu_manager.fences[engine->gpu_manager.current_buffer];
    u64          current_fence_value = engine->gpu_manager.fences_values[engine->gpu_manager.current_buffer];

    engine->gpu_manager.graphics_queue->lpVtbl->Signal(engine->gpu_manager.graphics_queue,
                                                       current_fence,
                                                       current_fence_value);

    u64 value = current_fence->lpVtbl->GetCompletedValue(current_fence);
    if (value < current_fence_value)
    {
        engine->gpu_manager.error = current_fence->lpVtbl->SetEventOnCompletion(current_fence,
                                                                                current_fence_value,
                                                                                engine->gpu_manager.fence_event);
        Check(SUCCEEDED(engine->gpu_manager.error));
        while (WaitForSingleObjectEx(engine->gpu_manager.fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }
    }

    engine->gpu_manager.fences_values[engine->gpu_manager.current_buffer] = current_fence_value + 1;
}

internal void FlushGPU(Engine *engine)
{
    for (u32 buffer_index = 0; buffer_index < SWAP_CHAIN_BUFFERS_COUNT; ++buffer_index)
    {
        ID3D12Fence *fence       = engine->gpu_manager.fences[buffer_index];
        u64          fence_value = engine->gpu_manager.fences_values[buffer_index];

        engine->gpu_manager.error = engine->gpu_manager.graphics_queue->lpVtbl->Signal(engine->gpu_manager.graphics_queue,
                                                                                       fence,
                                                                                       fence_value);
        Check(SUCCEEDED(engine->gpu_manager.error));

        engine->gpu_manager.error = fence->lpVtbl->SetEventOnCompletion(fence,
                                                                        fence_value,
                                                                        engine->gpu_manager.fence_event);
        Check(SUCCEEDED(engine->gpu_manager.error));
        while (WaitForSingleObjectEx(engine->gpu_manager.fence_event, INFINITE, false) != WAIT_OBJECT_0)
        {
        }

        ++engine->gpu_manager.fences_values[buffer_index];
    }
}

internal void DestroyGPUManager(Engine *engine)
{
    DebugResult(CloseHandle(engine->gpu_manager.fence_event));
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(engine->gpu_manager.fences[i]);
        SafeRelease(engine->gpu_manager.rt_buffers[i]);
    #if DEBUG
        SafeRelease(engine->gpu_manager.debug_graphics_lists[i]);
    #endif
        SafeRelease(engine->gpu_manager.graphics_lists[i]);
        SafeRelease(engine->gpu_manager.graphics_allocators[i]);
    }
    SafeRelease(engine->gpu_manager.ds_buffer);
    SafeRelease(engine->gpu_manager.dsv_heap_desc);
    SafeRelease(engine->gpu_manager.rtv_heap_desc);
    SafeRelease(engine->gpu_manager.swap_chain);
    SafeRelease(engine->gpu_manager.graphics_queue);
    SafeRelease(engine->gpu_manager.device);
    SafeRelease(engine->gpu_manager.adapter);
    SafeRelease(engine->gpu_manager.factory);
#if DEBUG
    engine->gpu_manager.error = engine->gpu_manager.dxgi_debug->lpVtbl->ReportLiveObjects(engine->gpu_manager.dxgi_debug,
                                                                                          DXGI_DEBUG_ALL,
                                                                                          DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    Check(SUCCEEDED(engine->gpu_manager.error));

    LogDirectXMessages(engine);

    engine->gpu_manager.dxgi_debug->lpVtbl->DisableLeakTrackingForThread(engine->gpu_manager.dxgi_debug);

    SafeRelease(engine->gpu_manager.dxgi_info_queue);
    SafeRelease(engine->gpu_manager.info_queue);
    SafeRelease(engine->gpu_manager.dxgi_debug);
    SafeRelease(engine->gpu_manager.debug);
#endif
    DestroyLogger(&engine->gpu_manager.logger);

    LogInfo(&engine->logger, "GPUManager was destroyed");
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

    u32                 node_masks[]     = { 0, 0 };
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

    engine->gpu_manager.dsv_heap_desc->lpVtbl->GetCPUDescriptorHandleForHeapStart(engine->gpu_manager.dsv_heap_desc, &engine->gpu_manager.dsv_cpu_desc_handle);

    D3D12_HEAP_PROPERTIES ds_heap_properties;
    ds_heap_properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    ds_heap_properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    ds_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    ds_heap_properties.CreationNodeMask     = 0;
    ds_heap_properties.VisibleNodeMask      = 0;

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
                                                                                            D3D12_HEAP_FLAG_SHARED,
                                                                                            &ds_resource_desc,
                                                                                            D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                                                            &ds_clear_value,
                                                                                            &IID_ID3D12Resource,
                                                                                            &engine->gpu_manager.ds_buffer);
    Check(SUCCEEDED(engine->gpu_manager.error));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
    dsv_desc.Format             = DXGI_FORMAT_D32_FLOAT;
    dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags              = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    dsv_desc.Texture2D.MipSlice = 1;

    engine->gpu_manager.device->lpVtbl->CreateDepthStencilView(engine->gpu_manager.device,
                                                                engine->gpu_manager.ds_buffer,
                                                                &dsv_desc,
                                                                engine->gpu_manager.dsv_cpu_desc_handle);

    engine->gpu_manager.first_frame = true;
}

internal void StartFrame(Engine *engine)
{
    ID3D12CommandAllocator    *graphics_allocator = engine->gpu_manager.graphics_allocators[engine->gpu_manager.current_buffer];
    ID3D12GraphicsCommandList *graphics_list      = engine->gpu_manager.graphics_lists[engine->gpu_manager.current_buffer];

    // Reinitialize all stuff was allocated on transient memory
    // ...

    // Pull
    InputPull(engine);
    TimerPull(engine);

    // Delayed SetFullscreen if we need to.
    SetFullscreen(engine, engine->window.fullscreened);

    WaitForGPU(engine);

    engine->gpu_manager.error = graphics_allocator->lpVtbl->Reset(graphics_allocator);
    Check(SUCCEEDED(engine->gpu_manager.error));
    engine->gpu_manager.error = graphics_list->lpVtbl->Reset(graphics_list, graphics_allocator, 0);
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

    if (engine->gpu_manager.first_frame)
    {
        graphics_list->lpVtbl->ResourceBarrier(graphics_list, 1, &rt_barrier);
    }
    else
    {
        D3D12_RESOURCE_BARRIER ds_barrier;
        ds_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        ds_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        ds_barrier.Transition.pResource   = engine->gpu_manager.ds_buffer;
        ds_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        ds_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        ds_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_DEPTH_WRITE;

        D3D12_RESOURCE_BARRIER begin_barriers[] = { rt_barrier, ds_barrier };
        graphics_list->lpVtbl->ResourceBarrier(graphics_list, ArrayCount(begin_barriers), begin_barriers);
    }

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
    ds_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    D3D12_RESOURCE_BARRIER end_barriers[] = { rt_barrier, ds_barrier };
    graphics_list->lpVtbl->ResourceBarrier(graphics_list, ArrayCount(end_barriers), end_barriers);

    // Close and execute lists
    engine->gpu_manager.error = graphics_list->lpVtbl->Close(graphics_list);
#if DEBUG
    LogDirectXMessages(engine);
#endif
    Check(SUCCEEDED(engine->gpu_manager.error));

    ID3D12CommandList *command_lists[] =
    {
        graphics_list,
    };
    engine->gpu_manager.graphics_queue->lpVtbl->ExecuteCommandLists(engine->gpu_manager.graphics_queue,
                                                                   ArrayCount(command_lists),
                                                                   command_lists);
    Check(SUCCEEDED(engine->gpu_manager.error));

#if DEBUG
    LogDirectXMessages(engine);
#endif

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

#if DEBUG
    LogDirectXMessages(engine);
#endif

    // Swap buffers
    engine->gpu_manager.current_buffer = engine->gpu_manager.swap_chain->lpVtbl->GetCurrentBackBufferIndex(engine->gpu_manager.swap_chain);

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

    LogSuccess(&engine->logger, "Window was created");

    engine->monitor.handle = MonitorFromWindow(engine->window.handle, MONITOR_DEFAULTTONEAREST);
    Check(engine->monitor.handle);

    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    DebugResult(GetMonitorInfoA(engine->monitor.handle, &info));

    engine->monitor.pos  = v2s_1(info.rcMonitor.left, info.rcMonitor.top);
    engine->monitor.size = v2s_1(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);

    LogSuccess(&engine->logger, "Monitor was initialized");

    SetWindowLongPtrA(engine->window.handle, GWLP_USERDATA, cast(LONG_PTR, engine));
}

internal void WindowDestroy(Engine *engine, HINSTANCE instance)
{
    DebugResult(UnregisterClassA("cengine_window_class", instance));
    LogInfo(&engine->logger, "Window was destroyed");
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

    HANDLE event        = CreateEventExA(0, "EngineSoundEvent", 0, EVENT_ALL_ACCESS);
    engine->sound.error = engine->sound.client->lpVtbl->SetEventHandle(engine->sound.client, event);
    Check(SUCCEEDED(engine->sound.error));

    u32 buffer_size     = 0;
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
    
    LogSuccess(&engine->logger, "Sound was initialized");
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
    
    LogSuccess(&engine->logger, "Sound was destroyed");
}

//
// GPUMemoryManager
//

internal void CreateGPUMemoryManager(
    IN Engine *engine,
    IN u64     gpu_memory_capacity)
{
    Check(engine);
    CheckM(gpu_memory_capacity, "GPU memory capacity can't be 0");

    LogInfo(&engine->gpu_manager.logger,
            "GPU Resource Heap Tier: D3D12_RESOURCE_HEAP_TIER_%I32d",
            engine->gpu_manager.features.options.ResourceHeapTier);

    gpu_memory_capacity = ALIGN_UP(gpu_memory_capacity, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    u64 gpu_buffer_memory_cap  = ALIGN_UP(cast(u64, 0.4f * gpu_memory_capacity), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    u64 gpu_texture_memory_cap = ALIGN_UP(cast(u64, 0.6f * gpu_memory_capacity), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    D3D12_HEAP_DESC default_buffer_heap_desc = {0};
    default_buffer_heap_desc.SizeInBytes                     = gpu_buffer_memory_cap;
    default_buffer_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    default_buffer_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    default_buffer_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    default_buffer_heap_desc.Properties.CreationNodeMask     = 0;
    default_buffer_heap_desc.Properties.VisibleNodeMask      = 0;
    default_buffer_heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    default_buffer_heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS
                                                             | D3D12_HEAP_FLAG_SHARED;

    D3D12_HEAP_DESC default_texture_heap_desc = {0};
    default_texture_heap_desc.SizeInBytes                     = gpu_texture_memory_cap;
    default_texture_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    default_texture_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    default_texture_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    default_texture_heap_desc.Properties.CreationNodeMask     = 0;
    default_texture_heap_desc.Properties.VisibleNodeMask      = 0;
    default_texture_heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    default_texture_heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES
                                                              | D3D12_HEAP_FLAG_SHARED;

    D3D12_HEAP_DESC upload_buffer_heap_desc = {0};
    upload_buffer_heap_desc.SizeInBytes                     = gpu_buffer_memory_cap;
    upload_buffer_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    upload_buffer_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    upload_buffer_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    upload_buffer_heap_desc.Properties.CreationNodeMask     = 0;
    upload_buffer_heap_desc.Properties.VisibleNodeMask      = 0;
    upload_buffer_heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    upload_buffer_heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    D3D12_HEAP_DESC upload_texture_heap_desc = {0};
    upload_texture_heap_desc.SizeInBytes                     = gpu_texture_memory_cap;
    upload_texture_heap_desc.Properties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    upload_texture_heap_desc.Properties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    upload_texture_heap_desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    upload_texture_heap_desc.Properties.CreationNodeMask     = 0;
    upload_texture_heap_desc.Properties.VisibleNodeMask      = 0;
    upload_texture_heap_desc.Alignment                       = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    upload_texture_heap_desc.Flags                           = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;

    // @Issue(Roman): Create a separate allocator for GPU Memory Manager?
    engine->gpu_memory_manager.allocator = &engine->allocator;

    // Default memory
    engine->gpu_memory_manager.buffer_memory.default_capacity  = gpu_buffer_memory_cap;
    engine->gpu_memory_manager.texture_memory.default_capacity = gpu_texture_memory_cap;

    engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                      &default_buffer_heap_desc,
                                                                                      &IID_ID3D12Heap,
                                                                                      &engine->gpu_memory_manager.buffer_memory.default_heap);
    CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Default Buffer Heap");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    engine->gpu_memory_manager.error = engine->gpu_memory_manager.buffer_memory.default_heap->lpVtbl->SetPrivateData(engine->gpu_memory_manager.buffer_memory.default_heap,
                                                                                                                     &WKPDID_D3DDebugObjectName,
                                                                                                                     CSTRLEN("Default Buffer Heap"),
                                                                                                                     "Default Buffer Heap");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                      &default_texture_heap_desc,
                                                                                      &IID_ID3D12Heap,
                                                                                      &engine->gpu_memory_manager.texture_memory.default_heap);
    CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Default Texture Heap");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    engine->gpu_memory_manager.error = engine->gpu_memory_manager.texture_memory.default_heap->lpVtbl->SetPrivateData(engine->gpu_memory_manager.texture_memory.default_heap,
                                                                                                                      &WKPDID_D3DDebugObjectName,
                                                                                                                      CSTRLEN("Default Texture Heap"),
                                                                                                                      "Default Texture Heap");
    Check(SUCCEEDED(engine->gpu_memory_manager.error));

    // Upload memory
    char heap_name[32];
    s32  heap_name_len = 0;
    
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        engine->gpu_memory_manager.buffer_memory.upload_capacity[i]  = gpu_buffer_memory_cap;
        engine->gpu_memory_manager.texture_memory.upload_capacity[i] = gpu_texture_memory_cap;

        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                          &upload_buffer_heap_desc,
                                                                                          &IID_ID3D12Heap,
                                                                                          engine->gpu_memory_manager.buffer_memory.upload_heap + i);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Upload Buffer Heap");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        ID3D12Heap *upload_buffer_heap = engine->gpu_memory_manager.buffer_memory.upload_heap[i];

        heap_name_len = sprintf(heap_name, "Upload Buffer Heap #%I32u", i);
        engine->gpu_memory_manager.error = upload_buffer_heap->lpVtbl->SetPrivateData(upload_buffer_heap,
                                                                                      &WKPDID_D3DDebugObjectName,
                                                                                      heap_name_len,
                                                                                      heap_name);
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
        
        engine->gpu_memory_manager.error = engine->gpu_manager.device->lpVtbl->CreateHeap(engine->gpu_manager.device,
                                                                                          &upload_texture_heap_desc,
                                                                                          &IID_ID3D12Heap,
                                                                                          engine->gpu_memory_manager.texture_memory.upload_heap + i);
        CheckM(engine->gpu_memory_manager.error != E_OUTOFMEMORY, "There is no enough GPU memory to create Upload Buffer Heap");
        Check(SUCCEEDED(engine->gpu_memory_manager.error));

        ID3D12Heap *upload_texture_heap = engine->gpu_memory_manager.texture_memory.upload_heap[i];

        heap_name_len = sprintf(heap_name, "Upload Texture Heap #%I32u", i);
        engine->gpu_memory_manager.error = upload_texture_heap->lpVtbl->SetPrivateData(upload_texture_heap,
                                                                                       &WKPDID_D3DDebugObjectName,
                                                                                       heap_name_len,
                                                                                       heap_name);
        Check(SUCCEEDED(engine->gpu_memory_manager.error));
    }
}

internal void DestroyGPUMemoryManager(Engine *engine)
{
    Check(engine);

    ReleaseGPUMemory(engine);

    SafeRelease(engine->gpu_memory_manager.buffer_memory.default_heap);
    SafeRelease(engine->gpu_memory_manager.texture_memory.default_heap);
    for (u32 i = 0; i < SWAP_CHAIN_BUFFERS_COUNT; ++i)
    {
        SafeRelease(engine->gpu_memory_manager.buffer_memory.upload_heap[i]);
        SafeRelease(engine->gpu_memory_manager.texture_memory.upload_heap[i]);
    }
}

//
// GPUProgramManager
//

internal void DestroyGPUProgramManager(
    IN Engine *engine)
{
    Check(engine);

    for (GraphicsProgram *it = engine->gpu_program_manager.graphics_programs; it; it = it->next)
    {
        DestroyGraphicsProgram(it);
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
                LogInfo(&engine->logger, "Window was resized");
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

            LogInfo(&engine->logger, "Monitor was changed");
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

    CreateLogger("Engine Logger", "../log/cengine.log", LOG_TO_FILE, &engine->logger);
    WindowCreate(engine, "CEngine", v2s_1(960, 540), instance);
    InputCreate(engine);
    TimerCreate(engine);
    CreateGPUManager(engine);
    CreateGPUMemoryManager(engine, MB(512ui64));
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

    DestroyGPUProgramManager(engine);
    DestroyGPUMemoryManager(engine);
    DestroyGPUManager(engine);
    InputDestroy(engine);
    WindowDestroy(engine, instance);
    DestroyLogger(&engine->logger);
    DestroyAllocator(&engine->allocator);
    DestroyMemory(engine->memory);

    return cast(int, msg.wParam);
}

#pragma warning(pop)
