//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/memory.h"

namespace REV
{

Memory *Memory::s_Memory = null;

Memory *Memory::Create(u64 transient_area_capacity, u64 permanent_area_capacity)
{
    REV_CHECK_M(!s_Memory, "Memory is already created. Use Memory::Get() function instead");
    REV_LOCAL Memory memory(transient_area_capacity, permanent_area_capacity);
    s_Memory = &memory;
    return s_Memory;
}

Memory *Memory::Get()
{
    REV_CHECK_M(s_Memory, "Memory is not created yet");
    return s_Memory;
}

Memory::Memory(u64 transient_area_capacity, u64 permanent_area_capacity)
    : m_CriticalSection()
{
    REV_CHECK_M(transient_area_capacity, "Transient area capacity must be greater than 0");
    REV_CHECK_M(permanent_area_capacity, "Permanent area capacity must be greater than 0");

    transient_area_capacity = AlignUp(transient_area_capacity, PAGE_SIZE);
    permanent_area_capacity = AlignUp(permanent_area_capacity, PAGE_SIZE);
    REV_CHECK(   transient_area_capacity <= MAX
              && permanent_area_capacity <= MAX
              && transient_area_capacity <= MAX - permanent_area_capacity
              && permanent_area_capacity <= MAX - transient_area_capacity);

    u64 memory_capacity = transient_area_capacity + permanent_area_capacity;

    m_TransientArea.capacity = transient_area_capacity;
    m_PermanentArea.capacity = permanent_area_capacity;

#if REV_PLATFORM_WIN64
    m_TransientArea.base = cast<byte *>(VirtualAlloc(null, memory_capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    REV_CHECK_M(m_TransientArea.base, "Physical memory overflow");
#else
    m_TransientArea.base = mmap(null, memory_capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    REV_CHECK_M(m_TransientArea.base != MAP_FAILED, "Physical memory overflow");
#endif

    m_PermanentArea.base = m_TransientArea.base + m_TransientArea.capacity;

    m_TransientArea.size = 0;
    m_PermanentArea.size = 0;
}

Memory::~Memory()
{
    if (m_TransientArea.base)
    {
    #if REV_PLATFORM_WIN64
        REV_DEBUG_RESULT(VirtualFree(m_TransientArea.base, 0, MEM_RELEASE));
    #else
        REV_DEBUG_RESULT(!munmap(m_TransientArea.base, m_TransientArea.capacity + m_PermanentArea.capacity));
    #endif

        m_TransientArea.base     = null;
        m_TransientArea.size     = 0;
        m_TransientArea.capacity = 0;

        m_PermanentArea.base     = null;
        m_PermanentArea.size     = 0;
        m_PermanentArea.capacity = 0;
    }
}

void *Memory::PushToTransientArea(u64 bytes)
{
    REV_CHECK_M(bytes, "0 bytes has been passed");

    m_CriticalSection.Enter();

    REV_CHECK_M(bytes <= m_TransientArea.capacity - m_TransientArea.size,
                "Transient area overflow.\n"
                "Bytes to allocate: %I64u.\n"
                "Remain transient area capacity: %I64u.",
                bytes,
                m_TransientArea.capacity - m_TransientArea.size);

    byte *retptr = m_TransientArea.base + m_TransientArea.size;
    m_TransientArea.size += bytes;

    m_CriticalSection.Leave();
    return retptr;
}

void *Memory::PushToTransientAreaAligned(u64 bytes, u64 alignment)
{
    REV_CHECK_M(bytes, "0 bytes was passed");

    if (alignment < REV::DEFAULT_ALIGNMENT) alignment = REV::DEFAULT_ALIGNMENT;
    REV_CHECK_M(IsPowOf2(alignment), "Alignment must be power of 2");

    u64 aligned_bytes = AlignUp(bytes, alignment);

    m_CriticalSection.Enter();

    REV_CHECK_M(bytes <= m_TransientArea.capacity - m_TransientArea.size,
                "Transient area overflow.\n"
                "Bytes: %I64u. Alignment: %I64u. Aligned bytes: %I64u.\n"
                "Remain transient area capacity: %I64u.",
                bytes, alignment, aligned_bytes,
                m_TransientArea.capacity - m_TransientArea.size);

    byte *retptr = m_TransientArea.base + m_TransientArea.size;
    m_TransientArea.size += aligned_bytes;

    m_CriticalSection.Leave();
    return retptr;
}

void Memory::ResetTransientArea()
{
    m_CriticalSection.Enter();
    ZeroMemory(m_TransientArea.base, m_TransientArea.size);
    m_TransientArea.size = 0;
    m_CriticalSection.Leave();
}

void *Memory::PushToPermanentArea(u64 bytes)
{
    REV_CHECK_M(bytes, "0 bytes was passed");

    m_CriticalSection.Enter();

    REV_CHECK_M(bytes <= m_PermanentArea.capacity - m_PermanentArea.size,
                "Permanent area overflow.\n"
                "Bytes to allocate: %I64u.\n"
                "Remain transient area capacity: %I64u.",
                bytes,
                m_PermanentArea.capacity - m_PermanentArea.size);

    byte *retptr = m_PermanentArea.base + m_PermanentArea.size;
    m_PermanentArea.size += bytes;

    m_CriticalSection.Leave();
    return retptr;
}

void *Memory::PushToPermanentAreaAligned(u64 bytes, u64 alignment)
{
    REV_CHECK_M(bytes, "0 bytes was passed");

    if (alignment < REV::DEFAULT_ALIGNMENT) alignment = REV::DEFAULT_ALIGNMENT;
    REV_CHECK_M(IsPowOf2(alignment), "Alignment must be power of 2");
    
    u64 aligned_bytes = AlignUp(bytes, alignment);

    m_CriticalSection.Enter();

    REV_CHECK_M(bytes <= m_PermanentArea.capacity - m_PermanentArea.size,
                "Transient area overflow.\n"
                "Bytes: %I64u. Alignment: %I64u. Aligned bytes: %I64u.\n"
                "Remain transient area capacity: %I64u.",
                bytes, alignment, aligned_bytes,
                m_PermanentArea.capacity - m_PermanentArea.size);

    byte *retptr = m_PermanentArea.base + m_PermanentArea.size;
    m_PermanentArea.size += aligned_bytes;

    m_CriticalSection.Leave();
    return retptr;
}

}
