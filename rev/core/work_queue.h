//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "memory/memlow.h"
#include "tools/logger.h"
#include "tools/function.hpp"

namespace REV
{
    class REV_API WorkQueue final
    {
    public:
        WorkQueue(const Logger& logger, Arena& arena);
        ~WorkQueue();

        void AddWork(const Function<void()>& callback);
        void Wait();

    private:
        friend u32 WINAPI ThreadProc(void *arg);

        REV_DELETE_CONSTRS_AND_OPS(WorkQueue);

    private:
        enum
        {
            MIN_THREADS = 4,
            MAX_THREADS = 128,

            MIN_WORKS   = MIN_THREADS - 1,
            MAX_WORKS   = __max(MIN_WORKS, MAX_THREADS - 1),
        };

        struct Work
        {
            HANDLE           thread_handle;
            volatile char    callback_is_changing;
            Function<void()> callback;
        };

        struct Header
        {
            HANDLE       semaphore;
            volatile s32 completion_goal;
            volatile s32 entries_completed;
            volatile s32 next_entry_to_read;
            volatile s32 next_entry_to_write;
            s32          works_count;
            Work         works[0];
        };

        Header *m_Header;
    };
}
