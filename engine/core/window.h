//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "math/vec.h"
#include "tools/logger.h"
#include "core/memory.h"

#pragma warning(push)
#pragma warning(disable: 4251) // v2s gotta have dll-instance

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
    typedef void OnMouseEventProc(const RAWMOUSE& raw_mouse);

public:
    Window(Memory&           memory,
           OnResizeProc     *OnResize,
           OnMouseEventProc *OnMouseEvent,
           const Logger&     logger,
           const char       *title,
           v2s               size         = S32_MIN,
           v2s               pos          = S32_MIN,
           HINSTANCE         instance     = null
    );
    Window(Window&& other) noexcept;

    ~Window();

    void RequstFullscreen(bool set);

    void Resset();
    void PollEvents();

    bool Closed() const;
    bool Resized() const;
    bool Fullscreened() const;

    void Show();

    const char *Title()    const { return m_Title; }
    v2s         Position() const { return m_Pos;   }
    v2s         Size()     const { return m_Size;  }

    Window& operator=(Window&& other) noexcept;

private:
    void ApplyFullscreenRequst();

    friend LRESULT WINAPI WindowProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

private:
    Monitor           m_Monitor;
    HINSTANCE         m_Instance;
    HWND              m_Handle;
    HDC               m_Context;
    const char       *m_Title;
    v2s               m_Pos;
    v2s               m_Size;
    FLAGS             m_Flags;
    const Logger&     m_Logger;
    OnResizeProc     *OnResizeCallback;
    OnMouseEventProc *OnMouseEventCallback;
    Memory&           m_Memory;

    friend class Monitor;
};

ENUM_CLASS_OPERATORS(Window::FLAGS);

#pragma warning(pop)