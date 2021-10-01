// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "core/work_queue.h"
#include "math/math.h"

namespace REV
{

WorkQueue::WorkQueue(const Logger& logger, Arena& arena, u64 max_simultaneous_works)
    : m_Semaphore(null),
      m_Works(null),
      m_WorksCount(0),
      m_WorksToDo(0),
      m_WorksExecuteIterator(null),
      m_WorksAddIterator(null),
      m_Threads(null),
      m_ThreadsCount(0),
      m_ThreadsCountWaiting(0)
{
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);

    m_WorksCount = Math::max<u64>(max_simultaneous_works, MIN_WORKS);
    // @NOTE(Roman): We exclude Main Thread to achieve better parallelism in WorkQueue::Wait function.
    m_ThreadsCount = Math::clamp<u64>(info.dwNumberOfProcessors - 1, MIN_THREADS, m_WorksCount);

    void *works_and_threads_memory = arena.PushBytes(m_WorksCount * sizeof(Work) + m_ThreadsCount * sizeof(void *));
    m_Works   = cast(Work *, works_and_threads_memory);
    m_Threads = cast(void **, cast(Work *, works_and_threads_memory) + m_WorksCount);

    m_WorksExecuteIterator = m_Works;
    m_WorksAddIterator     = m_Works;

    REV_DEBUG_RESULT(m_Semaphore = CreateSemaphoreExA(null, 0, cast(s32, m_ThreadsCount), null, 0, SEMAPHORE_ALL_ACCESS));

    void **end = m_Threads + m_ThreadsCount;
    for (void **thread = m_Threads; thread < end; ++thread)
    {
        *thread = CreateThread(null, 0, ThreadProc, this, 0, null);
    }

    logger.LogSuccess("Work queue has been created. Additional threads count: ", m_ThreadsCount);
}

WorkQueue::~WorkQueue()
{
    Wait();

    void **end = m_Threads + m_ThreadsCount;
    for (void **thread = m_Threads; thread < end; ++thread)
    {
        REV_DEBUG_RESULT(TerminateThread(*thread, EXIT_SUCCESS));
        REV_DEBUG_RESULT(CloseHandle(*thread));
    }
    m_Threads = null;

    REV_DEBUG_RESULT(CloseHandle(m_Semaphore));
    m_Semaphore = null;

    m_Works = null;
}

void WorkQueue::AddWork(const Function<void()>& proc)
{
    while (true)
    {
        Work *current_works_add_it = m_WorksAddIterator;
        Work *next_works_add_it    = m_Works + ((current_works_add_it - m_Works + 1) % m_WorksCount);

        if (next_works_add_it != m_WorksExecuteIterator)
        {
            if (current_works_add_it == _InterlockedCompareExchangePointer(cast(void *volatile *, &m_WorksAddIterator), next_works_add_it, current_works_add_it))
            {
                _InterlockedOr(cast(volatile s32 *, &current_works_add_it->flags), WORK_FLAG_IS_CHANGING);
                current_works_add_it->proc = proc;
                _InterlockedAnd(cast(volatile s32 *, &current_works_add_it->flags), ~WORK_FLAG_IS_CHANGING);

                _InterlockedIncrement64(&m_WorksToDo);

                if (m_ThreadsCountWaiting && !ReleaseSemaphore(m_Semaphore, 1, null))
                {
                    u32 error_code = GetSysErrorCode();
                    /**/ if (error_code == ERROR_TOO_MANY_POSTS) REV_WARNING_M("ReleaseSemaphore has completed with ERROR_TOO_MANY_POSTS")
                    else if (error_code != ERROR_SUCCESS)        REV_ERROR_M("ReleaseSemaphore has failed")
                }

                break;
            }
        }
    }
}

void WorkQueue::Wait()
{
    while (m_WorksToDo > 0)
    {
        Work *current_works_exec_it = m_WorksExecuteIterator;
        Work *next_works_exec_it    = m_Works + ((current_works_exec_it - m_Works + 1) % m_WorksCount);

        if (current_works_exec_it != m_WorksAddIterator)
        {
            if (current_works_exec_it == _InterlockedCompareExchangePointer(cast(void *volatile *, &m_WorksExecuteIterator), next_works_exec_it, current_works_exec_it))
            {
                while (current_works_exec_it->flags & WORK_FLAG_IS_CHANGING);

                current_works_exec_it->proc();

                _InterlockedDecrement64(&m_WorksToDo);
            }
        }
    }
}

u32 REV_STDCALL ThreadProc(void *arg)
{
    WorkQueue *work_queue = cast(WorkQueue *, arg);
    while (true)
    {
        _InterlockedIncrement64(cast(volatile s64 *, &work_queue->m_ThreadsCountWaiting));
        u32 res = WaitForSingleObjectEx(work_queue->m_Semaphore, INFINITE, false);
        REV_CHECK(res == WAIT_OBJECT_0);
        _InterlockedDecrement64(cast(volatile s64 *, &work_queue->m_ThreadsCountWaiting));

        WorkQueue::Work *current_works_exec_it = work_queue->m_WorksExecuteIterator;
        WorkQueue::Work *next_works_exec_it    = work_queue->m_Works + ((current_works_exec_it - work_queue->m_Works + 1) % work_queue->m_WorksCount);

        if (current_works_exec_it != work_queue->m_WorksAddIterator)
        {
            if (current_works_exec_it == _InterlockedCompareExchangePointer(cast(void *volatile *, &work_queue->m_WorksExecuteIterator), next_works_exec_it, current_works_exec_it))
            {
                while (current_works_exec_it->flags & WorkQueue::WORK_FLAG_IS_CHANGING);

                current_works_exec_it->proc();

                _InterlockedDecrement64(&work_queue->m_WorksToDo);
            }
        }
    }
    return 0;
}

}
