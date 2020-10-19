//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/window.h"
#include "core/input.h"
#include "renderer/graphics_api.h"

Window::Window(const Logger&            logger,
               const StaticString<128>& title,
               v4s                      xywh)
    : m_Instance(cast<HINSTANCE>(GetModuleHandleA(null))),
      m_Handle(null),
      m_Context(null),
      m_XYWH(xywh),
      m_Flags(FLAGS::CLOSED),
      m_Logger(logger),
      m_Title(title),
      m_ClassName(title)
{
    WNDCLASSA wca     = {0};
    wca.style         = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wca.lpfnWndProc   = WindowProc;
    wca.hInstance     = m_Instance;
    wca.hCursor       = LoadCursorA(0, IDC_ARROW);
    // wca.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
    wca.lpszClassName = m_ClassName;
    DebugResult(RegisterClassA(&wca));

    s32 width  = m_XYWH.z;
    s32 height = m_XYWH.w;

    if (width != S32_MIN && height != S32_MIN)
    {
        RECT wr = { 0, 0, width, height };
        if (AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false))
        {
            width  = wr.right  - wr.left;
            height = wr.bottom - wr.top;
        }
    }

    DebugResult(m_Handle = CreateWindowA(wca.lpszClassName, m_Title, WS_OVERLAPPEDWINDOW, m_XYWH.x, m_XYWH.y, width, height, null, null, wca.hInstance, 0));
    DebugResult(m_Context = GetDC(m_Handle));

    m_Logger.LogSuccess("Window \"%s\" has been created", m_Title.Data());

    SetWindowLongPtrA(m_Handle, GWLP_USERDATA, cast<LONG_PTR>(this));
}

Window::~Window()
{
    if (m_Instance)
    {
        DebugResult(UnregisterClassA(m_ClassName, m_Instance));
        m_Instance = null;
        m_Logger.LogInfo("Window \"%s\" has been destroyed", m_Title.Data());
    }
}

void Window::RequstFullscreen(bool set)
{
    if (set)
    {
        if ((m_Flags & FLAGS::FULLSCREENED) == FLAGS::NONE)
        {
            m_Flags |= FLAGS::_FULLSCREEN_SET_REQUESTED;
        }
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
    m_Flags &= ~FLAGS::MOVED;
}

void Window::SetTitle(const StaticString<128>& new_title)
{
    DebugResult(SetWindowTextA(m_Handle, new_title));
    m_Title = new_title;
}

void Window::ApplyFullscreenRequest()
{
    if ((m_Flags & FLAGS::_FULLSCREEN_SET_REQUESTED) != FLAGS::NONE)
    {
        m_Flags &= ~FLAGS::_FULLSCREEN_SET_REQUESTED;
        m_Flags |=  FLAGS::FULLSCREENED;

        GraphicsAPI::GetRenderer()->SetFullscreenMode(true);
    }
    else if ((m_Flags & FLAGS::_FULLSCREEN_UNSET_REQUESTED) != FLAGS::NONE)
    {
        m_Flags &= ~FLAGS::_FULLSCREEN_UNSET_REQUESTED;
        m_Flags &= ~FLAGS::FULLSCREENED;

        GraphicsAPI::GetRenderer()->SetFullscreenMode(false);
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

            if ((window->m_XYWH.wh.w != new_size.w || window->m_XYWH.wh.h != new_size.h) && wparam != SIZE_MINIMIZED)
            {
                window->m_XYWH.wh  = new_size;
                window->m_Flags   &= ~Window::FLAGS::MINIMIZED;
                window->m_Flags   |=  Window::FLAGS::RESIZED;

                window->m_Logger.LogInfo("Window \"%s\" has been resized: [%I32u, %I32u]", window->m_Title.Data(), window->m_XYWH.wh.w, window->m_XYWH.wh.h);
            }
            else if (wparam == SIZE_MINIMIZED)
            {
                window->m_Flags |= Window::FLAGS::MINIMIZED;
            }
        } break;

        case WM_WINDOWPOSCHANGED: // 0x0047
        {
            WINDOWPOS *window_pos = cast<WINDOWPOS *>(lparam);
            if (window->m_XYWH.x != window_pos->x || window->m_XYWH.y != window_pos->y)
            {
                window->m_XYWH.x  = window_pos->x;
                window->m_XYWH.y  = window_pos->y;
                window->m_Flags  |= Window::FLAGS::MOVED;
            }

            result = DefWindowProcA(handle, message, wparam, lparam);
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
