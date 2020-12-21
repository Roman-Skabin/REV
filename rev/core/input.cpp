//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/input.h"
#include <vcruntime_new.h>

namespace REV
{

//
// Keyboard
//

void Keyboard::Update()
{
    byte keyboard_state[256];
    REV_DEBUG_RESULT(GetKeyboardState(keyboard_state));

    for (u32 key = cast<u32>(KEY::FIRST); key < cast<u32>(KEY::MAX); ++key)
    {
        m_Keys[key].Update(keyboard_state[key] >> 7);
    }
}

//
// Mouse
//

Mouse::Mouse(const Logger& logger, HWND window_handle)
    : m_Pos(),
      m_DeltaPos(),
      m_Wheel(0),
      m_DeltaWheel(0),
      m_LeftButton(),
      m_MiddleButton(),
      m_RightButton(),
      m_X1Button(),
      m_X2Button()
{
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage     = 0x02;
    rid.dwFlags     = 0;
    rid.hwndTarget  = window_handle;

    if (RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        logger.LogSuccess("Mouse has been created");

        POINT screen_pos = {0};
        RECT  window_rect = {0};
        if (GetCursorPos(&screen_pos)
        &&  GetWindowRect(window_handle, &window_rect))
        {
            m_Pos.x = screen_pos.x - window_rect.left;
            m_Pos.y = window_rect.bottom - (screen_pos.y - window_rect.top);
        }
    }
    else
    {
        logger.LogError("Mouse has not been created");
        REV_FAILED_M("Mouse creation failure");
    }
}

void Mouse::Reset()
{
    m_DeltaPos.x = 0;
    m_DeltaPos.y = 0;

    m_DeltaWheel = 0;

    m_LeftButton.Reset();
    m_MiddleButton.Reset();
    m_RightButton.Reset();
    m_X1Button.Reset();
    m_X2Button.Reset();
}

void Mouse::Update(const RAWMOUSE& raw_mouse)
{
    m_DeltaPos.x =  raw_mouse.lLastX;
    m_DeltaPos.y = -raw_mouse.lLastY;

    m_Pos.x += m_DeltaPos.x;
    m_Pos.y += m_DeltaPos.y;

    if (raw_mouse.ulButtons & RI_MOUSE_WHEEL)
    {
        m_DeltaWheel  = cast<s16>(raw_mouse.usButtonData) / WHEEL_DELTA;
        m_Wheel      += m_DeltaWheel;
    }

    bool down = m_LeftButton.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP  ) down = false;
    m_LeftButton.Update(down);

    down = m_RightButton.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP  ) down = false;
    m_RightButton.Update(down);

    down = m_MiddleButton.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP  ) down = false;
    m_MiddleButton.Update(down);

    down = m_X1Button.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP  ) down = false;
    m_X1Button.Update(down);

    down = m_X2Button.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP  ) down = false;
    m_X2Button.Update(down);
}

//
// Gamepad
//

typedef DWORD WINAPI XInputGetStateProc(DWORD dwUserIndex, XINPUT_STATE *pState);

REV_GLOBAL HMODULE             g_Xinput         = null;
REV_GLOBAL XInputGetStateProc *g_XInputGetState = null;

Gamepad::Gamepad(const Logger& logger)
    : m_ButtonA(),
      m_ButtonB(),
      m_ButtonX(),
      m_ButtonY(),
      m_LeftTrigger(XINPUT_GAMEPAD_TRIGGER_THRESHOLD  / cast<f32>(REV_BYTE_MAX)),
      m_RightTrigger(XINPUT_GAMEPAD_TRIGGER_THRESHOLD / cast<f32>(REV_BYTE_MAX)),
      m_LeftShoulder(),
      m_RightShoulder(),
      m_ButtonUp(),
      m_ButtonDown(),
      m_ButtonLeft(),
      m_ButtonRight(),
      m_LeftStick(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE   / cast<f32>(REV_S16_MAX)),
      m_RightStick(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / cast<f32>(REV_S16_MAX)),
      m_ButtonStart(),
      m_ButtonBack(),
      m_Connected(false)
{
    if (!g_Xinput)
    {
        REV_DEBUG_RESULT(g_Xinput         = LoadLibraryA(XINPUT_DLL_A));
        REV_DEBUG_RESULT(g_XInputGetState = cast<XInputGetStateProc *>(GetProcAddress(g_Xinput, "XInputGetState")));
    }

    logger.LogSuccess("Gamepad has been created");

    XINPUT_STATE xinput_state = {0};
    if (m_Connected = (g_XInputGetState(0, &xinput_state) == ERROR_SUCCESS))
    {
        logger.LogInfo("Gamepad is connected");
    }
    else
    {
        logger.LogInfo("Gamepad is not connected");
    }
}

