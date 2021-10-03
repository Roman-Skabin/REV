// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "core/work_queue.h"
#include "math/math.h"

#include "platform/windows/syscalls.h"

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
      m_ThreadsCount(0)
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

    Windows::NTSTATUS status = Windows::NtCreateSemaphore(&m_Semaphore, SEMAPHORE_ALL_ACCESS, null, 0, cast(u32, m_ThreadsCount));
    REV_CHECK(status == STATUS_SUCCESS);

    void **end = m_Threads + m_ThreadsCount;
    for (void **thread = m_Threads; thread < end; ++thread)
    {
        REV_DEBUG_RESULT(*thread = CreateThread(null, 0, ThreadProc, this, 0, null));
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

    Windows::NTSTATUS status = Windows::NtClose(m_Semaphore);
    REV_CHECK(status == STATUS_SUCCESS);
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

                Windows::SEMAPHORE_BASIC_INFORMATION semaphore_basic_info{};
                Windows::NTSTATUS status = Windows::NtQuerySemaphore(m_Semaphore, Windows::SemaphoreBasicInformation, &semaphore_basic_info, sizeof(Windows::SEMAPHORE_BASIC_INFORMATION), null);
                REV_CHECK(status == STATUS_SUCCESS);

                if (semaphore_basic_info.CurrentCount < semaphore_basic_info.MaximumCount)
                {
                    Windows::NTSTATUS status = Windows::NtReleaseSemaphore(m_Semaphore, 1, null);
                    REV_CHECK(status == STATUS_SUCCESS);
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
        else
        {
            Windows::NTSTATUS status = Windows::NtWaitForSingleObject(work_queue->m_Semaphore, false, null);
            REV_CHECK(status == STATUS_WAIT_0);
        }
    }
    return 0;
}

}
