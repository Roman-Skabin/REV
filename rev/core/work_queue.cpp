//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "core/work_queue.h"
#include "math/math.h"

namespace REV
{

WorkQueue::WorkQueue(const Logger& logger, Arena& arena)
    : m_Header(null)
{
    SYSTEM_INFO info{};
    GetNativeSystemInfo(&info);

    // @NOTE(Roman): We exclude Main Thread to achieve better parallelism in WorkQueue::Wait function.
    s32 works_count = Math::clamp<s32>(cast(s32, info.dwNumberOfProcessors - 1), MIN_WORKS, MAX_WORKS);

    m_Header              = cast(Header *, arena.PushBytesAligned(REV_StructFieldOffset(Header, works) + works_count * sizeof(Work), CACHE_LINE_SIZE));
    m_Header->semaphore   = CreateSemaphoreExA(null, 0, works_count, null, 0, SEMAPHORE_ALL_ACCESS);
    m_Header->works_count = works_count;

    Work *end = m_Header->works + m_Header->works_count;
    for (Work *work = m_Header->works; work < end; ++work)
    {
        work->thread_handle = CreateThread(null, 0, ThreadProc, m_Header, 0, null);
    }

    logger.LogSuccess("Work queue has been created. Additional threads count: ", m_Header->works_count);
}

WorkQueue::~WorkQueue()
{
    if (m_Header)
    {
        Wait();

        Work *end = m_Header->works + m_Header->works_count;
        for (Work *work = m_Header->works; work < end; ++work)
        {
            REV_DEBUG_RESULT(TerminateThread(work->thread_handle, 0));
            REV_DEBUG_RESULT(CloseHandle(work->thread_handle));
            work->thread_handle = null;
        }

        REV_DEBUG_RESULT(CloseHandle(m_Header->semaphore));
        m_Header->semaphore = null;

        m_Header = null;
    }
}

void WorkQueue::AddWork(const Function<void()>& callback)
{
    while (true)
    {
        s32 old_next_entry_to_write = m_Header->next_entry_to_write;
        s32 new_next_entry_to_write = (old_next_entry_to_write + 1) % m_Header->works_count;

        if (new_next_entry_to_write != m_Header->next_entry_to_read)
        {
            s32 old = _InterlockedCompareExchange(&m_Header->next_entry_to_write,
                                                  new_next_entry_to_write,
                                                  old_next_entry_to_write);

            if (old == old_next_entry_to_write)
            {
                m_Header->works[old_next_entry_to_write].callback = callback;

                _InterlockedIncrement(&m_Header->completion_goal);
                REV_DEBUG_RESULT(ReleaseSemaphore(m_Header->semaphore, 1, null));

                break;
            }
        }
    }
}

void WorkQueue::Wait()
{
    while (m_Header->entries_completed < m_Header->completion_goal)
    {
        s32 old_next_entry_to_read = m_Header->next_entry_to_read;
        s32 new_next_entry_to_read = (old_next_entry_to_read + 1) % m_Header->works_count;

        if (old_next_entry_to_read != m_Header->next_entry_to_write)
        {
            s32 old = _InterlockedCompareExchange(&m_Header->next_entry_to_read,
                                                  new_next_entry_to_read,
                                                  old_next_entry_to_read);

            if (old == old_next_entry_to_read)
            {
                m_Header->works[old].callback();
                _InterlockedIncrement(&m_Header->entries_completed);
            }
        }
    }

    m_Header->entries_completed = 0;
    m_Header->completion_goal   = 0;
}

u32 WINAPI ThreadProc(void *arg)
{
    WorkQueue::Header *header = cast(WorkQueue::Header *, arg);
    while (true)
    {
        s32 old_next_entry_to_read = header->next_entry_to_read;
        s32 new_next_entry_to_read = (old_next_entry_to_read + 1) % header->works_count;

        if (old_next_entry_to_read != header->next_entry_to_write)
        {
            s32 old = _InterlockedCompareExchange(&header->next_entry_to_read,
                                                  new_next_entry_to_read,
                                                  old_next_entry_to_read);

            if (old == old_next_entry_to_read)
            {
                header->works[old].callback();
                _InterlockedIncrement(&header->entries_completed);
            }
        }
        else
        {
            u32 res = WaitForSingleObjectEx(header->semaphore, INFINITE, false);
            REV_CHECK(res == WAIT_OBJECT_0);
        }
    }
    return 0;
}

}
