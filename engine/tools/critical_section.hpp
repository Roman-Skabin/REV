//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

template<bool waitable>
class CriticalSection;

template<>
class CriticalSection<true>
{
public:
    CriticalSection()
        : m_Handle{}
    {
    #if RELEASE
        InitializeCriticalSectionEx(&m_Handle, 4096, RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO);
    #else
        InitializeCriticalSectionEx(&m_Handle, 4096, 0);
    #endif
    }

    CriticalSection(CriticalSection&& other)
        : m_Handle(other.m_Handle)
    {
        other.m_Handle.LockSemaphore = null;
    }

    ~CriticalSection()
    {
        if (m_Handle.LockSemaphore)
        {
            DeleteCriticalSection(&m_Handle);
        }
    }

    void Enter()
    {
        EnterCriticalSection(&m_Handle);
    }

    void Leave()
    {
        LeaveCriticalSection(&m_Handle);
    }

    constexpr bool Waitable() const { return true; }

    CriticalSection& operator=(CriticalSection&& other)
    {
        m_Handle = other.m_Handle;
        other.m_Handle.LockSemaphore = null;
        return *this;
    }

private:
    CriticalSection(const CriticalSection&)            = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;

private:
    RTL_CRITICAL_SECTION m_Handle;
};

template<>
class CriticalSection<false>
{
public:
    CriticalSection()
        : m_CurrentThreadID(0),
          m_NextThreadID(0),
          m_Owner(null),
          m_RecursionCount(0)
    {
    }

    CriticalSection(CriticalSection&& other)
        : m_CurrentThreadID(other.m_CurrentThreadID),
          m_NextThreadID(other.m_NextThreadID),
          m_Owner(other.m_Owner),
          m_RecursionCount(other.m_RecursionCount)
    {
    }

    ~CriticalSection()
    {
    }

    void Enter()
    {
        HANDLE old_owner = m_Owner;
        HANDLE new_owner = GetCurrentThread();

        if (new_owner != old_owner)
        {
            s64 current = _InterlockedIncrement64(&m_CurrentThreadID) - 1;
            while (current != m_NextThreadID);
            _InterlockedExchangePointer(&m_Owner, new_owner);
        }
        _InterlockedIncrement64(&m_RecursionCount);
    }

    void Leave()
    {
        if (_InterlockedDecrement64(&m_RecursionCount) == 0)
        {
            _InterlockedExchangePointer(&m_Owner, null);
            _InterlockedIncrement64(&m_NextThreadID);
        }
    }

    constexpr bool Waitable() const { return false; }

    CriticalSection& operator=(CriticalSection&& other)
    {
        if (this != &other)
        {
            m_CurrentThreadID = other.m_CurrentThreadID;
            m_NextThreadID    = other.m_NextThreadID;
            m_Owner           = other.m_Owner;
            m_RecursionCount  = other.m_RecursionCount;
        }
        return *this;
    }

private:
    CriticalSection(const CriticalSection&)            = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;

private:
    volatile s64    m_CurrentThreadID;
    volatile s64    m_NextThreadID;
    volatile HANDLE m_Owner;
    volatile s64    m_RecursionCount;
};
