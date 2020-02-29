//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/work_queue.h"
#include "cengine.h"
#include <TlHelp32.h>

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
             Logger         *logger;
             HANDLE          semaphore;
    volatile s16             completion_goal;
    volatile s16             entries_completed;
    volatile s16             next_entry_to_read;
    volatile s16             next_entry_to_write;
             WorkQueueEntry  entries[MAX_ENTRIES];
};

typedef struct WorkQueueThread
{
    WorkQueue *queue;
    u32        id;
} WorkQueueThread;

internal THREAD_PROC(ThreadProc)
{
    WorkQueueThread *thread = (WorkQueueThread *)param;
    while (true)
    {
        s16 old_next_entry_to_read = thread->queue->next_entry_to_read;
        s16 new_next_entry_to_read = (old_next_entry_to_read + 1) % MAX_ENTRIES;

        if (old_next_entry_to_read != thread->queue->next_entry_to_write)
        {
            s16 old = _InterlockedCompareExchange16(&thread->queue->next_entry_to_read,
                                                      new_next_entry_to_read,
                                                      old_next_entry_to_read);

            if (old == old_next_entry_to_read)
            {
                WorkQueueEntry *entry = thread->queue->entries + old;
                entry->Proc(thread->id, entry->arg);
                _InterlockedIncrement16(&thread->queue->entries_completed);
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

u32 GetCurrentThreadCount()
{
    DWORD  id       = GetCurrentProcessId();
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    Check(snapshot != INVALID_HANDLE_VALUE);

    PROCESSENTRY32 entry = {0};
    entry.dwSize = sizeof(PROCESSENTRY32);

    b32 ret = false;
    ret = Process32First(snapshot, &entry);
    while (ret && entry.th32ProcessID != id)
    {
        ret = Process32Next(snapshot, &entry);
    }

    DebugResult(b32, CloseHandle(snapshot));

    Check(entry.cntThreads);
    return entry.cntThreads;
}

WorkQueue *CreateWorkQueue(EngineState *state)
{
    WorkQueue *queue = PushToPA(WorkQueue, &state->memory, 1);
    queue->logger    = &state->logger;

    SYSTEM_INFO info;
    GetSystemInfo(&info);

    s32 threads_count = info.dwNumberOfProcessors - GetCurrentThreadCount();
    if (state->window.closed && threads_count > 0) --threads_count;
    if (threads_count <= 0                       ) threads_count = 1;

    WorkQueueThread *threads = PushToPA(WorkQueueThread, &state->memory, threads_count);

    queue->semaphore = CreateSemaphoreExA(0, 0, threads_count, 0, 0, SEMAPHORE_ALL_ACCESS);
    Check(queue->semaphore);

    for (s32 i = 0; i < threads_count; ++i)
    {
        WorkQueueThread *thread = threads + i;
        thread->id    = i + 1;
        thread->queue = queue;
        DebugResult(b32, CloseHandle(CreateThread(0, 0, ThreadProc, thread, 0, 0)));
        Log(queue->logger, "Thread was created: id = %I32u, queue = 0x%p", thread->id, thread->queue);
    }

    Success(queue->logger, "Work queue was created");
    Log(queue->logger, "Additional threads count = %I32u", threads_count);
    return queue;
}

void AddWorkQueueEntry(WorkQueue *queue, WorkQueueEntryProc *Proc, void *arg)
{
    while (true)
    {
        s16 old_next_entry_to_write = queue->next_entry_to_write;
        s16 new_next_entry_to_write = (old_next_entry_to_write + 1) % MAX_ENTRIES;

        if (new_next_entry_to_write != queue->next_entry_to_read)
        {
            WorkQueueEntry *entry = queue->entries + old_next_entry_to_write;
            entry->Proc           = Proc;
            entry->arg            = arg;

            s16 old = _InterlockedCompareExchange16(&queue->next_entry_to_write,
                                                      new_next_entry_to_write,
                                                      old_next_entry_to_write);

            if (old == old_next_entry_to_write)
            {
                _InterlockedIncrement16(&queue->completion_goal);
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
        s16 old_next_entry_to_read = queue->next_entry_to_read;
        s16 new_next_entry_to_read = (old_next_entry_to_read + 1) % MAX_ENTRIES;

        if (old_next_entry_to_read != queue->next_entry_to_write)
        {
            s16 old = _InterlockedCompareExchange16(&queue->next_entry_to_read,
                                                      new_next_entry_to_read,
                                                      old_next_entry_to_read);

            if (old == old_next_entry_to_read)
            {
                WorkQueueEntry *entry = queue->entries + old;
                entry->Proc(0, entry->arg);
                _InterlockedIncrement16(&queue->entries_completed);
            }
        }
    }

    queue->entries_completed = 0;
    queue->completion_goal   = 0;
}
