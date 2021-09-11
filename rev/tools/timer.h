//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "tools/logger.h"
#include "tools/static_string.hpp"

namespace REV
{
    class REV_API Timer final
    {
    public:
        Timer(const ConstString& name);
        Timer(const Timer& other);

        REV_INLINE ~Timer() {}

        void Tick();

        void Start();
        void Stop();

        REV_INLINE s64 TicksPerSecond() const { return m_TicksPerSecond; }
        REV_INLINE s64 InitialTicks()   const { return m_InitialTicks;   }

        REV_INLINE s64 Ticks()      const { return m_Ticks;      }
        REV_INLINE s64 DeltaTicks() const { return m_DeltaTicks; }

        REV_INLINE s64 StopBegin()        const { return m_StopBegin;        }
        REV_INLINE s64 StopDuration()     const { return m_StopDuration;     }
        REV_INLINE s64 StopLastDuration() const { return m_StopLastDuration; }

        REV_INLINE f32 Seconds()      const { return m_Seconds;      }
        REV_INLINE f32 DeltaSeconds() const { return m_DeltaSeconds; }
        REV_INLINE f32 TotalSeconds() const { return m_TotalSeconds; }

        REV_INLINE bool Stopped() const { return m_Stopped; }

        REV_INLINE const StaticString<256>& Name() const { return m_Name; }

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

    class REV_API ProfilingTimer final
    {
    public:
        ProfilingTimer(const ConstString& name);

        REV_INLINE ~ProfilingTimer() {}

        void StopProfiling(const Logger& logger);

        REV_INLINE s64 TicksPerSecond() const { return m_Timer.m_TicksPerSecond; }
        REV_INLINE s64 InitialTicks()   const { return m_Timer.m_InitialTicks;   }

        REV_INLINE s64 Ticks()      const { return m_Timer.m_Ticks;      }
        REV_INLINE s64 DeltaTicks() const { return m_Timer.m_DeltaTicks; }

        REV_INLINE s64 StopBegin()        const { return m_Timer.m_StopBegin;        }
        REV_INLINE s64 StopDuration()     const { return m_Timer.m_StopDuration;     }
        REV_INLINE s64 StopLastDuration() const { return m_Timer.m_StopLastDuration; }

        REV_INLINE f32 Seconds()      const { return m_Timer.m_Seconds;      }
        REV_INLINE f32 DeltaSeconds() const { return m_Timer.m_DeltaSeconds; }
        REV_INLINE f32 TotalSeconds() const { return m_Timer.m_TotalSeconds; }

        REV_INLINE bool Stopped() const { return m_Timer.m_Stopped; }

        REV_INLINE const StaticString<256>& Name() const { return m_Timer.m_Name; }

    private:
        REV_DELETE_CONSTRS_AND_OPS(ProfilingTimer);

    private:
        Timer m_Timer;
    };
}
