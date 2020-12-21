//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/timer.h"

namespace REV
{

REV_INTERNAL REV_INLINE void QPF(s64& val) { QueryPerformanceFrequency(cast<LARGE_INTEGER *>(&val)); }
REV_INTERNAL REV_INLINE void QPC(s64& val) { QueryPerformanceCounter(cast<LARGE_INTEGER *>(&val));   }

REV_INTERNAL REV_INLINE s64 QPF() { s64 val = 0; QueryPerformanceFrequency(cast<LARGE_INTEGER *>(&val)); return val; }
REV_INTERNAL REV_INLINE s64 QPC() { s64 val = 0; QueryPerformanceCounter(cast<LARGE_INTEGER *>(&val));   return val; }

//
// Timer
//

Timer::Timer(const StaticString<256>& name)
    : m_TicksPerSecond(QPF()),
      m_InitialTicks(QPC()),
      m_Ticks(0),
      m_DeltaTicks(0),
      m_StopBegin(m_InitialTicks),
      m_StopDuration(0),
      m_StopLastDuration(0),
      m_Seconds(0.0f),
      m_DeltaSeconds(0.0f),
      m_TotalSeconds(0.0f),
      m_Stopped(true),
      m_Name(name)
{
}

Timer::Timer(const Timer& other)
    : m_Name(other.m_Name)
{
    CopyMemory(this, &other, REV_StructFieldOffset(Timer, m_Name));
}

Timer::~Timer()
{
    ZeroMemory(this, REV_StructFieldOffset(Timer, m_Name));
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
        CopyMemory(this, &other, REV_StructFieldOffset(Timer, m_Name));
        m_Name = other.m_Name;
    }
    return *this;
}

//
// ProfilingTimer
//

ProfilingTimer::ProfilingTimer(const StaticString<256>& name)
    : m_Timer(name)
{
    m_Timer.Start();
}

ProfilingTimer::~ProfilingTimer()
{
}

void ProfilingTimer::StopProfiling(const Logger& logger)
{
    m_Timer.Tick();
    m_Timer.Stop();
    logger.LogInfo("%s: time elapsed: %f seconds (%I64d ticks)",
                   m_Timer.m_Name, m_Timer.m_DeltaSeconds, m_Timer.m_DeltaTicks);
}

}
