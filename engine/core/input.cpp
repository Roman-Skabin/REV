//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/input.h"

//
// DigitalButton
//

void DigitalButton::Reset()
{
    m_Pressed  = false;
    m_Released = false;
}

void DigitalButton::UpdateState(bool down)
{
    bool was_down = m_Down;

    m_Down     =  down;
    m_Pressed  = !was_down &&  down;
    m_Released =  was_down && !down;
}

//
// AnalogButton
//

AnalogButton::AnalogButton(f32 threshold)
    : m_Value(0.0f),
      m_Threshold(threshold),
      m_Down(false),
      m_Pressed(false),
      m_Released(false)
{
}

void AnalogButton::UpdateState(f32 value)
{
    bool was_down = m_Down;

    m_Value    =  value;
    m_Down     =  m_Value >= m_Threshold;
    m_Pressed  = !was_down &&  m_Down;
    m_Released =  was_down && !m_Down;
}

//
// Keyboard
//

Keyboard::Keyboard()
{
    ZeroMemory(m_Keys, sizeof(m_Keys));
}

Keyboard::Keyboard(Keyboard&& other) noexcept
{
    CopyMemory(m_Keys, other.m_Keys, sizeof(m_Keys));
    ZeroMemory(other.m_Keys, sizeof(other.m_Keys));
}

void Keyboard::UpdateState()
{
    BYTE keyboard_state[256];
    GetKeyboardState(keyboard_state);

    for (u16 key = cast<u16>(KEY::FIRST); key < cast<u16>(KEY::MAX); ++key)
    {
        m_Keys[key].UpdateState(keyboard_state[key] >> 7);
    }
}

Keyboard& Keyboard::operator=(Keyboard&& other) noexcept
{
    if (this != &other)
    {
        CopyMemory(m_Keys, other.m_Keys, sizeof(m_Keys));
        ZeroMemory(other.m_Keys, sizeof(other.m_Keys));
    }
    return *this;
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
        FailedM("Mouse creation failure");
    }
}

Mouse::Mouse(Mouse&& other) noexcept
    : m_Pos(RTTI::move(other.m_Pos)),
      m_DeltaPos(RTTI::move(other.m_DeltaPos)),
      m_Wheel(RTTI::move(other.m_Wheel)),
      m_DeltaWheel(RTTI::move(other.m_DeltaWheel)),
      m_LeftButton(RTTI::move(other.m_LeftButton)),
      m_MiddleButton(RTTI::move(other.m_MiddleButton)),
      m_RightButton(RTTI::move(other.m_RightButton)),
      m_X1Button(RTTI::move(other.m_X1Button)),
      m_X2Button(RTTI::move(other.m_X2Button))
{
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

void Mouse::UpdateState(const RAWMOUSE& raw_mouse)
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

    b32 down = m_LeftButton.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP  ) down = false;
    m_LeftButton.UpdateState(down);

    down = m_RightButton.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP  ) down = false;
    m_RightButton.UpdateState(down);

    down = m_MiddleButton.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP  ) down = false;
    m_MiddleButton.UpdateState(down);

    down = m_X1Button.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP  ) down = false;
    m_X1Button.UpdateState(down);

    down = m_X2Button.Down();
    if (raw_mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) down = true;
    if (raw_mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP  ) down = false;
    m_X2Button.UpdateState(down);
}

Mouse& Mouse::operator=(Mouse&& other) noexcept
{
    if (this != &other)
    {
        m_Pos          = RTTI::move(other.m_Pos);
        m_DeltaPos     = RTTI::move(other.m_DeltaPos);
        m_Wheel        = RTTI::move(other.m_Wheel);
        m_DeltaWheel   = RTTI::move(other.m_DeltaWheel);
        m_LeftButton   = RTTI::move(other.m_LeftButton);
        m_MiddleButton = RTTI::move(other.m_MiddleButton);
        m_RightButton  = RTTI::move(other.m_RightButton);
        m_X1Button     = RTTI::move(other.m_X1Button);
        m_X2Button     = RTTI::move(other.m_X2Button);
    }
    return *this;
}

//
// Stick
//

Stick::Stick(f32 deadzone)
    : m_Deadzone(deadzone),
      m_Offset(0.0f),
      m_Button()
{
}

void Stick::UpdateState(f32 x, f32 y, bool down)
{
    #define ABSF(val) ((val) < 0.0f ? (-(val)) : (val))
    m_Offset.x = ABSF(x) <= m_Deadzone ? x : 0;
    m_Offset.y = ABSF(y) <= m_Deadzone ? y : 0;
    #undef ABSF

    m_Button.UpdateState(down);
}

//
// Gamepad
//

HMODULE                      Gamepad::s_Xinput         = null;
Gamepad::XInputGetStateProc *Gamepad::s_XInputGetState = null;

