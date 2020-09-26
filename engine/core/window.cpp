//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/window.h"
#include "core/input.h"

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
    DebugResult(m_Handle = MonitorFromWindow(window.m_Handle, MONITOR_DEFAULTTONEAREST));

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
    if (m_Handle)
    {
        CloseHandle(m_Handle);
        m_Handle = null;
    }
    m_Pos  = 0L;
    m_Size = 0L;
}

void Monitor::OnMonitorChange(const Window& window, const Logger& logger)
{
    CloseHandle(m_Handle);
    DebugResult(m_Handle = MonitorFromWindow(window.m_Handle, MONITOR_DEFAULTTONEAREST));

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

Window::Window(const Logger& logger,
               OnResizeProc *OnResize,
               const char   *title,
               v2s           size,
               v2s           pos)
    : m_Monitor(),
      m_Instance(cast<HINSTANCE>(GetModuleHandleA(null))),
      m_Handle(null),
      m_Context(null),
      m_Title(title),
      m_Pos(pos),
      m_Size(size),
      m_Flags(FLAGS::CLOSED),
      m_Logger(logger),
      OnResizeCallback(OnResize)
{
    // CheckM(OnResize, "OnResize can't be null");

    WNDCLASSA wca     = {0};
    wca.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wca.lpfnWndProc   = WindowProc;
    wca.hInstance     = m_Instance;
    wca.hCursor       = LoadCursorA(0, IDC_ARROW);
    wca.lpszClassName = m_Title;
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
      m_Title(other.m_Title),
      m_Pos(other.m_Pos),
      m_Size(other.m_Size),
      m_Flags(other.m_Flags),
      m_Logger(RTTI::move(other.m_Logger)),
      OnResizeCallback(other.OnResizeCallback)
{
    other.m_Instance = null;
    other.m_Handle   = null;
    other.m_Context  = null;
}

Window::~Window()
{
    if (m_Instance)
    {
        DebugResult(UnregisterClassA(m_Title, m_Instance));
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

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other)
    {
        m_Monitor        = RTTI::move(other.m_Monitor);
        m_Instance       = other.m_Instance;
        m_Handle         = other.m_Handle;
        m_Context        = other.m_Context;
        m_Title          = other.m_Title;
        m_Pos            = RTTI::move(other.m_Pos);
        m_Size           = RTTI::move(other.m_Size);
        m_Flags          = other.m_Flags;
        m_Logger         = RTTI::move(other.m_Logger);
        OnResizeCallback = other.OnResizeCallback;

        other.m_Instance       = null;
        other.m_Handle         = null;
        other.m_Context        = null;
        other.m_Title          = null;
        other.m_Flags          = FLAGS::NONE;
        other.OnResizeCallback = null;
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
                // @TODO(Roman): Move to the OnResizeCallback.
            #if 0
            #if 0
                ResizeGPUBuffers(engine);
            #else
                DXGI_MODE_DESC mode_desc;
                mode_desc.Width                   = new_size.w;
                mode_desc.Height                  = new_size.h;
                mode_desc.RefreshRate.Numerator   = 0;
                mode_desc.RefreshRate.Denominator = 1;
                mode_desc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
                mode_desc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                mode_desc.Scaling                 = DXGI_MODE_SCALING_STRETCHED;
                gpu_manager.error = gpu_manager.swap_chain->lpVtbl->ResizeTarget(gpu_manager.swap_chain, &mode_desc);
                Check(SUCCEEDED(gpu_manager.error));
            #endif
            #endif

                if (window->OnResizeCallback) window->OnResizeCallback(new_size);

                window->m_Size = new_size;
                window->m_Flags |= Window::FLAGS::RESIZED;
                window->m_Logger.LogInfo("Window \"%s\" has been resized", window->m_Title);
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
