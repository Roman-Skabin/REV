//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "memory/memlow.h"
#include "tools/critical_section.hpp"

namespace REV
{
    class REV_API Arena final
    {
    public:
        using Marker = u64;

        Arena(void *base, u64 capacity);
        Arena(Arena&& other);

        ~Arena();

        void *PushBytes(u64 bytes);
        void *PushBytesAligned(u64 bytes, u64 alignment);

        template<typename T> REV_INLINE T *Push(u64 count = 1)                     { return cast<T *>(PushBytes(count * sizeof(T)));                   }
        template<typename T> REV_INLINE T *PushA(u64 count = 1, u64 alignment = 0) { return cast<T *>(PushBytesAligned(count * sizeof(T), alignment)); }

        template<> REV_INLINE void *Push<void>(u64 bytes)                 { return PushBytes(bytes);                   }
        template<> REV_INLINE void *PushA<void>(u64 bytes, u64 alignment) { return PushBytesAligned(bytes, alignment); }

        REV_INLINE Marker SaveMarker() const { return m_Size; }

        void Clear(Marker marker = 0);

        REV_INLINE const byte *Base() const { return m_Base; }
        REV_INLINE       byte *Base()       { return m_Base; }

        REV_INLINE u64 Size()     const { return m_Size;     }
        REV_INLINE u64 Capacity() const { return m_Capacity; }

    private:
        Arena();

        Arena(const Arena&)            = delete;
        Arena& operator=(const Arena&) = delete;
        Arena& operator=(Arena&&)      = delete;

    private:
        byte                   *m_Base;
        u64                     m_Size;
        u64                     m_Capacity;
        CriticalSection<false>  m_CriticalSection;

        friend class Memory;
    };
}
