//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "memory/arena.h"

namespace REV
{

Arena::Arena(const ConstString& name)
    : m_Base(null),
      m_Used(0),
      m_Capacity(0),
      m_Name(name),
#if REV_DEBUG
      m_MaxMemoryUsed(0),
#endif
      m_CriticalSection()
{
}

Arena::Arena(void *base, u64 capacity, const ConstString& name)
    : m_Base(cast(byte *, base)),
      m_Used(0),
      m_Capacity(capacity),
      m_Name(name),
#if REV_DEBUG
      m_MaxMemoryUsed(0),
#endif
      m_CriticalSection()
{
    REV_CHECK_M(base,     "Arena's base address can not be null");
    REV_CHECK_M(capacity, "Arena's capacity must be greater than 0");
}

Arena::Arena(Arena&& other)
    : m_Base(other.m_Base),
      m_Used(other.m_Used),
      m_Capacity(other.m_Capacity),
      m_Name(RTTI::move(other.m_Name)),
#if REV_DEBUG
      m_MaxMemoryUsed(other.m_MaxMemoryUsed),
#endif
      m_CriticalSection(RTTI::move(other.m_CriticalSection))
{
    other.m_Base     = null;
    other.m_Used     = 0;
    other.m_Capacity = 0;
#if REV_DEBUG
    other.m_MaxMemoryUsed = 0;
#endif
}

Arena::~Arena()
{
#if REV_DEBUG
    PrintDebugMessage(DEBUG_COLOR::INFO, "Arena \"%.*s\" stats:", m_Name.Length(), m_Name.Data());
    PrintDebugMessage(DEBUG_COLOR::INFO, "    Max memory used: %I64u B = %f KB = %f MB = %f GB", m_MaxMemoryUsed,
                                                                                                 m_MaxMemoryUsed / 1024.0f,
                                                                                                 m_MaxMemoryUsed / 1048576.0f,
                                                                                                 m_MaxMemoryUsed / 1073741824.0f);
    PrintDebugMessage(DEBUG_COLOR::INFO, "    Capacity: %I64u B = %f KB = %f MB = %f GB", m_Capacity,
                                                                                          m_Capacity / 1024.0f,
                                                                                          m_Capacity / 1048576.0f,
                                                                                          m_Capacity / 1073741824.0f);
#endif
    m_Base     = null;
    m_Used     = 0;
    m_Capacity = 0;
}

void *Arena::PushBytes(u64 bytes)
{
    REV_CHECK_M(bytes, "0 bytes has been passed");

    m_CriticalSection.Enter();

    REV_CHECK_M(bytes <= m_Capacity - m_Used,
                "Arena overflow: bytes to allocate: %I64u, remain capacity: %I64u",
                bytes,
                m_Capacity - m_Used);

    byte *retptr = m_Base + m_Used;
    m_Used += bytes;

#if REV_DEBUG
    if (m_Used > m_MaxMemoryUsed) m_MaxMemoryUsed = m_Used;
#endif

    m_CriticalSection.Leave();
    return retptr;
}

void *Arena::PushBytesAligned(u64 bytes, u64 alignment)
{
    REV_CHECK_M(bytes, "0 bytes has been passed");

    if (alignment < DEFAULT_ALIGNMENT) alignment = DEFAULT_ALIGNMENT;
    REV_CHECK_M(IsPowOf2(alignment), "Alignment must be power of 2");

    u64 aligned_bytes = AlignUp(bytes, alignment);

    m_CriticalSection.Enter();

    REV_CHECK_M(aligned_bytes <= m_Capacity - m_Used,
                "Arena overflow: bytes to allocate: %I64u, remain capacity: %I64u",
                aligned_bytes,
                m_Capacity - m_Used);

    byte *retptr = m_Base + m_Used;
    m_Used += aligned_bytes;

#if REV_DEBUG
    if (m_Used > m_MaxMemoryUsed) m_MaxMemoryUsed = m_Used;
#endif

    m_CriticalSection.Leave();
    return retptr;
}

void Arena::Clear(Marker marker)
{
    m_CriticalSection.Enter();
    ZeroMemory(m_Base + marker, m_Used - marker);
    m_Used = marker;
    m_CriticalSection.Leave();
}

}
