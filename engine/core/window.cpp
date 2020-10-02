//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/window.h"
#include "core/input.h"
#include "renderer/graphics_api.h"

//
// Monitor
//

Monitor::Monitor()
    : m_Handle(null),
      m_Pos(0, 0),
      m_Size(0, 0)
{
}

Monitor::Monitor(const Window& window, const Logger& logger)
{
    DebugResult(m_Handle = MonitorFromWindow(window.Handle(), MONITOR_DEFAULTTONEAREST));

    MONITORINFO info = {0};
    info.cbSize = sizeof(MONITORINFO);
    DebugResult(GetMonitorInfoA(m_Handle, &info));

    m_Pos  = v2s(info.rcMonitor.left, info.rcMonitor.top);
    m_Size = v2s(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);

    logger.LogSuccess("Monitor has been created");
}

Monitor::Monitor(Monitor&& other) noexcept
    : m_Handle(other.m_Handle),
      m_Pos(other.m_Pos),
      m_Size(other.m_Size)
{
    other.m_Handle = null;
}

Monitor::~Monitor()
{
    m_Handle = null;
    m_Pos    = 0L;
    m_Size   = 0L;
}

void Monitor::OnMonitorChange(const Window& window, const Logger& logger)
{
    CloseHandle(m_Handle);
    DebugResult(m_Handle = MonitorFromWindow(window.Handle(), MONITOR_DEFAULTTONEAREST));

    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    DebugResult(GetMonitorInfoA(m_Handle, &info));

    m_Pos  = v2s(info.rcMonitor.left, info.rcMonitor.top);
    m_Size = v2s(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);

    logger.LogInfo("Monitor has been changed");
}

Monitor& Monitor::operator=(Monitor&& other) noexcept
{
    if (this != &other)
    {
        m_Handle = other.m_Handle;
        m_Pos    = other.m_Pos;
        m_Size   = other.m_Size;

        other.m_Handle = null;
    }
    return *this;
}

//
// Window
//

Window::Window(in const Logger& logger,
               in const char   *title,
               in v2s           size,
               in v2s           pos)
    : m_Monitor(),
      m_Instance(cast<HINSTANCE>(GetModuleHandleA(null))),
      m_Handle(null),
      m_Context(null),
      m_Title(),
      m_Pos(pos),
      m_Size(size),
      m_Flags(FLAGS::CLOSED),
      m_Logger(logger)
{
    Check(strlen(title) < sizeof(m_Title));

    strcpy(m_Title, title);
    strcpy(m_ClassName, m_Title);

    WNDCLASSA wca     = {0};
    wca.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wca.lpfnWndProc   = WindowProc;
    wca.hInstance     = m_Instance;
    wca.hCursor       = LoadCursorA(0, IDC_ARROW);
    wca.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
    wca.lpszClassName = m_ClassName;
    DebugResult(RegisterClassA(&wca));

    s32 width  = m_Size.w;
    s32 height = m_Size.h;

    if (width != S32_MIN && height != S32_MIN)
    {
        RECT wr = { 0, 0, width, height };
        if (AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false))
        {
            width  = wr.right  - wr.left;
            height = wr.bottom - wr.top;
        }
    }

    DebugResult(m_Handle = CreateWindowA(wca.lpszClassName, m_Title, WS_OVERLAPPEDWINDOW, m_Pos.x, m_Pos.y, width, height, null, null, wca.hInstance, 0));
    DebugResult(m_Context = GetDC(m_Handle));

    m_Logger.LogSuccess("Window \"%s\" has been created", m_Title);

    m_Monitor = Monitor(*this, m_Logger);

    SetWindowLongPtrA(m_Handle, GWLP_USERDATA, cast<LONG_PTR>(this));
}

Window::Window(Window&& other) noexcept
    : m_Monitor(RTTI::move(other.m_Monitor)),
      m_Instance(other.m_Instance),
      m_Handle(other.m_Handle),
      m_Context(other.m_Context),
      m_Pos(other.m_Pos),
      m_Size(other.m_Size),
      m_Flags(other.m_Flags),
      m_Logger(RTTI::move(other.m_Logger))
{
    strcpy(m_Title, other.m_Title);
    strcpy(m_ClassName, other.m_ClassName);
    other.m_Instance = null;
    other.m_Handle   = null;
    other.m_Context  = null;
}

Window::~Window()
{
    if (m_Instance)
    {
        DebugResult(UnregisterClassA(m_ClassName, m_Instance));
        m_Instance = null;
        m_Logger.LogInfo("Window \"%s\" has been destroyed", m_Title);
    }
}

void Window::RequstFullscreen(bool set)
{
    if (set && (m_Flags & FLAGS::FULLSCREENED) == FLAGS::NONE)
    {
        m_Flags |= FLAGS::_FULLSCREEN_SET_REQUESTED;
    }
    else if ((m_Flags & FLAGS::FULLSCREENED) != FLAGS::NONE)
    {
        m_Flags |= FLAGS::_FULLSCREEN_UNSET_REQUESTED;
    }
}

void Window::Resset()
{
    m_Flags &= ~FLAGS::RESIZED;
}

