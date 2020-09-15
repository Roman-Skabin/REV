//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/logger.h"

class ENGINE_IMPEXP Timer final
{
public:
    // @NOTE(Roman): It would be better if you pass non-null name_len,
    //               otherwise strlen will be used.
    Timer(in const char *name, in_opt u64 name_len = 0);
    Timer(const Timer& other);
    Timer(Timer&& other) noexcept;

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

    const char *Name() const { return m_Name; }

    Timer& operator=(const Timer& other);
    Timer& operator=(Timer&& other) noexcept;

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

    char m_Name[256];

    friend class ScopeTimer;
    friend class ProfilingTimer;
};

class ENGINE_IMPEXP ProfilingTimer final
{
public:
    // @NOTE(Roman): It would be better if you pass non-null name_len,
    //               otherwise strlen will be used.
    ProfilingTimer(in const char *name, in_opt u64 name_len = 0);
    ~ProfilingTimer();

    void StopProfiling(in const Logger& logger);

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

    const char *Name() const { return m_Timer.m_Name; }

    ProfilingTimer(in const ProfilingTimer&)            = delete;
    ProfilingTimer(in ProfilingTimer&&)                 = delete;
    ProfilingTimer& operator=(in const ProfilingTimer&) = delete;
    ProfilingTimer& operator=(in ProfilingTimer&&)      = delete;

private:
    Timer m_Timer;
};
