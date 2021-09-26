// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/logger.h"

namespace REV
{
    class REV_API Window final
    {
    public:
        Window(const Logger& logger, const ConstString& title);
        ~Window();

        void RequstFullscreen(bool set);

        void Resset();
        void PollEvents();

        void Show();

        void SetTitle(const StaticString<128>& new_title);

        REV_INLINE const HWND               Handle()    const { return m_Handle;  }
        REV_INLINE Math::v4s REV_VECTORCALL XYWH()      const { return m_XYWH;    }
        REV_INLINE Math::v2s REV_VECTORCALL Position()  const { return m_XYWH.xy; }
        REV_INLINE Math::v2s REV_VECTORCALL Size()      const { return m_XYWH.wh; }
        REV_INLINE const u32                DPI()       const { return m_DPI;     }
        REV_INLINE const Logger&            GetLogger() const { return m_Logger;  }
        REV_INLINE const StaticString<128>& Title()     const { return m_Title;   }

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
        u32               m_DPI;

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
