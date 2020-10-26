//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/work_queue.h"

WorkQueue *WorkQueue::s_WorkQueue = null;

WorkQueue *WorkQueue::Create(const Logger& logger)
{
    CheckM(!s_WorkQueue, "Work queue is already created. Use WorkQueue::Get() function instead");
    s_WorkQueue = new WorkQueue(logger);
    return s_WorkQueue;
}

WorkQueue *WorkQueue::Get()
{
    CheckM(s_WorkQueue, "Work queue is not created yet");
    return s_WorkQueue;
}

WorkQueue::WorkQueue(const Logger& logger)
    : m_Semaphore(null),
      m_CompletionGoal(0),
      m_EntriesCompleted(0),
      m_NextEntryToRead(0),
      m_NextEntryToWrite(0),
      m_Works{null}
{
    SYSTEM_INFO info = {0};
    GetNativeSystemInfo(&info);

    s32 cpu_virtual_threads_count = info.dwNumberOfProcessors - 1; // minus main thread
    if (cpu_virtual_threads_count <= 0) cpu_virtual_threads_count = 1;

    if (cpu_virtual_threads_count > MAX_THREADS) cpu_virtual_threads_count = MAX_THREADS;

    DebugResult(m_Semaphore = CreateSemaphoreExA(0, 0, cpu_virtual_threads_count, 0, 0, SEMAPHORE_ALL_ACCESS));

    for (s32 i = 0; i < cpu_virtual_threads_count; ++i)
    {
        DebugResult(CloseHandle(CreateThread(null, 0, ThreadProc, this, 0, null)));
    }

    logger.LogSuccess("Work queue has been created");
    logger.LogInfo("Additional threads count = %I32u", cpu_virtual_threads_count);
}

WorkQueue::~WorkQueue()
{
    ZeroMemory(this, sizeof(WorkQueue));
}

void WorkQueue::AddWork(const WorkType& work)
{
    while (true)
    {
        s32 old_next_entry_to_write = m_NextEntryToWrite;
        s32 new_next_entry_to_write = (old_next_entry_to_write + 1) % MAX_WORKS;

        if (new_next_entry_to_write != m_NextEntryToRead)
        {
            m_Works[old_next_entry_to_write] = work;

            s32 old = _InterlockedCompareExchange(&m_NextEntryToWrite,
                                                  new_next_entry_to_write,
                                                  old_next_entry_to_write);

            if (old == old_next_entry_to_write)
            {
                _InterlockedIncrement(&m_CompletionGoal);
                ReleaseSemaphore(m_Semaphore, 1, 0);
            }

            break;
        }
    }
}

void WorkQueue::Wait()
{
    while (m_EntriesCompleted < m_CompletionGoal)
    {
        s32 old_next_entry_to_read = m_NextEntryToRead;
        s32 new_next_entry_to_read = (old_next_entry_to_read + 1) % MAX_WORKS;

        if (old_next_entry_to_read != m_NextEntryToWrite)
        {
            s32 old = _InterlockedCompareExchange(&m_NextEntryToRead,
                                                  new_next_entry_to_read,
                                                  old_next_entry_to_read);

            if (old == old_next_entry_to_read)
            {
                m_Works[old]();
                _InterlockedIncrement(&m_EntriesCompleted);
            }
        }
    }

    m_EntriesCompleted = 0;
    m_CompletionGoal   = 0;
}

u32 WINAPI ThreadProc(void *arg)
{
    WorkQueue *work_queue = cast<WorkQueue *>(arg);
    while (true)
    {
        s32 old_next_entry_to_read = work_queue->m_NextEntryToRead;
        s32 new_next_entry_to_read = (old_next_entry_to_read + 1) % WorkQueue::MAX_WORKS;

        if (old_next_entry_to_read != work_queue->m_NextEntryToWrite)
        {
            s32 old = _InterlockedCompareExchange(&work_queue->m_NextEntryToRead,
                                                  new_next_entry_to_read,
                                                  old_next_entry_to_read);

            if (old == old_next_entry_to_read)
            {
                work_queue->m_Works[old]();
                _InterlockedIncrement(&work_queue->m_EntriesCompleted);
            }
        }
        else
        {
            while (WaitForSingleObjectEx(work_queue->m_Semaphore, INFINITE, false) != WAIT_OBJECT_0)
            {
            }
        }
    }
    return 0;
}