Gamepad::Gamepad(const Logger& logger)
    : m_ButtonA(),
      m_ButtonB(),
      m_ButtonX(),
      m_ButtonY(),
      m_LeftTrigger(XINPUT_GAMEPAD_TRIGGER_THRESHOLD / cast<f32>(MAXBYTE)),
      m_RightTrigger(XINPUT_GAMEPAD_TRIGGER_THRESHOLD / cast<f32>(MAXBYTE)),
      m_LeftShoulder(),
      m_RightShoulder(),
      m_ButtonUp(),
      m_ButtonDown(),
      m_ButtonLeft(),
      m_ButtonRight(),
      m_LeftStick(XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  / cast<f32>(MAXSHORT)),
      m_RightStick(XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE / cast<f32>(MAXSHORT)),
      m_ButtonStart(),
      m_ButtonBack(),
      m_Connected(false)
{
    if (!s_Xinput)
    {
        DebugResult(s_Xinput         = LoadLibraryA(XINPUT_DLL_A));
        DebugResult(s_XInputGetState = cast<XInputGetStateProc *>(GetProcAddress(s_Xinput, "XInputGetState")));
    }

    logger.LogSuccess("Gamepad has been created");

    XINPUT_STATE xinput_state = {0};
    if (m_Connected = (s_XInputGetState(0, &xinput_state) == ERROR_SUCCESS))
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

void Gamepad::UpdateState(const Logger& logger)
{
    XINPUT_STATE xinput_state = {0};

    b32 was_connected = m_Connected;
    m_Connected = s_XInputGetState(0, &xinput_state) == ERROR_SUCCESS;

    /**/ if (!was_connected &&  m_Connected) logger.LogInfo("gamepad has been connected");
    else if ( was_connected && !m_Connected) logger.LogInfo("gamepad has been disconnected");

    if (m_Connected)
    {
        #define IS_DIGITAL_DOWN(button) ((xinput_state.Gamepad.wButtons & (button)) != 0)

        m_ButtonA.UpdateState(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_A));
        m_ButtonB.UpdateState(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_B));
        m_ButtonX.UpdateState(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_X));
        m_ButtonY.UpdateState(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_Y));

        m_LeftShoulder.UpdateState(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_SHOULDER));
        m_RightShoulder.UpdateState(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_SHOULDER));

        m_ButtonStart.UpdateState(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_START));
        m_ButtonBack.UpdateState(IS_DIGITAL_DOWN(XINPUT_GAMEPAD_BACK));

        m_LeftTrigger.UpdateState(xinput_state.Gamepad.bLeftTrigger / cast<f32>(MAXBYTE));
        m_RightTrigger.UpdateState(xinput_state.Gamepad.bRightTrigger / cast<f32>(MAXBYTE));

        #define STICK_OFFSET(stick_dir) (2.0f * (((stick_dir) + 0x8000) / cast<f32>(MAXWORD) - 0.5f))

        m_LeftStick.UpdateState(STICK_OFFSET(xinput_state.Gamepad.sThumbLX), STICK_OFFSET(xinput_state.Gamepad.sThumbLY), IS_DIGITAL_DOWN(XINPUT_GAMEPAD_LEFT_THUMB));
        m_RightStick.UpdateState(STICK_OFFSET(xinput_state.Gamepad.sThumbRX), STICK_OFFSET(xinput_state.Gamepad.sThumbRY), IS_DIGITAL_DOWN(XINPUT_GAMEPAD_RIGHT_THUMB));

        #undef STICK_OFFSET

        #undef IS_DIGITAL_DOWN
    }
}

Gamepad& Gamepad::operator=(Gamepad&& other) noexcept
{
    if (this != &other)
    {
        m_ButtonA       = RTTI::move(other.m_ButtonA);
        m_ButtonB       = RTTI::move(other.m_ButtonB);
        m_ButtonX       = RTTI::move(other.m_ButtonX);
        m_ButtonY       = RTTI::move(other.m_ButtonY);
        m_LeftTrigger   = RTTI::move(other.m_LeftTrigger);
        m_RightTrigger  = RTTI::move(other.m_RightTrigger);
        m_LeftShoulder  = RTTI::move(other.m_LeftShoulder);
        m_RightShoulder = RTTI::move(other.m_RightShoulder);
        m_ButtonUp      = RTTI::move(other.m_ButtonUp);
        m_ButtonDown    = RTTI::move(other.m_ButtonDown);
        m_ButtonLeft    = RTTI::move(other.m_ButtonLeft);
        m_ButtonRight   = RTTI::move(other.m_ButtonRight);
        m_LeftStick     = RTTI::move(other.m_LeftStick);
        m_RightStick    = RTTI::move(other.m_RightStick);
        m_ButtonStart   = RTTI::move(other.m_ButtonStart);
        m_ButtonBack    = RTTI::move(other.m_ButtonBack);
        m_Connected     = other.m_Connected;

        other.m_Connected = false;
    }
    return *this;
}

//
// Input
//

Input *Input::s_Input = null;

Input *Input::Create(const Window& window, const Logger& logger)
{
    CheckM(!s_Input, "Input is already created. Use Input::Get() function instead");
    s_Input  = Memory::Get()->PushToPA<Input>();
    *s_Input = Input(window, logger);
    return s_Input;
}

Input *Input::Get()
{
    CheckM(s_Input, "Input is not created yet");
    return s_Input;
}

Input::Input(const Window& window, const Logger& logger)
    : m_Mouse(logger, window.m_Handle),
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
    m_Keyboard.UpdateState();
    m_Gamepad.UpdateState(logger);
}

Input& Input::operator=(Input&& other) noexcept
{
    if (this != &other)
    {
        m_Mouse    = RTTI::move(other.m_Mouse);
        m_Gamepad  = RTTI::move(other.m_Gamepad);
        m_Keyboard = RTTI::move(other.m_Keyboard);
    }
    return *this;
}
