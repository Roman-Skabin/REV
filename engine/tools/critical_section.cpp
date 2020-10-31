//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/critical_section.h"

CriticalSection::CriticalSection()
{
#if RELEASE
    InitializeCriticalSectionEx(&m_Handle, 4096, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
#else
    InitializeCriticalSectionEx(&m_Handle, 4096, 0);
#endif
}

CriticalSection::CriticalSection(CriticalSection&& other)
{
    m_Handle = other.m_Handle;
    other.m_Handle.LockSemaphore = null;
}

CriticalSection::~CriticalSection()
{
    if (m_Handle.LockSemaphore)
    {
        DeleteCriticalSection(&m_Handle);
    }
}

void CriticalSection::Enter()
{
    EnterCriticalSection(&m_Handle);
}

void CriticalSection::Leave()
{
    LeaveCriticalSection(&m_Handle);
}

CriticalSection& CriticalSection::operator=(CriticalSection&& other)
{
    if (this != &other)
    {
        m_Handle = other.m_Handle;
        other.m_Handle.LockSemaphore = null;
    }
    return *this;
}
