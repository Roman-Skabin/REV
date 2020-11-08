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
        : m_Semaphore(CreateSemaphoreExA(null, 1, 1, "CriticalSection Semaphore", 0, SEMAPHORE_ALL_ACCESS)),
          m_Owner(null),
          m_RecursionCount(0)
    {
    }

    CriticalSection(CriticalSection&& other)
        : m_Semaphore(other.m_Semaphore),
          m_Owner(other.m_Owner),
          m_RecursionCount(other.m_RecursionCount)
    {
        other.m_Semaphore = null;
    }

    ~CriticalSection()
    {
        if (m_Semaphore) CloseHandle(m_Semaphore);
    }

    void Enter()
    {
        HANDLE old_owner = m_Owner;
        HANDLE new_owner = GetCurrentThread();

        if (new_owner != old_owner)
        {
            // @NOTE(Roman): If more than one thread is waiting on a semaphore, a waiting thread is selected.
            //               Do not assume a first-in, first-out (FIFO) order.
            //               External events such as kernel-mode APCs can change the wait order.
            while (WaitForSingleObjectEx(m_Semaphore, INFINITE, false) != WAIT_OBJECT_0);
            _InterlockedExchangePointer(&m_Owner, new_owner);
        }
        _InterlockedIncrement64(&m_RecursionCount);
    }

    void Leave()
    {
        if (_InterlockedDecrement64(&m_RecursionCount) == 0)
        {
            _InterlockedExchangePointer(&m_Owner, null);
            ReleaseSemaphore(m_Semaphore, 1, null);
        }
    }

    constexpr bool Waitable() const { return true; }

    CriticalSection& operator=(CriticalSection&& other)
    {
        if (this != &other)
        {
            m_Semaphore      = other.m_Semaphore;
            m_Owner          = other.m_Owner;
            m_RecursionCount = other.m_RecursionCount;

            other.m_Semaphore = null;
        }
        return *this;
    }

private:
    CriticalSection(const CriticalSection&)            = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;

private:
    volatile HANDLE m_Semaphore;
    volatile HANDLE m_Owner;
    volatile s64    m_RecursionCount;
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
        other.m_CurrentThreadID = 0;
        other.m_NextThreadID    = 0;
        other.m_Owner           = null;
        other.m_RecursionCount  = 0;
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

            other.m_CurrentThreadID = 0;
            other.m_NextThreadID    = 0;
            other.m_Owner           = null;
            other.m_RecursionCount  = 0;
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
