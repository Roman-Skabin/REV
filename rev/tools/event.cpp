//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/event.h"

namespace REV
{

Event::Event(const char *name, FLAGS flags)
    : m_Handle(null),
      m_Flags(flags)
{
    REV_DEBUG_RESULT(m_Handle = CreateEventExA(null, name, cast(u32, m_Flags), EVENT_ALL_ACCESS));
}

Event::Event(const Event& other)
    : m_Handle(null),
      m_Flags(other.m_Flags)
{
    HANDLE current_process = GetCurrentProcess();
    REV_DEBUG_RESULT(DuplicateHandle(current_process,
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
    if (m_Handle) REV_DEBUG_RESULT(CloseHandle(m_Handle));
}

bool Event::Set()
{
    return m_Handle ? SetEvent(m_Handle) : false;
}

bool Event::Reset()
{
    return m_Handle && (m_Flags & FLAGS::RESETTABLE) != FLAGS::NONE
         ? ResetEvent(m_Handle)
         : false;
}

void Event::Wait(u32 milliseconds, bool alerable) const
{
    if (m_Handle)
    {
        while (WaitForSingleObjectEx(m_Handle, milliseconds, alerable) != WAIT_OBJECT_0)
        {
        }
    }
}

Event& Event::operator=(const Event& other)
{
    if (this != &other)
    {
        m_Flags = other.m_Flags;

        HANDLE current_process = GetCurrentProcess();
        REV_DEBUG_RESULT(DuplicateHandle(current_process,
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

}
