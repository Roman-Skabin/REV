//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/window.h"
#include "core/input.h"
#include "core/settings.h"
#include "graphics/graphics_api.h"

namespace REV
{

Window::Window(const Logger& logger, const StaticString<128>& title)
    : m_Instance(cast<HINSTANCE>(GetModuleHandleA(null))),
      m_Handle(null),
      m_XYWH(Settings::Get()->window_xywh),
      m_Closed(true),
      m_Moved(false),
      m_Resized(false),
      m_Fullscreened(false),
      m_Minimized(false),
      m_FullscreenSetRequested(Settings::Get()->fullscreen),
      m_FullscreenUnsetRequested(false),
      m_Logger(logger),
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

    s32 width  = m_XYWH.z;
    s32 height = m_XYWH.w;

    if (width != REV_S32_MIN && height != REV_S32_MIN)
    {
        RECT wr = { 0, 0, width, height };
        if (AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false))
        {
            width  = wr.right  - wr.left;
            height = wr.bottom - wr.top;
        }
    }

    REV_DEBUG_RESULT(m_Handle = CreateWindowExA(0, wcexa.lpszClassName, m_Title.Data(), WS_OVERLAPPEDWINDOW, m_XYWH.x, m_XYWH.y, width, height, null, null, wcexa.hInstance, 0));

    m_Logger.LogSuccess("Window \"%s\" has been created", m_Title.Data());

    SetWindowLongPtrA(m_Handle, GWLP_USERDATA, cast<LONG_PTR>(this));
}

Window::~Window()
{
    if (m_Instance)
    {
        REV_DEBUG_RESULT(UnregisterClassA(m_ClassName.Data(), m_Instance));
        m_Instance = null;
        m_Logger.LogInfo("Window \"%s\" has been destroyed", m_Title.Data());
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

        GraphicsAPI::GetRenderer()->SetFullscreenMode(true);
    }
    else if (m_FullscreenUnsetRequested)
    {
        m_FullscreenUnsetRequested = false;
        m_Fullscreened             = false;

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
            window->m_Closed = true;
        } break;

        case WM_SIZE: // 0x0005
        {
            Math::v2s new_size(cast<s32>(lparam & 0xFFFF), cast<s32>(lparam >> 16));

            if ((window->m_XYWH.wh.w != new_size.w || window->m_XYWH.wh.h != new_size.h) && wparam != SIZE_MINIMIZED)
            {
                window->m_XYWH.wh   = new_size;
                window->m_Minimized = false;
                window->m_Resized   = true;

                window->m_Logger.LogInfo("Window \"%s\" has been resized: [%I32u, %I32u]", window->m_Title.Data(), window->m_XYWH.wh.w, window->m_XYWH.wh.h);
            }
            else if (wparam == SIZE_MINIMIZED)
            {
                window->m_Minimized = true;
            }
        } break;

        case WM_WINDOWPOSCHANGED: // 0x0047
        {
            WINDOWPOS *window_pos = cast<WINDOWPOS *>(lparam);
            if (window->m_XYWH.x != window_pos->x || window->m_XYWH.y != window_pos->y)
            {
                window->m_XYWH.x = window_pos->x;
                window->m_XYWH.y = window_pos->y;
                window->m_Moved  = true;
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
                Input::Get()->m_Mouse.Update(raw_input->data.mouse);
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

}
