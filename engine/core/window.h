//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/memory.h"
#include "math/vec.h"
#include "tools/logger.h"
#include "tools/static_string.hpp"

class ENGINE_API Window final
{
public:
    Window(const Logger&            logger,
           const StaticString<128>& title,
           Math::v4s                xywh = S32_MIN
    );

    ~Window();

    void RequstFullscreen(bool set);

    void Resset();
    void PollEvents();

    void Show();

    void SetTitle(const StaticString<128>& new_title);

    constexpr const HWND               Handle()   const { return m_Handle;  }
    constexpr const StaticString<128>& Title()    const { return m_Title;   }
    constexpr const Math::v2s&         Position() const { return m_XYWH.xy; }
    constexpr const Math::v2s&         Size()     const { return m_XYWH.wh; }
    constexpr const Math::v4s&         XYWH()     const { return m_XYWH;    }

    constexpr bool Closed()       const { return m_Closed;       }
    constexpr bool Moved()        const { return m_Moved;        }
    constexpr bool Resized()      const { return m_Resized;      }
    constexpr bool Fullscreened() const { return m_Fullscreened; }
    constexpr bool Minimized()    const { return m_Minimized;    }

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
    Math::v4s         m_XYWH;

    // Flags
    u32 m_Closed                   : 1;
    u32 m_Moved                    : 1;
    u32 m_Resized                  : 1;
    u32 m_Fullscreened             : 1;
    u32 m_Minimized                : 1;
    u32 m_FullscreenSetRequested   : 1; // @NOTE(Roman): Internal
    u32 m_FullscreenUnsetRequested : 1; // @NOTE(Roman): Internal

    Logger            m_Logger;
    StaticString<128> m_Title;
    StaticString<128> m_ClassName;

    friend class Application;
};
