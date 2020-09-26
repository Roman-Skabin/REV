//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/window.h"
#include "core/key_codes.h"
#include "math/vec.h"
#include "tools/logger.h"

class ENGINE_IMPEXP DigitalButton final
{
public:
    bool Down()     const { return m_Down;     }
    bool Pressed()  const { return m_Pressed;  }
    bool Released() const { return m_Released; }

    void Reset();
    void UpdateState(bool down);

private:
    bool m_Down     = false;
    bool m_Pressed  = false;
    bool m_Released = false;
};

class ENGINE_IMPEXP AnalogButton final
{
public:
    AnalogButton(f32 threshold);

    f32  Value()     const { return m_Value;     }
    f32  Threshold() const { return m_Threshold; }
    bool Down()      const { return m_Down;      }
    bool Pressed()   const { return m_Pressed;   }
    bool Released()  const { return m_Released;  }

    void UpdateState(f32 value);

private:
    f32  m_Value;
    f32  m_Threshold;
    bool m_Down;
    bool m_Pressed;
    bool m_Released;
};

class ENGINE_IMPEXP Keyboard final
{
public:
    using Key = DigitalButton;

    Keyboard();
    Keyboard(Keyboard&& other) noexcept;

    void UpdateState();

    const Key& operator[](KEY key) const
    {
        Check(KEY::FIRST <= key && key <= KEY::LAST);
        return m_Keys[cast<u16>(key)];
    }

    Keyboard& operator=(Keyboard&& other) noexcept;

private:
    Key m_Keys[cast<u16>(KEY::MAX)];
};

class ENGINE_IMPEXP Mouse final
{
public:
    Mouse(const Logger& logger, HWND window_handle);
    Mouse(Mouse&& other) noexcept;

    v2s                  Pos()          const { return m_Pos;          }
    v2s                  DeltaPos()     const { return m_DeltaPos;     }
    s32                  Wheel()        const { return m_Wheel;        }
    s32                  DeltaWheel()   const { return m_DeltaWheel;   }
    const DigitalButton& LeftButton()   const { return m_LeftButton;   }
    const DigitalButton& MiddleButton() const { return m_MiddleButton; }
    const DigitalButton& RightButton()  const { return m_RightButton;  }
    const DigitalButton& X1Button()     const { return m_X1Button;     }
    const DigitalButton& X2Button()     const { return m_X2Button;     }

    void Reset();
    void UpdateState(const RAWMOUSE& raw_mouse);

    Mouse& operator=(Mouse&& other) noexcept;

private:
    v2s           m_Pos;
    v2s           m_DeltaPos;
    s32           m_Wheel;
    s32           m_DeltaWheel;
    DigitalButton m_LeftButton;
    DigitalButton m_MiddleButton;
    DigitalButton m_RightButton;
    DigitalButton m_X1Button;
    DigitalButton m_X2Button;
};

class ENGINE_IMPEXP Stick final
{
public:
    Stick(f32 deadzone);

    f32                  Deadzone() const { return m_Deadzone; }
    v2                   Offset()   const { return m_Offset;   }
    const DigitalButton& Button()   const { return m_Button;   }

    void UpdateState(f32 x, f32 y, bool down);

private:
    f32           m_Deadzone;
    v2            m_Offset;
    DigitalButton m_Button;
};

class ENGINE_IMPEXP Gamepad final
{
public:
    Gamepad(const Logger& logger);
    Gamepad(Gamepad&& other) noexcept;

    const DigitalButton& ButtonA()       const { return m_ButtonA;       }
    const DigitalButton& ButtonB()       const { return m_ButtonB;       }
    const DigitalButton& ButtonX()       const { return m_ButtonX;       }
    const DigitalButton& ButtonY()       const { return m_ButtonY;       }
    const AnalogButton&  LeftTrigger()   const { return m_LeftTrigger;   }
    const AnalogButton&  RightTrigger()  const { return m_RightTrigger;  }
    const DigitalButton& LeftShoulder()  const { return m_LeftShoulder;  }
    const DigitalButton& RightShoulder() const { return m_RightShoulder; }
    const DigitalButton& ButtonUp()      const { return m_ButtonUp;      }
    const DigitalButton& ButtonDown()    const { return m_ButtonDown;    }
    const DigitalButton& ButtonLeft()    const { return m_ButtonLeft;    }
    const DigitalButton& ButtonRight()   const { return m_ButtonRight;   }
    const Stick&         LeftStick()     const { return m_LeftStick;     }
    const Stick&         RightStick()    const { return m_RightStick;    }
    const DigitalButton& ButtonStart()   const { return m_ButtonStart;   }
    const DigitalButton& ButtonBack()    const { return m_ButtonBack;    }
    bool                 Connected()     const { return m_Connected;     }

    void UpdateState(const Logger& logger);

    Gamepad& operator=(Gamepad&& other) noexcept;

private:
    DigitalButton m_ButtonA;
    DigitalButton m_ButtonB;
    DigitalButton m_ButtonX;
    DigitalButton m_ButtonY;
    AnalogButton  m_LeftTrigger;
    AnalogButton  m_RightTrigger;
    DigitalButton m_LeftShoulder;
    DigitalButton m_RightShoulder;
    DigitalButton m_ButtonUp;
    DigitalButton m_ButtonDown;
    DigitalButton m_ButtonLeft;
    DigitalButton m_ButtonRight;
    Stick         m_LeftStick;
    Stick         m_RightStick;
    DigitalButton m_ButtonStart;
    DigitalButton m_ButtonBack;
    bool          m_Connected;

    typedef DWORD WINAPI XInputGetStateProc(DWORD dwUserIndex, XINPUT_STATE *pState);

    static HMODULE             s_Xinput;
    static XInputGetStateProc *s_XInputGetState;
};

class ENGINE_IMPEXP Input final
{
public:
    static Input *Create(const Window& window, const Logger& logger);
    static Input *Get();

private:
    Input(const Window& window, const Logger& logger);

public:
    void Reset();
    void Update(const Logger& logger);

    const Mouse&    GetMouse()    const { return m_Mouse;    }
    const Keyboard& GetKeyboard() const { return m_Keyboard; }
    const Gamepad&  GetGamepad()  const { return m_Gamepad;  }

private:
    Input& operator=(Input&& other) noexcept;

    Input(const Input&) = delete;
    Input(Input&&)      = delete;

    Input& operator=(const Input&) = delete;

private:
    Mouse    m_Mouse;
    Gamepad  m_Gamepad;
    Keyboard m_Keyboard;

    static Input *s_Input;

    friend LRESULT WINAPI WindowProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);
};
