//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/work_queue.h"
#include "cengine.h"

#define THREAD_PROC(name) u32 WINAPI name(LPVOID param)

typedef struct WorkQueueEntry
{
    WorkQueueEntryProc *Proc;
    void               *arg;
} WorkQueueEntry;

enum
{
    MAX_THREADS = 64,

    MAX_ENTRIES_BYTES = ALIGN_UP(MAX_THREADS * sizeof(WorkQueueEntry), CACHE_LINE_SIZE),
    MAX_ENTRIES       = MAX_ENTRIES_BYTES / sizeof(WorkQueueEntry),
};

struct WorkQueue
{
             HANDLE          semaphore;
    volatile s32             completion_goal;
    volatile s32             entries_completed;
    volatile s32             next_entry_to_read;
    volatile s32             next_entry_to_write;
             WorkQueueEntry  entries[MAX_ENTRIES];
};

typedef struct WorkQueueThread
{
    WorkQueue *queue;
    u32        id;
} WorkQueueThread;

internal THREAD_PROC(ThreadProc)
{
    DebugResult(SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST));

    WorkQueueThread *thread = (WorkQueueThread *)param;
    while (true)
    {
        s32 old_next_entry_to_read = thread->queue->next_entry_to_read;
        s32 new_next_entry_to_read = (old_next_entry_to_read + 1) % MAX_ENTRIES;

        if (old_next_entry_to_read != thread->queue->next_entry_to_write)
        {
            s32 old = _InterlockedCompareExchange_HLERelease(&thread->queue->next_entry_to_read,
                                                             new_next_entry_to_read,
                                                             old_next_entry_to_read);

            if (old == old_next_entry_to_read)
            {
                WorkQueueEntry *entry = thread->queue->entries + old;
                entry->Proc(thread->id, entry->arg);
                _InterlockedIncrement(&thread->queue->entries_completed);
            }
        }
        else
        {
            while (WaitForSingleObjectEx(thread->queue->semaphore, INFINITE, FALSE) != WAIT_OBJECT_0)
            {
            }
        }
    }
    return 0;
}

WorkQueue *CreateWorkQueue(Engine *engine)
{
    WorkQueue *queue = PushToPA(WorkQueue, engine->memory, 1);

#if 1
    SYSTEM_INFO info;
    GetNativeSystemInfo(&info);

    s32 cpu_virtual_threads_count = info.dwNumberOfProcessors - 1; // minus main thread
    if (cpu_virtual_threads_count <= 0) cpu_virtual_threads_count = 1;
#else
    cpu_virtual_threads_count = MAX_THREADS;
#endif

    WorkQueueThread *threads = PushToPA(WorkQueueThread, engine->memory, cpu_virtual_threads_count);

    queue->semaphore = CreateSemaphoreExA(0, 0, cpu_virtual_threads_count, 0, 0, SEMAPHORE_ALL_ACCESS);
    Check(queue->semaphore);

    for (s32 i = 0; i < cpu_virtual_threads_count; ++i)
    {
        WorkQueueThread *thread = threads + i;
        thread->id    = i + 1;
        thread->queue = queue;
        DebugResult(CloseHandle(CreateThread(0, 0, ThreadProc, thread, 0, 0)));
        LogInfo(&engine->logger, "Thread was created: id = %I32u, queue = 0x%p", thread->id, thread->queue);
    }

    LogSuccess(&engine->logger, "Work queue was created");
    LogInfo(&engine->logger, "Additional threads count = %I32u", cpu_virtual_threads_count);
    return queue;
}

void AddWorkQueueEntry(WorkQueue *queue, WorkQueueEntryProc *Proc, void *arg)
{
    while (true)
    {
        s32 old_next_entry_to_write = queue->next_entry_to_write;
        s32 new_next_entry_to_write = (old_next_entry_to_write + 1) % MAX_ENTRIES;

        if (new_next_entry_to_write != queue->next_entry_to_read)
        {
            WorkQueueEntry *entry = queue->entries + old_next_entry_to_write;
            entry->Proc           = Proc;
            entry->arg            = arg;

            s32 old = _InterlockedCompareExchange_HLERelease(&queue->next_entry_to_write,
                                                             new_next_entry_to_write,
                                                             old_next_entry_to_write);

            if (old == old_next_entry_to_write)
            {
                _InterlockedIncrement(&queue->completion_goal);
                ReleaseSemaphore(queue->semaphore, 1, 0);
            }

            break;
        }
    }
}

void WaitForWorkQueue(WorkQueue *queue)
{
    while (queue->entries_completed < queue->completion_goal)
    {
        s32 old_next_entry_to_read = queue->next_entry_to_read;
        s32 new_next_entry_to_read = (old_next_entry_to_read + 1) % MAX_ENTRIES;

        if (old_next_entry_to_read != queue->next_entry_to_write)
        {
            s32 old = _InterlockedCompareExchange_HLERelease(&queue->next_entry_to_read,
                                                             new_next_entry_to_read,
                                                             old_next_entry_to_read);

            if (old == old_next_entry_to_read)
            {
                WorkQueueEntry *entry = queue->entries + old;
                entry->Proc(0, entry->arg);
                _InterlockedIncrement(&queue->entries_completed);
            }
        }
    }

    queue->entries_completed = 0;
    queue->completion_goal   = 0;
}
