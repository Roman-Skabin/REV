//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "core/memory.h"
#include "math/vec.h"
#include "tools/logger.h"
#include "tools/static_string.hpp"

namespace REV
{
    class REV_API Window final
    {
    public:
        Window(const Logger& logger, const StaticString<128>& title);
        ~Window();

        void RequstFullscreen(bool set);

        void Resset();
        void PollEvents();

        void Show();

        void SetTitle(const StaticString<128>& new_title);

        REV_INLINE const HWND               Handle()   const { return m_Handle;  }
        REV_INLINE const StaticString<128>& Title()    const { return m_Title;   }
        REV_INLINE const Math::v2s&         Position() const { return m_XYWH.xy; }
        REV_INLINE const Math::v2s&         Size()     const { return m_XYWH.wh; }
        REV_INLINE const Math::v4s&         XYWH()     const { return m_XYWH;    }

        REV_INLINE bool Closed()       const { return m_Closed;       }
        REV_INLINE bool Moved()        const { return m_Moved;        }
        REV_INLINE bool Resized()      const { return m_Resized;      }
        REV_INLINE bool Fullscreened() const { return m_Fullscreened; }
        REV_INLINE bool Minimized()    const { return m_Minimized;    }

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
}
