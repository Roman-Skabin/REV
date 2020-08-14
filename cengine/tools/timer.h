//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/logger.h"

typedef struct Timer
{
    s64 ticks_per_second;
    s64 initial_ticks;

    s64 ticks;
    s64 delta_ticks;

    s64 stop_begin;
    s64 stop_duration;
    s64 stop_last_duration;

    f32 seconds;
    f32 delta_seconds;
    f32 total_seconds;

    b32 stopped;

    char name[256];
} Timer;

// @NOTE(Roman): It would be better if you pass non-null name_len,
//               otherwise strlen will be used.
CENGINE_FUN void CreateTimer(
    in  const char *name,
    opt u64         name_len,
    out Timer      *timer
);

CENGINE_FUN void StepTimer(
    in Timer *timer
);

CENGINE_FUN void StartTimer(
    in Timer *timer
);

CENGINE_FUN void StopTimer(
    in Timer *timer
);

// @NOTE(Roman): It would be better if you pass non-null name_len,
//               otherwise strlen will be used.
CENGINE_FUN void StartProfiling(
    in  const char *name,
    opt u64         name_len,
    out Timer      *timer
);

CENGINE_FUN void StopProfiling(
    in Timer  *timer,
    in Logger *logger
);
