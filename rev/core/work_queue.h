//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "memory/memlow.h"
#include "tools/logger.h"
#include "tools/function.hpp"

namespace REV
{
    // @TODO(Roman): Fix the destructor: terminate threads and close semaphore.
    //               Work entries should hold thread handles to terminate them in destructor.
    class REV_API WorkQueue final
    {
    public:
        using Work = Function<void()>;

        WorkQueue(const Logger& logger);
        ~WorkQueue();

        void AddWork(const Work& work);
        void Wait();

    private:
        friend u32 WINAPI ThreadProc(void *arg);

        REV_DELETE_CONSTRS_AND_OPS(WorkQueue);

    private:
        enum
        {
            MAX_THREADS = 64 - 1,

            MAX_WORKS_BYTES = AlignUp(MAX_THREADS * sizeof(Work), CACHE_LINE_SIZE),
            MAX_WORKS       = MAX_WORKS_BYTES / sizeof(Work),
        };

        HANDLE       m_Semaphore;
        volatile s32 m_CompletionGoal;
        volatile s32 m_EntriesCompleted;
        volatile s32 m_NextEntryToRead;
        volatile s32 m_NextEntryToWrite;
        Work         m_Works[MAX_WORKS];
    };
}
