// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "core/window.h"
#include "core/input.h"
#include "core/settings.h"
#include "memory/memory.h"
#include "graphics/graphics_api.h"

#if WINVER < 0x0605
    #error "Unsupported Windows version: minimal version is 1703"
#endif

namespace REV
{

Window::Window(const Logger& logger, const ConstString& title)
    : m_Instance(cast(HINSTANCE, GetModuleHandleA(null))),
      m_Handle(null),
      m_XYWH(Settings::Get()->window_xywh),
      m_DPI(GetDpiForSystem()),
      m_Closed(true),
      m_Moved(false),
      m_Resized(false),
      m_Fullscreened(false),
      m_Minimized(false),
      m_FullscreenSetRequested(Settings::Get()->fullscreen),
      m_FullscreenUnsetRequested(false),
      m_Logger(logger, null, Logger::TARGET_FILE),
      m_Title(title),
      m_ClassName(title)
{
    WNDCLASSEXA wcexa   = {0};
    wcexa.cbSize        = sizeof(WNDCLASSEXA);
    wcexa.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wcexa.lpfnWndProc   = WindowProc;
    wcexa.hInstance     = m_Instance;
    wcexa.hCursor       = LoadCursorA(0, IDC_ARROW);
    wcexa.lpszClassName = m_ClassName.Data();
    REV_DEBUG_RESULT(RegisterClassExA(&wcexa));

    REV_DEBUG_RESULT(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
    REV_DEBUG_RESULT(SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
    REV_DEBUG_RESULT(GetAwarenessFromDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == DPI_AWARENESS_PER_MONITOR_AWARE);

#if WINVER >= 0x0606
    REV_DEBUG_RESULT(SetThreadDpiHostingBehavior(DPI_HOSTING_BEHAVIOR_MIXED) != DPI_HOSTING_BEHAVIOR_INVALID);
#endif

    s32 width  = m_XYWH.z;
    s32 height = m_XYWH.w;

    if (width != REV_S32_MIN && height != REV_S32_MIN)
    {
        RECT wr = { 0, 0, width, height };
        if (AdjustWindowRectEx(&wr, WS_OVERLAPPEDWINDOW, false, 0))
        {
            width  = wr.right  - wr.left;
            height = wr.bottom - wr.top;
        }
    }

    REV_DEBUG_RESULT(m_Handle = CreateWindowExA(0, wcexa.lpszClassName, m_Title.Data(), WS_OVERLAPPEDWINDOW, m_XYWH.x, m_XYWH.y, width, height, null, null, wcexa.hInstance, 0));
    SetWindowLongPtrA(m_Handle, GWLP_USERDATA, cast(LONG_PTR, this));

    // @NOTE(Roman): Initial DPI changes. Windows doesn't send WM_DPICHANGED message
    //               on its creation even if the window is being created on a monitor
    //               DPI > system DPI (96)
    {
        u32 monitor_dpi = GetDpiForWindow(m_Handle);

        width  = MulDiv(width,  monitor_dpi, m_DPI);
        height = MulDiv(height, monitor_dpi, m_DPI);

        REV_DEBUG_RESULT(SetWindowPos(m_Handle, null,
                                    0, 0,
                                    width, height,
                                    SWP_ASYNCWINDOWPOS | SWP_NOREPOSITION | SWP_NOZORDER | SWP_NOACTIVATE));
        
        m_DPI = monitor_dpi;
    }

    m_Logger.LogSuccess("Window \"", m_Title, "\" has been created");
}

Window::~Window()
{
    if (m_Instance)
    {
        REV_DEBUG_RESULT(UnregisterClassA(m_ClassName.Data(), m_Instance));
        m_Instance = null;
        m_Logger.LogInfo("Window \"", m_Title, "\" has been destroyed");
    }
}

void Window::RequstFullscreen(bool set)
{
    if (set != m_Fullscreened)
    {
        if (set) m_FullscreenSetRequested   = true;
        else     m_FullscreenUnsetRequested = true;
    }
}

void Window::Resset()
{
    m_Resized = false;
    m_Moved   = false;
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
    m_Closed = false;
    m_Moved  = false;
}

void Window::SetTitle(const StaticString<128>& new_title)
{
    REV_DEBUG_RESULT(SetWindowTextA(m_Handle, new_title.Data()));
    m_Title = new_title;
}

void Window::ApplyFullscreenRequest()
{
    if (m_FullscreenSetRequested)
    {
        m_FullscreenSetRequested = false;
        m_Fullscreened           = true;

        GraphicsAPI::GetDeviceContext()->SetFullscreenMode(true);
    }
    else if (m_FullscreenUnsetRequested)
    {
        m_FullscreenUnsetRequested = false;
        m_Fullscreened             = false;

        GraphicsAPI::GetDeviceContext()->SetFullscreenMode(false);
    }
}

LRESULT WINAPI WindowProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    LRESULT  result = 0;
    Window  *window = cast(Window *, GetWindowLongPtrA(handle, GWLP_USERDATA));

    switch (message)
    {
        case WM_DESTROY: // 0x0002
        {
            window->m_Closed = true;
        } break;

        case WM_SIZE: // 0x0005
        {
            Math::v2s new_size(cast(s32, lparam & 0xFFFF), cast(s32, lparam >> 16));

            if ((window->m_XYWH.wh != new_size) && wparam != SIZE_MINIMIZED)
            {
                window->m_Minimized = false;
                window->m_Resized   = true;
                window->m_XYWH.wh = new_size;
            }
            else if (wparam == SIZE_MINIMIZED)
            {
                window->m_Minimized = true;
            }
        } break;

        case WM_WINDOWPOSCHANGED: // 0x0047
        {
            WINDOWPOS *window_pos = cast(WINDOWPOS *, lparam);
            Math::v2s  new_pos(window_pos->x, window_pos->y);

            if (window->m_XYWH.xy != new_pos)
            {
                window->m_Moved = true;
                window->m_XYWH.xy = new_pos;
            }

            result = DefWindowProcA(handle, message, wparam, lparam);
        } break;

        case WM_INPUT: // 0x00FF
        {
            HRAWINPUT raw_input_handle = cast(HRAWINPUT, lparam);

            UINT bytes = 0;
            GetRawInputData(raw_input_handle, RID_INPUT, 0, &bytes, sizeof(RAWINPUTHEADER));

            RAWINPUT *raw_input = cast(RAWINPUT *, Memory::Get()->PushToFrameArena(bytes));
            u32       ret_bytes = GetRawInputData(raw_input_handle, RID_INPUT, raw_input, &bytes, sizeof(RAWINPUTHEADER));
            REV_CHECK(ret_bytes == bytes);

            if (raw_input->header.dwType == RIM_TYPEMOUSE)
            {
                Input::Get()->m_Mouse.Update(raw_input->data.mouse, *window);
            }

            result = DefWindowProcA(handle, message, wparam, lparam);
        } break;

        case WM_MENUCHAR: // 0x0100
        {
            // @NOTE(Roman): suppress bip sound on alt+enter
            result = MNC_CLOSE << 16;
        } break;

        case WM_DPICHANGED: // 0x02E0
        {
            u32   new_dpi  = cast(u32, wparam >> 16);
            RECT *new_size = cast(RECT *, lparam);

            REV_DEBUG_RESULT(SetWindowPos(handle, null,
                                          new_size->left, new_size->top,
                                          new_size->right - new_size->left, new_size->bottom - new_size->top,
                                          SWP_ASYNCWINDOWPOS | SWP_NOZORDER | SWP_NOACTIVATE));

            window->m_Logger.LogDebug("DPI for window \"", window->m_Title, "\" has been changed. Old DPI: ", window->m_DPI, ", new DPI: ", new_dpi);

            window->m_DPI = new_dpi;
        } break;

        default:
        {
            result = DefWindowProcA(handle, message, wparam, lparam);
        } break;
    }

    return result;
}

}
