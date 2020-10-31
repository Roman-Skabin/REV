//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"
#include "tools/logger.h"
#include "tools/function.hpp"

class ENGINE_API WorkQueue final
{
public:
    using WorkType = Function<void()>;

    static WorkQueue *Create(const Logger& logger);
    static WorkQueue *Get();

private:
    WorkQueue(const Logger& logger);

public:
    ~WorkQueue();

    void AddWork(const WorkType& work);
    void Wait();

private:
    friend u32 WINAPI ThreadProc(void *arg);

    void *operator new(size_t)    { return Memory::Get()->PushToPA<WorkQueue>(); }
    void  operator delete(void *) {}

    WorkQueue(const WorkQueue&) = delete;
    WorkQueue(WorkQueue&&)      = delete;

    WorkQueue& operator=(const WorkQueue&) = delete;
    WorkQueue& operator=(WorkQueue&&)      = delete;

private:
    enum
    {
        MAX_THREADS = 64 - 1,

        MAX_WORKS_BYTES = AlignUp(MAX_THREADS * sizeof(WorkType), CACHE_LINE_SIZE),
        MAX_WORKS       = MAX_WORKS_BYTES / sizeof(WorkType),
    };

    HANDLE       m_Semaphore;
    volatile s32 m_CompletionGoal;
    volatile s32 m_EntriesCompleted;
    volatile s32 m_NextEntryToRead;
    volatile s32 m_NextEntryToWrite;
    WorkType     m_Works[MAX_WORKS];

    static WorkQueue *s_WorkQueue;
};
