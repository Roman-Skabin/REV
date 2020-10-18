//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/work_queue.h"

typedef struct WorkQueueThread
{
    WorkQueue *queue;
    u32        id;
} WorkQueueThread;

WorkQueue *WorkQueue::s_WorkQueue = null;

WorkQueue *WorkQueue::Create(const Logger& logger)
{
    CheckM(!s_WorkQueue, "Work queue is already created. Use WorkQueue::Get() function instead");
    s_WorkQueue  = Memory::Get()->PushToPA<WorkQueue>();
    *s_WorkQueue = WorkQueue(logger);
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
      m_Entries{null}
{
    SYSTEM_INFO info = {0};
    GetNativeSystemInfo(&info);

    s32 cpu_virtual_threads_count = info.dwNumberOfProcessors - 1; // minus main thread
    if (cpu_virtual_threads_count <= 0) cpu_virtual_threads_count = 1;

    WorkQueueThread *threads = Memory::Get()->PushToPA<WorkQueueThread>(cpu_virtual_threads_count);

    m_Semaphore = CreateSemaphoreExA(0, 0, cpu_virtual_threads_count, 0, 0, SEMAPHORE_ALL_ACCESS);
    Check(m_Semaphore);

    for (s32 i = 0; i < cpu_virtual_threads_count; ++i)
    {
        WorkQueueThread *thread = threads + i;
        thread->id    = i + 1;
        thread->queue = this;
        DebugResult(CloseHandle(CreateThread(null, 0, ThreadProc, thread, 0, null)));
        logger.LogInfo("Thread has been created: id = %I32u, queue = 0x%p", thread->id, thread->queue);
    }

    logger.LogSuccess("Work queue has been created");
    logger.LogInfo("Additional threads count = %I32u", cpu_virtual_threads_count);
}

WorkQueue::WorkQueue(WorkQueue&& other) noexcept
{
    CopyMemory(m_Entries, &other, sizeof(WorkQueue));
    ZeroMemory(&other, sizeof(WorkQueue));
}

WorkQueue::~WorkQueue()
{
    if (m_Semaphore) ZeroMemory(this, sizeof(WorkQueue));
}

void WorkQueue::AddEntry(WorkQueueEntryProc *Proc, void *arg)
{
    while (true)
    {
        s32 old_m_NextEntryToWrite = m_NextEntryToWrite;
        s32 new_m_NextEntryToWrite = (old_m_NextEntryToWrite + 1) % MAX_ENTRIES;

        if (new_m_NextEntryToWrite != m_NextEntryToRead)
        {
            WorkQueueEntry *entry = m_Entries + old_m_NextEntryToWrite;
            entry->Proc           = Proc;
            entry->arg            = arg;

            s32 old = _InterlockedCompareExchange(&m_NextEntryToWrite,
                                                  new_m_NextEntryToWrite,
                                                  old_m_NextEntryToWrite);

            if (old == old_m_NextEntryToWrite)
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
        s32 old_m_NextEntryToRead = m_NextEntryToRead;
        s32 new_m_NextEntryToRead = (old_m_NextEntryToRead + 1) % MAX_ENTRIES;

        if (old_m_NextEntryToRead != m_NextEntryToWrite)
        {
            s32 old = _InterlockedCompareExchange(&m_NextEntryToRead,
                                                  new_m_NextEntryToRead,
                                                  old_m_NextEntryToRead);

            if (old == old_m_NextEntryToRead)
            {
                WorkQueueEntry *entry = m_Entries + old;
                entry->Proc(0, entry->arg);
                _InterlockedIncrement(&m_EntriesCompleted);
            }
        }
    }

    m_EntriesCompleted = 0;
    m_CompletionGoal   = 0;
}

WorkQueue& WorkQueue::operator=(WorkQueue&& other) noexcept
{
    if (this != &other)
    {
        CopyMemory(this, &other, sizeof(WorkQueue));
        ZeroMemory(&other, sizeof(WorkQueue));
    }
    return *this;
}

u32 WINAPI ThreadProc(void *arg)
{
    WorkQueueThread *thread = (WorkQueueThread *)arg;
    while (true)
    {
        s32 old_m_NextEntryToRead = thread->queue->m_NextEntryToRead;
        s32 new_m_NextEntryToRead = (old_m_NextEntryToRead + 1) % MAX_ENTRIES;

        if (old_m_NextEntryToRead != thread->queue->m_NextEntryToWrite)
        {
            s32 old = _InterlockedCompareExchange(&thread->queue->m_NextEntryToRead,
                                                  new_m_NextEntryToRead,
                                                  old_m_NextEntryToRead);

            if (old == old_m_NextEntryToRead)
            {
                WorkQueueEntry *entry = thread->queue->m_Entries + old;
                entry->Proc(thread->id, entry->arg);
                _InterlockedIncrement(&thread->queue->m_EntriesCompleted);
            }
        }
        else
        {
            while (WaitForSingleObjectEx(thread->queue->m_Semaphore, INFINITE, false) != WAIT_OBJECT_0)
            {
            }
        }
    }
    return 0;
}
