//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/timer.h"

internal INLINE void QPF(s64& val) { QueryPerformanceFrequency(cast<LARGE_INTEGER *>(&val)); }
internal INLINE void QPC(s64& val) { QueryPerformanceCounter(cast<LARGE_INTEGER *>(&val));   }

//
// Timer
//

Timer::Timer(in const char *name, in_opt u64 name_len)
{
    Check(name);

    if (!name_len) name_len = strlen(name);
    CheckM(name_len && name_len < ArrayCount(m_Name),
           "Timer name is too long. Max length is %I64u",
           ArrayCount(m_Name) - 1);

    ZeroMemory(this, sizeof(Timer));

    QPF(m_TicksPerSecond);
    QPC(m_InitialTicks);

    m_Stopped   = true;
    m_StopBegin = m_InitialTicks;

    CopyMemory(m_Name, name, name_len);
    m_Name[name_len] = '\0';
}

Timer::Timer(const Timer& other)
{
    CopyMemory(this, &other, sizeof(Timer));
}

Timer::Timer(Timer&& other) noexcept
{
    CopyMemory(this, &other, sizeof(Timer));
}

Timer::~Timer()
{
    ZeroMemory(this, sizeof(Timer));
}

void Timer::Tick()
{
    s64 cur_ticks;
    QPC(cur_ticks);
    cur_ticks -= m_InitialTicks + m_StopDuration;

    if (!m_Stopped)
    {
        m_DeltaTicks = cur_ticks - m_Ticks;
        m_Ticks      = cur_ticks;

        m_DeltaSeconds = m_DeltaTicks / cast<f32>(m_TicksPerSecond);
        m_Seconds      = m_Ticks      / cast<f32>(m_TicksPerSecond);
    }

    m_TotalSeconds = (cur_ticks + m_StopDuration) / cast<f32>(m_TicksPerSecond);
}

void Timer::Start()
{
    if (m_Stopped)
    {
        s64 stop_end;
        QPC(stop_end);
        m_StopLastDuration  = stop_end - m_StopBegin;
        m_StopDuration     += m_StopLastDuration;
        m_Stopped           = false;
    }
}

void Timer::Stop()
{
    if (!m_Stopped)
    {
        QPC(m_StopBegin);
        m_Stopped = true;
    }
}

Timer& Timer::operator=(const Timer& other)
{
    if (this != &other)
    {
        CopyMemory(this, &other, sizeof(Timer));
    }
    return *this;
}

Timer& Timer::operator=(Timer&& other) noexcept
{
    if (this != &other)
    {
        CopyMemory(this, &other, sizeof(Timer));
    }
    return *this;
}

//
// ProfilingTimer
//

ProfilingTimer::ProfilingTimer(in const char *name, in_opt u64 name_len)
    : m_Timer(name, name_len)
{
    m_Timer.Start();
}

ProfilingTimer::~ProfilingTimer()
{
}

void ProfilingTimer::StopProfiling(in const Logger& logger)
{
    m_Timer.Tick();
    m_Timer.Stop();
    logger.LogInfo("%s: time elapsed: %f seconds (%I64d ticks)",
                   m_Timer.m_Name, m_Timer.m_DeltaSeconds, m_Timer.m_DeltaTicks);
}
