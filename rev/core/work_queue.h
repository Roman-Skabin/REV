// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "memory/memlow.h"
#include "tools/logger.h"
#include "tools/function.hpp"

namespace REV
{
    class REV_API WorkQueue final
    {
    public:
        WorkQueue(const Logger& logger, Arena& arena, u64 max_simultaneous_works = 16);
        ~WorkQueue();

        void AddWork(const Function<void()>& proc);
        void Wait();

    private:
        friend u32 REV_STDCALL ThreadProc(void *arg);

        REV_DELETE_CONSTRS_AND_OPS(WorkQueue);

    private:
        enum : u64
        {
            MIN_THREADS =   4 - 1,
            MAX_THREADS = 128 - 1,

            MIN_WORKS   = MIN_THREADS + 1,
        };

    public:
        enum WORK_FLAG : s32
        {
            WORK_FLAG_NONE         = 0,
            WORK_FLAG_IS_CHANGING  = 1 << 0,
        };

    private:
        struct Work
        {
            volatile WORK_FLAG flags;
            Function<void()>   proc;
        };

        void            *m_Semaphore;

        Work            *m_Works;
        u64              m_WorksCount;
        volatile s64     m_WorksToDo;
        Work *volatile   m_WorksExecuteIterator;
        Work *volatile   m_WorksAddIterator;

        void           **m_Threads;
        u64              m_ThreadsCount;
    };
    REV_ENUM_OPERATORS(WorkQueue::WORK_FLAG);
}
