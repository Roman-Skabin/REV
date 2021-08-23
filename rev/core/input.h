//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "core/window.h"
#include "core/key_codes.h"
#include "math/vec.h"
#include "tools/logger.h"
#include "math/math.h"

namespace REV
{
    class DigitalButton final
    {
    public:
        REV_INLINE DigitalButton()
            : m_Down(0),
              m_Pressed(0),
              m_Released(0)
        {
        }

        REV_INLINE bool Down()     const { return m_Down;     }
        REV_INLINE bool Pressed()  const { return m_Pressed;  }
        REV_INLINE bool Released() const { return m_Released; }

        REV_INLINE void Reset()
        {
            m_Pressed  = false;
            m_Released = false;
        }

        REV_INLINE void Update(bool down)
        {
            bool was_down = m_Down;

            m_Down     =  down;
            m_Pressed  = !was_down &&  down;
            m_Released =  was_down && !down;
        }

    private:
        u8 m_Down     : 1;
        u8 m_Pressed  : 1;
        u8 m_Released : 1;
    };

    class AnalogButton final
    {
    public:
        REV_INLINE AnalogButton(f32 threshold)
            : m_Value(0.0f),
              m_Threshold(threshold),
              m_Down(false),
              m_Pressed(false),
              m_Released(false)
        {
        }

        REV_INLINE f32  Value()     const { return m_Value;     }
        REV_INLINE f32  Threshold() const { return m_Threshold; }
        REV_INLINE bool Down()      const { return m_Down;      }
        REV_INLINE bool Pressed()   const { return m_Pressed;   }
        REV_INLINE bool Released()  const { return m_Released;  }

        REV_INLINE void Update(f32 value)
        {
            bool was_down = m_Down;

            m_Value    =  value;
            m_Down     =  m_Value  >=  m_Threshold;
            m_Pressed  = !was_down &&  m_Down;
            m_Released =  was_down && !m_Down;
        }

    private:
        f32  m_Value;
        f32  m_Threshold;
        bool m_Down;
        bool m_Pressed;
        bool m_Released;
    };

    class REV_API Keyboard final
    {
    public:
        using Key = DigitalButton;

        REV_INLINE Keyboard()
        {
            ZeroMemory(m_Keys, sizeof(m_Keys));
        }

        void Update();

        REV_INLINE const Key& operator[](KEY key) const
        {
            REV_CHECK(KEY::FIRST <= key && key <= KEY::LAST);
            return m_Keys[cast<u32>(key)];
        }
    
    private:
        REV_DELETE_CONSTRS_AND_OPS(Keyboard);

    private:
        Key m_Keys[cast<u32>(KEY::MAX)];
    };

    class REV_API Mouse final
    {
    public:
        Mouse(const Logger& logger, const Window& window);

        REV_INLINE Math::v2s REV_VECTORCALL Pos()          const { return m_Pos;          }
        REV_INLINE Math::v2s REV_VECTORCALL DeltaPos()     const { return m_DeltaPos;     }
        REV_INLINE s32                      Wheel()        const { return m_Wheel;        }
        REV_INLINE s32                      DeltaWheel()   const { return m_DeltaWheel;   }
        REV_INLINE const DigitalButton&     LeftButton()   const { return m_LeftButton;   }
        REV_INLINE const DigitalButton&     MiddleButton() const { return m_MiddleButton; }
        REV_INLINE const DigitalButton&     RightButton()  const { return m_RightButton;  }
        REV_INLINE const DigitalButton&     X1Button()     const { return m_X1Button;     }
        REV_INLINE const DigitalButton&     X2Button()     const { return m_X2Button;     }

        void Reset();
        void Update(const RAWMOUSE& raw_mouse, const Window& window);

    private:
        REV_DELETE_CONSTRS_AND_OPS(Mouse);
    
    private:
        Math::v2s     m_Pos;
        Math::v2s     m_DeltaPos;
        s32           m_Wheel;
        s32           m_DeltaWheel;
        DigitalButton m_LeftButton;
        DigitalButton m_MiddleButton;
        DigitalButton m_RightButton;
        DigitalButton m_X1Button;
        DigitalButton m_X2Button;
    };

    class Stick final
    {
    public:
        REV_INLINE Stick(f32 deadzone)
            : m_Deadzone(deadzone),
              m_Offset(0.0f),
              m_Button()
        {
        }

        REV_INLINE f32                     Deadzone() const { return m_Deadzone; }
        REV_INLINE Math::v2 REV_VECTORCALL Offset()   const { return m_Offset;   }
        REV_INLINE const DigitalButton&    Button()   const { return m_Button;   }

        REV_INLINE void Update(f32 x, f32 y, bool down)
        {
            m_Offset.x = Math::abs(x) <= m_Deadzone ? 0 : x;
            m_Offset.y = Math::abs(y) <= m_Deadzone ? 0 : y;
            m_Button.Update(down);
        }
    
    private:
        f32           m_Deadzone;
        Math::v2      m_Offset;
        DigitalButton m_Button;
    };

    class REV_API Gamepad final
    {
    public:
        Gamepad(const Logger& logger);

        REV_INLINE const DigitalButton& ButtonA()       const { return m_ButtonA;       }
        REV_INLINE const DigitalButton& ButtonB()       const { return m_ButtonB;       }
        REV_INLINE const DigitalButton& ButtonX()       const { return m_ButtonX;       }
        REV_INLINE const DigitalButton& ButtonY()       const { return m_ButtonY;       }
        REV_INLINE const AnalogButton&  LeftTrigger()   const { return m_LeftTrigger;   }
        REV_INLINE const AnalogButton&  RightTrigger()  const { return m_RightTrigger;  }
        REV_INLINE const DigitalButton& LeftShoulder()  const { return m_LeftShoulder;  }
        REV_INLINE const DigitalButton& RightShoulder() const { return m_RightShoulder; }
        REV_INLINE const DigitalButton& ButtonUp()      const { return m_ButtonUp;      }
        REV_INLINE const DigitalButton& ButtonDown()    const { return m_ButtonDown;    }
        REV_INLINE const DigitalButton& ButtonLeft()    const { return m_ButtonLeft;    }
        REV_INLINE const DigitalButton& ButtonRight()   const { return m_ButtonRight;   }
        REV_INLINE const Stick&         LeftStick()     const { return m_LeftStick;     }
        REV_INLINE const Stick&         RightStick()    const { return m_RightStick;    }
        REV_INLINE const DigitalButton& ButtonStart()   const { return m_ButtonStart;   }
        REV_INLINE const DigitalButton& ButtonBack()    const { return m_ButtonBack;    }
        REV_INLINE bool                 Connected()     const { return m_Connected;     }

        void Update(const Logger& logger);
    
    private:
        REV_DELETE_CONSTRS_AND_OPS(Gamepad);

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
    };

    class REV_API Input final
    {
    public:
        static Input *Create(const Window& window, const Logger& logger);
        static Input *Get();

    private:
        Input(const Window& window, const Logger& logger);

    public:
        void Reset();
        void Update(const Logger& logger);

        REV_INLINE const Mouse&    GetMouse()    const { return m_Mouse;    }
        REV_INLINE const Keyboard& GetKeyboard() const { return m_Keyboard; }
        REV_INLINE const Gamepad&  GetGamepad()  const { return m_Gamepad;  }

    private:
        REV_DELETE_CONSTRS_AND_OPS(Input);

    private:
        Mouse    m_Mouse;
        Gamepad  m_Gamepad;
        Keyboard m_Keyboard;

        friend LRESULT WINAPI WindowProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);
    };
}
