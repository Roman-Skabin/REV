//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"
#include "tools/logger.h"

#define WORK_QUEUE_ENTRY_PROC(name) void name(u32 thread_id, void *arg)
typedef WORK_QUEUE_ENTRY_PROC(WorkQueueEntryProc);

struct WorkQueueEntry
{
    WorkQueueEntryProc *Proc;
    void               *arg;
};

enum
{
    MAX_THREADS = 16,

    MAX_ENTRIES_BYTES = AlignUp(MAX_THREADS * sizeof(WorkQueueEntry), CACHE_LINE_SIZE),
    MAX_ENTRIES       = MAX_ENTRIES_BYTES / sizeof(WorkQueueEntry),
};

// @TODO(Roman): Any function to AddEntry

class ENGINE_IMPEXP WorkQueue final
{
public:
    static WorkQueue *Create(const Logger& logger);
    static WorkQueue *Get();

private:
    WorkQueue(const Logger& logger);

public:
    WorkQueue(WorkQueue&& other) noexcept;

    ~WorkQueue();

    void AddEntry(WorkQueueEntryProc *Proc, void *arg);
    void Wait();

    WorkQueue& operator=(WorkQueue&&) noexcept;

private:
    friend u32 WINAPI ThreadProc(void *arg);

    WorkQueue(const WorkQueue&) = delete;
    WorkQueue& operator=(const WorkQueue&) = delete;

private:
    HANDLE         m_Semaphore;
    volatile s32   m_CompletionGoal;
    volatile s32   m_EntriesCompleted;
    volatile s32   m_NextEntryToRead;
    volatile s32   m_NextEntryToWrite;
    WorkQueueEntry m_Entries[MAX_ENTRIES];

    static WorkQueue *s_WorkQueue;
};