Gamepad::Gamepad(Gamepad&& other) noexcept
    : m_ButtonA(RTTI::move(other.m_ButtonA)),
      m_ButtonB(RTTI::move(other.m_ButtonB)),
      m_ButtonX(RTTI::move(other.m_ButtonX)),
      m_ButtonY(RTTI::move(other.m_ButtonY)),
      m_LeftTrigger(RTTI::move(other.m_LeftTrigger)),
      m_RightTrigger(RTTI::move(other.m_RightTrigger)),
      m_LeftShoulder(RTTI::move(other.m_LeftShoulder)),
      m_RightShoulder(RTTI::move(other.m_RightShoulder)),
      m_ButtonUp(RTTI::move(other.m_ButtonUp)),
      m_ButtonDown(RTTI::move(other.m_ButtonDown)),
      m_ButtonLeft(RTTI::move(other.m_ButtonLeft)),
      m_ButtonRight(RTTI::move(other.m_ButtonRight)),
      m_LeftStick(RTTI::move(other.m_LeftStick)),
      m_RightStick(RTTI::move(other.m_RightStick)),
      m_ButtonStart(RTTI::move(other.m_ButtonStart)),
      m_ButtonBack(RTTI::move(other.m_ButtonBack)),
      m_Connected(other.m_Connected)
{
    other.m_Connected = false;
}

void Gamepad::Update(const Logger& logger)
{
    XINPUT_STATE xinput_state = {0};

    bool was_connected = m_Connected;
    m_Connected = g_XInputGetState(0, &xinput_state) == ERROR_SUCCESS;

    /**/ if (!was_connected &&  m_Connected) logger.LogInfo("gamepad has been connected");
    else if ( was_connected && !m_Connected) logger.LogInfo("gamepad has been disconnected");

    if (m_Connected)
    {
        #define IS_DIGITAL_DOWN(button) ((xinput_state.Gamepad.wButtons & (button)) != 0)

        m_ButtonA.Update(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_A));
        m_ButtonB.Update(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_B));
        m_ButtonX.Update(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_X));
        m_ButtonY.Update(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_Y));

        m_LeftShoulder.Update(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_SHOULDER));
        m_RightShoulder.Update(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_SHOULDER));

        m_ButtonStart.Update(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_START));
        m_ButtonBack.Update(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_BACK));

        m_LeftTrigger.Update(xinput_state.Gamepad.bLeftTrigger / cast<f32>(REV_BYTE_MAX));
        m_RightTrigger.Update(xinput_state.Gamepad.bRightTrigger / cast<f32>(REV_BYTE_MAX));

        // @NOTE(Roman): negative invlerp (ret = [-1, 1])
        #define ADAPT_OFFSET(stick_dir) ((((stick_dir) + 0x8000) / cast<f32>(0xFFFF) - 0.5f) * 2.0f)

        m_LeftStick.Update(ADAPT_OFFSET(xinput_state.Gamepad.sThumbLX), ADAPT_OFFSET(xinput_state.Gamepad.sThumbLY), IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_THUMB));
        m_RightStick.Update(ADAPT_OFFSET(xinput_state.Gamepad.sThumbRX), ADAPT_OFFSET(xinput_state.Gamepad.sThumbRY), IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_THUMB));

        #undef ADAPT_OFFSET

        #undef IS_DIGITAL_DOWN
    }
}

//
// Input
//

REV_GLOBAL Input *g_Input = null;

Input *Input::Create(const Window& window, const Logger& logger)
{
    REV_CHECK_M(!g_Input, "Input is already created. Use Input::Get() function instead");
    g_Input = new (Memory::Get()->PushToPA<Input>()) Input(window, logger);
    return g_Input;
}

Input *Input::Get()
{
    REV_CHECK_M(g_Input, "Input is not created yet");
    return g_Input;
}

Input::Input(const Window& window, const Logger& logger)
    : m_Mouse(logger, window.Handle()),
      m_Gamepad(logger),
      m_Keyboard()
{
    logger.LogSuccess("Input has been created");
}

void Input::Reset()
{
    m_Mouse.Reset();
}

void Input::Update(const Logger& logger)
{
    m_Keyboard.Update();
    m_Gamepad.Update(logger);
}

}
