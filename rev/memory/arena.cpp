//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "memory/arena.h"

namespace REV
{

Arena::Arena()
    : m_Base(null),
      m_Size(0),
      m_Capacity(0),
      m_CriticalSection()
{
}

Arena::Arena(void *base, u64 capacity)
    : m_Base(cast<byte *>(base)),
      m_Size(0),
      m_Capacity(capacity),
      m_CriticalSection()
{
    REV_CHECK_M(base,     "Arena's base address can not be null");
    REV_CHECK_M(capacity, "Arena's capacity must be greater than 0");
}

Arena::Arena(Arena&& other)
    : m_Base(other.m_Base),
      m_Size(other.m_Size),
      m_Capacity(other.m_Capacity),
      m_CriticalSection(RTTI::move(other.m_CriticalSection))
{
    other.m_Base     = null;
    other.m_Size     = 0;
    other.m_Capacity = 0;
}

Arena::~Arena()
{
    m_Base     = null;
    m_Size     = 0;
    m_Capacity = 0;
}

void *Arena::PushBytes(u64 bytes)
{
    REV_CHECK_M(bytes, "0 bytes has been passed");

    m_CriticalSection.Enter();

    REV_CHECK_M(bytes <= m_Capacity - m_Size,
                "Arena overflow: bytes to allocate: %I64u, remain capacity: %I64u.",
                bytes,
                m_Capacity - m_Size);

    byte *retptr = m_Base + m_Size;
    m_Size += bytes;

    m_CriticalSection.Leave();
    return retptr;
}

void *Arena::PushBytesAligned(u64 bytes, u64 alignment)
{
    REV_CHECK_M(bytes, "0 bytes has been passed");

    if (alignment < REV::DEFAULT_ALIGNMENT) alignment = REV::DEFAULT_ALIGNMENT;
    REV_CHECK_M(IsPowOf2(alignment), "Alignment must be power of 2");

    u64 aligned_bytes = AlignUp(bytes, alignment);

    m_CriticalSection.Enter();

    REV_CHECK_M(aligned_bytes <= m_Capacity - m_Size,
                "Arena overflow: bytes to allocate: %I64u, remain capacity: %I64u.",
                aligned_bytes,
                m_Capacity - m_Size);

    byte *retptr = m_Base + m_Size;
    m_Size += aligned_bytes;

    m_CriticalSection.Leave();
    return retptr;
}

void Arena::Clear(Marker marker)
{
    m_CriticalSection.Enter();
    ZeroMemory(m_Base + marker, m_Size - marker);
    m_Size = marker;
    m_CriticalSection.Leave();
}

}
