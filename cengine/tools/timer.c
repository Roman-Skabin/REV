//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/timer.h"

#define QPF(s64_val) QueryPerformanceFrequency(cast(LARGE_INTEGER *, &(s64_val)))
#define QPC(s64_val) QueryPerformanceCounter(cast(LARGE_INTEGER *, &(s64_val)))

void CreateTimer(
    in  const char *name,
    opt u64         name_len,
    out Timer      *timer)
{
    Check(name);
    Check(timer);

    if (!name_len) name_len = strlen(name);
    CheckM(name_len && name_len < ArrayCount(timer->name),
           "Timer name is too long. Max length is %I64u",
           ArrayCount(timer->name) - 1);

    ZeroMemory(timer, sizeof(Timer));

    QPF(timer->ticks_per_second);
    QPC(timer->initial_ticks);

    timer->stopped    = true;
    timer->stop_begin = timer->initial_ticks;

    CopyMemory(timer->name, name, name_len);
    timer->name[name_len] = '\0';
}

void StepTimer(
    in Timer *timer)
{
    Check(timer);

    s64 cur_ticks;
    QPC(cur_ticks);
    cur_ticks -= timer->initial_ticks + timer->stop_duration;

    if (!timer->stopped)
    {
        timer->delta_ticks = cur_ticks - timer->ticks;
        timer->ticks       = cur_ticks;

        timer->delta_seconds = timer->delta_ticks / cast(f32, timer->ticks_per_second);
        timer->seconds       = timer->ticks       / cast(f32, timer->ticks_per_second);
    }

    timer->total_seconds = (cur_ticks + timer->stop_duration) / cast(f32, timer->ticks_per_second);
}

void StartTimer(
    in Timer *timer)
{
    Check(timer);

    if (timer->stopped)
    {
        s64 stop_end;
        QPC(stop_end);
        timer->stop_last_duration  = stop_end - timer->stop_begin;
        timer->stop_duration      += timer->stop_last_duration;
        timer->stopped             = false;
    }
}

void StopTimer(
    in Timer *timer)
{
    Check(timer);

    if (!timer->stopped)
    {
        QPC(timer->stop_begin);
        timer->stopped = true;
    }
}

void StartProfiling(
    in  const char *name,
    opt u64         name_len,
    out Timer      *timer)
{
    Check(name);
    Check(timer);

    CreateTimer(name, name_len, timer);
    StartTimer(timer);
}

void StopProfiling(
    in Timer  *timer,
    in Logger *logger)
{
    Check(timer);
    Check(logger);

    StepTimer(timer);
    LogInfo(logger, "%s: time elapsed: %f seconds (%I64d ticks)",
            timer->name, timer->delta_seconds, timer->delta_ticks);
    ZeroMemory(timer, sizeof(Timer));
}
