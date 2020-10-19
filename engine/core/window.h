//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/memory.h"
#include "math/vec.h"
#include "tools/logger.h"
#include "tools/static_string.hpp"

class ENGINE_IMPEXP Window final
{
public:
    enum class FLAGS
    {
        NONE                       = 0,
        CLOSED                     = BIT(0),
        MOVED                      = BIT(1),
        RESIZED                    = BIT(2),
        FULLSCREENED               = BIT(3),
        MINIMIZED                  = BIT(4),

        // @NOTE(Roman): Internal
        _FULLSCREEN_SET_REQUESTED   = BIT(30),
        _FULLSCREEN_UNSET_REQUESTED = BIT(31),
    };

public:
    Window(const Logger&            logger,
           const StaticString<128>& title,
           v4s                      xywh = S32_MIN
    );

    ~Window();

    void RequstFullscreen(bool set);

    void Resset();
    void PollEvents();

    void Show();

    void SetTitle(const StaticString<128>& new_title);

    constexpr const HWND               Handle()   const { return m_Handle;  }
    constexpr const StaticString<128>& Title()    const { return m_Title;   }
    constexpr const v2s&               Position() const { return m_XYWH.xy; }
    constexpr const v2s&               Size()     const { return m_XYWH.wh; }
    constexpr const v4s&               XYWH()     const { return m_XYWH;    }

    constexpr bool Closed()       const { return cast<u32>(m_Flags) & cast<u32>(FLAGS::CLOSED);       }
    constexpr bool Moved()        const { return cast<u32>(m_Flags) & cast<u32>(FLAGS::MOVED);        }
    constexpr bool Resized()      const { return cast<u32>(m_Flags) & cast<u32>(FLAGS::RESIZED);      }
    constexpr bool Fullscreened() const { return cast<u32>(m_Flags) & cast<u32>(FLAGS::FULLSCREENED); }
    constexpr bool Minimized()    const { return cast<u32>(m_Flags) & cast<u32>(FLAGS::MINIMIZED);    }

private:
    void ApplyFullscreenRequest();

    friend LRESULT WINAPI WindowProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);

    Window(const Window&)  = delete;
    Window(Window&& other) = delete;

    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&)      = delete;

private:
    HINSTANCE         m_Instance;
    HWND              m_Handle;
    HDC               m_Context;
    v4s               m_XYWH;
    FLAGS             m_Flags;
    Logger            m_Logger;
    StaticString<128> m_Title;
    StaticString<128> m_ClassName;

    friend class Application;
};

ENUM_CLASS_OPERATORS(Window::FLAGS);
