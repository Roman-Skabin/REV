//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/memory.h"
#include "math/vec.h"
#include "tools/logger.h"

class Window;

class ENGINE_IMPEXP Monitor final
{
public:
    Monitor();
    Monitor(const Window& window, const Logger& logger);
    Monitor(Monitor&& other) noexcept;

    ~Monitor();

    void OnMonitorChange(const Window& window, const Logger& logger);

    Monitor& operator=(Monitor&& other) noexcept;

private:
    Monitor(const Monitor&) = delete;
    Monitor& operator=(const Monitor&) = delete;

private:
    HMONITOR m_Handle;
    v2s      m_Pos;
    v2s      m_Size;

    friend class Window;
};

class ENGINE_IMPEXP Window final
{
public:
    enum class FLAGS
    {
        NONE                       = 0,
        CLOSED                     = BIT(0),
        RESIZED                    = BIT(1),
        FULLSCREENED               = BIT(2),
        MINIMIZED                  = BIT(3),

        // @NOTE(Roman): Internal
        _FULLSCREEN_SET_REQUESTED   = BIT(30),
        _FULLSCREEN_UNSET_REQUESTED = BIT(31),
    };

public:
    Window(in const Logger& logger,
           in const char   *title,
           in v2s           size = S32_MIN,
           in v2s           pos  = S32_MIN
    );
    Window(Window&& other) noexcept;

    ~Window();

    void RequstFullscreen(bool set);

    void Resset();
    void PollEvents();

    void Show();

    void SetTitle(const char *title);

    HWND        Handle()   const { return m_Handle; }
    const char *Title()    const { return m_Title;  }
    v2s         Position() const { return m_Pos;    }
    v2s         Size()     const { return m_Size;   }

    bool Closed()       const;
    bool Resized()      const;
    bool Fullscreened() const;
    bool Minimized()    const;

    Window& operator=(Window&& other) noexcept;

private:
    void ApplyFullscreenRequst();

    friend LRESULT WINAPI WindowProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

private:
    // @TODO(Roman): Reorganize and clean up data.

    Monitor   m_Monitor;
    HINSTANCE m_Instance;
    HWND      m_Handle;
    HDC       m_Context;
    v2s       m_Pos;
    v2s       m_Size;
    FLAGS     m_Flags;
    Logger    m_Logger;
    char      m_Title[128];
    char      m_ClassName[128];

    friend class Application;
};

ENUM_CLASS_OPERATORS(Window::FLAGS);
