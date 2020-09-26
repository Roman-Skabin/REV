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

        // @NOTE(Roman): Internal
        _FULLSCREEN_SET_REQUESTED   = BIT(3),
        _FULLSCREEN_UNSET_REQUESTED = BIT(4),
    };

    typedef void OnResizeProc(v2s new_size);

public:
    Window(const Logger& logger,
           OnResizeProc *OnResize,
           const char   *title,
           v2s           size = S32_MIN,
           v2s           pos  = S32_MIN
    );
    Window(Window&& other) noexcept;

    ~Window();

    void RequstFullscreen(bool set);

    void Resset();
    void PollEvents();

    void Show();

    const char *Title()    const { return m_Title; }
    v2s         Position() const { return m_Pos;   }
    v2s         Size()     const { return m_Size;  }

    bool Closed() const;
    bool Resized() const;
    bool Fullscreened() const;

    Window& operator=(Window&& other) noexcept;

private:
    void ApplyFullscreenRequst();

    friend LRESULT WINAPI WindowProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

private:
    // @TODO(Roman): Reorganize and clean up data.

    Monitor       m_Monitor;
    HINSTANCE     m_Instance;
    HWND          m_Handle;
    HDC           m_Context;
    const char   *m_Title;
    v2s           m_Pos;
    v2s           m_Size;
    FLAGS         m_Flags;
    Logger        m_Logger;
    OnResizeProc *OnResizeCallback;

    friend class Monitor;
    friend class Input;
    friend class Application;
};

ENUM_CLASS_OPERATORS(Window::FLAGS);