void Window::PollEvents()
{
    MSG msg = {0};
    while (PeekMessageA(&msg, m_Handle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void Window::Show()
{
    ShowWindow(m_Handle, SW_SHOW);
    m_Flags &= ~FLAGS::CLOSED;
}

void Window::SetTitle(const char *title)
{
    Check(strlen(title) < sizeof(m_Title));
    DebugResult(SetWindowTextA(m_Handle, title));
    strcpy(m_Title, title);
}

bool Window::Closed() const
{
    return (m_Flags & FLAGS::CLOSED) != FLAGS::NONE;
}

bool Window::Resized() const
{
    return (m_Flags & FLAGS::RESIZED) != FLAGS::NONE;
}

bool Window::Fullscreened() const
{
    return (m_Flags & FLAGS::FULLSCREENED) != FLAGS::NONE;
}

bool Window::Minimized() const
{
    return (m_Flags & FLAGS::MINIMIZED) != FLAGS::NONE;
}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other)
    {
        m_Monitor  = RTTI::move(other.m_Monitor);
        m_Instance = other.m_Instance;
        m_Handle   = other.m_Handle;
        m_Context  = other.m_Context;
        m_Pos      = RTTI::move(other.m_Pos);
        m_Size     = RTTI::move(other.m_Size);
        m_Flags    = other.m_Flags;
        m_Logger   = RTTI::move(other.m_Logger);

        strcpy(m_Title, other.m_Title);
        strcpy(m_ClassName, other.m_ClassName);

        other.m_Instance = null;
        other.m_Handle   = null;
        other.m_Context  = null;
        other.m_Flags    = FLAGS::NONE;
    }
    return *this;
}

void Window::ApplyFullscreenRequst()
{
    local RECT wr;

    if ((m_Flags & FLAGS::_FULLSCREEN_SET_REQUESTED) != FLAGS::NONE)
    {
        DebugResult(GetWindowRect(m_Handle, &wr));
        SetWindowLongPtrA(m_Handle, GWL_STYLE,
                          GetWindowLongPtrA(m_Handle, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
        DebugResult(SetWindowPos(m_Handle, HWND_TOP,
                                 m_Monitor.m_Pos.x, m_Monitor.m_Pos.y,
                                 m_Monitor.m_Size.w, m_Monitor.m_Size.h,
                                 SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED | SWP_DEFERERASE | SWP_NOCOPYBITS | SWP_NOREDRAW));
        m_Flags &= ~FLAGS::_FULLSCREEN_SET_REQUESTED;
        m_Flags |= FLAGS::FULLSCREENED;
    }
    else if ((m_Flags & FLAGS::_FULLSCREEN_UNSET_REQUESTED) != FLAGS::NONE)
    {
        SetWindowLongPtrA(m_Handle, GWL_STYLE,
                          GetWindowLongPtrA(m_Handle, GWL_STYLE) | WS_OVERLAPPEDWINDOW);
        DebugResult(SetWindowPos(m_Handle, HWND_TOP,
                                 wr.left, wr.top,
                                 wr.right - wr.left, wr.bottom - wr.top,
                                 SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED | SWP_DEFERERASE | SWP_NOCOPYBITS | SWP_NOREDRAW));
        m_Flags &= ~FLAGS::_FULLSCREEN_UNSET_REQUESTED;
        m_Flags &= ~FLAGS::FULLSCREENED;
    }
}

LRESULT WINAPI WindowProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT  result = 0;
    Window  *window = cast<Window *>(GetWindowLongPtrA(handle, GWLP_USERDATA));

    switch (message)
    {
        case WM_DESTROY: // 0x0002
        {
            window->m_Flags |= Window::FLAGS::CLOSED;
        } break;

        case WM_SIZE: // 0x0005
        {
            v2s new_size = v2s(cast<s32>(lparam & 0xFFFF), cast<s32>(lparam >> 16));

            if ((window->m_Size.w != new_size.w || window->m_Size.h != new_size.h) && wparam != SIZE_MINIMIZED)
            {
                window->m_Size = new_size;

            #if 0
                GraphicsAPI::GetRenderer()->ResizeBuffers();
            #else
                GraphicsAPI::GetRenderer()->ResizeTarget();
            #endif

                window->m_Flags &= ~Window::FLAGS::MINIMIZED;
                window->m_Flags |=  Window::FLAGS::RESIZED;

                window->m_Logger.LogInfo("Window \"%s\" has been resized", window->m_Title);
            }
            else if (wparam == SIZE_MINIMIZED)
            {
                window->m_Flags |= Window::FLAGS::MINIMIZED;
            }
        } break;

        case WM_DISPLAYCHANGE: // 0x007E
        {
            window->m_Monitor.OnMonitorChange(*window, window->m_Logger);
        } break;

        case WM_INPUT: // 0x00FF
        {
            HRAWINPUT raw_input_handle = cast<HRAWINPUT>(lparam);

            UINT bytes = 0;
            GetRawInputData(raw_input_handle, RID_INPUT, 0, &bytes, sizeof(RAWINPUTHEADER));

            RAWINPUT *raw_input = cast<RAWINPUT *>(Memory::Get()->PushToTransientArea(bytes));

            if (GetRawInputData(raw_input_handle, RID_INPUT, raw_input, &bytes, sizeof(RAWINPUTHEADER)) == bytes
            &&  raw_input->header.dwType == RIM_TYPEMOUSE)
            {
                Input::Get()->m_Mouse.UpdateState(raw_input->data.mouse);
            }

            result = DefWindowProcA(handle, message, wparam, lparam);
        } break;

        case WM_MENUCHAR: // 0x0100
        {
            // @NOTE(Roman): suppress bip sound on alt+enter
            result = MNC_CLOSE << 16;
        } break;

        default:
        {
            result = DefWindowProcA(handle, message, wparam, lparam);
        } break;
    }

    return result;
}
