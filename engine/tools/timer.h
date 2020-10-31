//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/logger.h"
#include "tools/static_string.hpp"

class ENGINE_API Timer final
{
public:
    Timer(const StaticString<256>& name);
    Timer(const Timer& other);

    ~Timer();

    void Tick();

    void Start();
    void Stop();

    s64 TicksPerSecond() const { return m_TicksPerSecond; }
    s64 InitialTicks()   const { return m_InitialTicks;   }

    s64 Ticks()      const { return m_Ticks;      }
    s64 DeltaTicks() const { return m_DeltaTicks; }

    s64 StopBegin()        const { return m_StopBegin;        }
    s64 StopDuration()     const { return m_StopDuration;     }
    s64 StopLastDuration() const { return m_StopLastDuration; }

    f32 Seconds()      const { return m_Seconds;      }
    f32 DeltaSeconds() const { return m_DeltaSeconds; }
    f32 TotalSeconds() const { return m_TotalSeconds; }

    b32 Stopped() const { return m_Stopped; }

    const StaticString<256>& Name() const { return m_Name; }

    Timer& operator=(const Timer& other);

private:
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

private:
    s64 m_TicksPerSecond;
    s64 m_InitialTicks;

    s64 m_Ticks;
    s64 m_DeltaTicks;

    s64 m_StopBegin;
    s64 m_StopDuration;
    s64 m_StopLastDuration;

    f32 m_Seconds;
    f32 m_DeltaSeconds;
    f32 m_TotalSeconds;

    b32 m_Stopped;

    StaticString<256> m_Name;

    friend class ScopeTimer;
    friend class ProfilingTimer;
};

class ENGINE_API ProfilingTimer final
{
public:
    ProfilingTimer(const StaticString<256>& name);
    ~ProfilingTimer();

    void StopProfiling(const Logger& logger);

    s64 TicksPerSecond() const { return m_Timer.m_TicksPerSecond; }
    s64 InitialTicks()   const { return m_Timer.m_InitialTicks;   }

    s64 Ticks()      const { return m_Timer.m_Ticks;      }
    s64 DeltaTicks() const { return m_Timer.m_DeltaTicks; }

    s64 StopBegin()        const { return m_Timer.m_StopBegin;        }
    s64 StopDuration()     const { return m_Timer.m_StopDuration;     }
    s64 StopLastDuration() const { return m_Timer.m_StopLastDuration; }

    f32 Seconds()      const { return m_Timer.m_Seconds;      }
    f32 DeltaSeconds() const { return m_Timer.m_DeltaSeconds; }
    f32 TotalSeconds() const { return m_Timer.m_TotalSeconds; }

    b32 Stopped() const { return m_Timer.m_Stopped; }

    const StaticString<256>& Name() const { return m_Timer.m_Name; }

private:
    ProfilingTimer(const ProfilingTimer&)            = delete;
    ProfilingTimer(ProfilingTimer&&)                 = delete;
    ProfilingTimer& operator=(const ProfilingTimer&) = delete;
    ProfilingTimer& operator=(ProfilingTimer&&)      = delete;

private:
    Timer m_Timer;
};
