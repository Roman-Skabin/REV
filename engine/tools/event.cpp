//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/event.h"

Event::Event(const char *name, FLAGS flags)
    : m_Handle(null),
      m_Flags(flags)
{
    DebugResult(m_Handle = CreateEventExA(null, name, cast<u32>(m_Flags), EVENT_ALL_ACCESS));
}

Event::Event(const Event& other)
    : m_Handle(null),
      m_Flags(other.m_Flags)
{
    other.Wait();

    HANDLE current_process = GetCurrentProcess();
    DebugResult(DuplicateHandle(current_process,
                                other.m_Handle,
                                current_process,
                                &m_Handle,
                                0,
                                false,
                                DUPLICATE_SAME_ACCESS));
}

Event::Event(Event&& other) noexcept
    : m_Handle(other.m_Handle),
      m_Flags(other.m_Flags)
{
    other.m_Handle = null;
}

Event::~Event()
{
    Wait();
    if (m_Handle) DebugResult(CloseHandle(m_Handle));
}

bool Event::Set()
{
    return m_Handle ? SetEvent(m_Handle) : false;
}

bool Event::Reset()
{
    if (m_Handle && (m_Flags & FLAGS::RESETTABLE) != FLAGS::NONE)
    {
        return ResetEvent(m_Handle);
    }
    return false;
}

void Event::Wait() const
{
    if (m_Handle)
    {
        while (WaitForSingleObjectEx(m_Handle, INFINITE, false) != WAIT_OBJECT_0)
        {
        }
    }
}

Event& Event::operator=(const Event& other)
{
    if (this != &other)
    {
        other.Wait();

        m_Flags = other.m_Flags;

        HANDLE current_process = GetCurrentProcess();
        DebugResult(DuplicateHandle(current_process,
                                    other.m_Handle,
                                    current_process,
                                    &m_Handle,
                                    0,
                                    false,
                                    DUPLICATE_SAME_ACCESS));
    }
    return *this;
}

Event& Event::operator=(Event&& other) noexcept
{
    if (this != &other)
    {
        m_Handle = other.m_Handle;
        m_Flags  = other.m_Flags;

        other.m_Handle = null;
    }
    return *this;
}
