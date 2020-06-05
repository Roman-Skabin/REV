//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#define WORK_QUEUE_ENTRY_PROC(name) void name(u32 thread_id, void *arg)
typedef WORK_QUEUE_ENTRY_PROC(WorkQueueEntryProc);

typedef struct WorkQueue WorkQueue;

CENGINE_FUN WorkQueue *CreateWorkQueue(Engine *engine);

CENGINE_FUN void AddWorkQueueEntry(WorkQueue *queue, WorkQueueEntryProc *Proc, void *arg);
CENGINE_FUN void WaitForWorkQueue(WorkQueue *queue);
