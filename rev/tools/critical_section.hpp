// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/common.h"

namespace REV
{
    template<bool waitable>
    class CriticalSection;

    template<>
    class CriticalSection<true>
    {
    public:
        REV_INLINE CriticalSection()
            : m_Semaphore(CreateSemaphoreExA(null, 1, 1, "REV::CriticalSection Semaphore", 0, SEMAPHORE_ALL_ACCESS)),
              m_RecursionCount(0),
              m_Owner(0)
        {
            REV_CHECK(m_Semaphore);
        }

        REV_INLINE CriticalSection(CriticalSection&& other)
            : m_Semaphore(other.m_Semaphore),
              m_RecursionCount(other.m_RecursionCount),
              m_Owner(other.m_Owner)
        {
            other.m_Semaphore = null;
        }

        REV_INLINE ~CriticalSection()
        {
            if (m_Semaphore)
            {
                REV_DEBUG_RESULT(CloseHandle(m_Semaphore));
                m_Semaphore = null;
            }
        }

        void Enter()
        {
            u32 old_owner = m_Owner;
            u32 new_owner = GetCurrentThreadId();

            if (new_owner != old_owner)
            {
                // @NOTE(Roman): If more than one thread is waiting on a semaphore, a waiting thread is selected.
                //               Do not assume a first-in, first-out (FIFO) order.
                //               External events such as kernel-mode APCs can change the wait order.
                u32 wait_result = WaitForSingleObjectEx(m_Semaphore, INFINITE, false);
                REV_CHECK(wait_result == WAIT_OBJECT_0);

                _InterlockedExchange(&m_Owner, new_owner);
            }
            _InterlockedIncrement64(&m_RecursionCount);
        }

        REV_INLINE void Leave()
        {
            if (_InterlockedDecrement64(&m_RecursionCount) == 0)
            {
                _InterlockedExchange(&m_Owner, 0);
                REV_DEBUG_RESULT(ReleaseSemaphore(m_Semaphore, 1, null));
            }
        }

        constexpr REV_INLINE bool Waitable() const { return true; }

        REV_INLINE CriticalSection& operator=(CriticalSection&& other)
        {
            if (this != &other)
            {
                REV_DEBUG_RESULT(CloseHandle(m_Semaphore));

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
        volatile s64    m_RecursionCount;
        volatile u32    m_Owner;
    };

    template<>
    class CriticalSection<false>
    {
    public:
        REV_INLINE CriticalSection()
            : m_CurrentThreadID(0),
              m_NextThreadID(0),
              m_RecursionCount(0),
              m_Owner(0)
        {
        }

        REV_INLINE CriticalSection(CriticalSection&& other)
            : m_CurrentThreadID(other.m_CurrentThreadID),
              m_NextThreadID(other.m_NextThreadID),
              m_RecursionCount(other.m_RecursionCount),
              m_Owner(other.m_Owner)
        {
            other.m_CurrentThreadID = 0;
            other.m_NextThreadID    = 0;
            other.m_RecursionCount  = 0;
            other.m_Owner           = 0;
        }

        REV_INLINE ~CriticalSection()
        {
        }

        void Enter()
        {
            u32 old_owner = m_Owner;
            u32 new_owner = GetCurrentThreadId();

            if (new_owner != old_owner)
            {
                s64 current = _InterlockedIncrement64(&m_CurrentThreadID) - 1;
                while (current != m_NextThreadID)
                {
                }
                _InterlockedExchange(&m_Owner, new_owner);
            }
            _InterlockedIncrement64(&m_RecursionCount);
        }

        REV_INLINE void Leave()
        {
            if (_InterlockedDecrement64(&m_RecursionCount) == 0)
            {
                _InterlockedExchange(&m_Owner, 0);
                _InterlockedIncrement64(&m_NextThreadID);
            }
        }

        constexpr REV_INLINE bool Waitable() const { return false; }

        REV_INLINE CriticalSection& operator=(CriticalSection&& other)
        {
            if (this != &other)
            {
                m_CurrentThreadID = other.m_CurrentThreadID;
                m_NextThreadID    = other.m_NextThreadID;
                m_RecursionCount  = other.m_RecursionCount;
                m_Owner           = other.m_Owner;

                other.m_CurrentThreadID = 0;
                other.m_NextThreadID    = 0;
                other.m_RecursionCount  = 0;
                other.m_Owner           = 0;
            }
            return *this;
        }

    private:
        CriticalSection(const CriticalSection&)            = delete;
        CriticalSection& operator=(const CriticalSection&) = delete;

    private:
        volatile s64 m_CurrentThreadID;
        volatile s64 m_NextThreadID;
        volatile s64 m_RecursionCount;
        volatile u32 m_Owner;
    };
}
