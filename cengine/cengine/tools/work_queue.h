//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#ifndef ENGINE_STATE_DEFINED
#define ENGINE_STATE_DEFINED
    typedef struct EngineState EngineState;
#endif

#define WORK_QUEUE_ENTRY_PROC(name) void name(u32 thread_id, void *arg)
typedef WORK_QUEUE_ENTRY_PROC(WorkQueueEntryProc);

typedef struct WorkQueue WorkQueue;

CEXTERN WorkQueue *CreateWorkQueue(EngineState *state);

CEXTERN void AddWorkQueueEntry(WorkQueue *queue, WorkQueueEntryProc *Proc, void *arg);
CEXTERN void WaitForWorkQueue(WorkQueue *queue);
