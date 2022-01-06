// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "memory/memlow.h"
#include "tools/critical_section.hpp"
#include "tools/const_string.h"

namespace REV
{
    class REV_API Arena final
    {
    public:
        using Marker = u64;

        Arena(void *base, u64 capacity, const ConstString& name = null);
        Arena(Arena&& other);

        ~Arena();

        void *PushBytes(u64 bytes);
        void *PushBytesAligned(u64 bytes, u64 alignment = DEFAULT_ALIGNMENT);

        template<typename T> REV_INLINE T *Push(u64 count = 1)                     { return cast(T *, PushBytes(count * sizeof(T)));                   }
        template<typename T> REV_INLINE T *PushA(u64 count = 1, u64 alignment = 0) { return cast(T *, PushBytesAligned(count * sizeof(T), alignment)); }

        template<> REV_INLINE void *Push<void>(u64 bytes)                 { return PushBytes(bytes);                   }
        template<> REV_INLINE void *PushA<void>(u64 bytes, u64 alignment) { return PushBytesAligned(bytes, alignment); }

        REV_INLINE Marker SaveMarker() const { return m_Used; }

        void Clear(Marker marker = 0);

        REV_INLINE const byte *Base() const { return m_Base; }
        REV_INLINE       byte *Base()       { return m_Base; }

        REV_INLINE u64 Used()     const { return m_Used;     }
        REV_INLINE u64 Capacity() const { return m_Capacity; }

    private:
        Arena(const ConstString& name = null);

        Arena(const Arena&)            = delete;
        Arena& operator=(const Arena&) = delete;
        Arena& operator=(Arena&&)      = delete;

    private:
        byte                   *m_Base;
        u64                     m_Used;
        u64                     m_Capacity;
        ConstString             m_Name;
    #if REV_DEBUG
        u64                     m_MaxMemoryUsed;
    #endif
        CriticalSection<false>  m_CriticalSection;

        friend class Memory;
    };
}
