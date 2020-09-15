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

class ENGINE_IMPEXP WorkQueue final
{
public:
    static WorkQueue *Create(in Memory& memory, const Logger& logger);

private:
    WorkQueue(in Memory& memory, in const Logger& logger);

public:
    WorkQueue(in WorkQueue&& other) noexcept;

    ~WorkQueue();

    void AddEntry(in WorkQueueEntryProc *Proc, in void *arg);
    void Wait();

    WorkQueue& operator=(in WorkQueue&&) noexcept;

private:
    friend u32 WINAPI ThreadProc(in void *arg);

    WorkQueue(in const WorkQueue&) = delete;
    WorkQueue& operator=(in const WorkQueue&) = delete;

private:
    HANDLE         m_Semaphore;
    volatile s32   m_CompletionGoal;
    volatile s32   m_EntriesCompleted;
    volatile s32   m_NextEntryToRead;
    volatile s32   m_NextEntryToWrite;
    WorkQueueEntry m_Entries[MAX_ENTRIES];
};
