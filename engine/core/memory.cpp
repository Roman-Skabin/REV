//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/memory.h"

//
// Memory
//

Memory *Memory::s_Memory = null;

Memory *Memory::Create(u64 transient_area_capacity, u64 permanent_area_capacity)
{
    CheckM(!s_Memory, "Memory is already created. Use Memory::Get() function instead");
    local Memory memory(transient_area_capacity, permanent_area_capacity);
    s_Memory = &memory;
    return s_Memory;
}

Memory *Memory::Get()
{
    CheckM(s_Memory, "Memory is not created yet");
    return s_Memory;
}

Memory::Memory(u64 transient_area_capacity, u64 permanent_area_capacity)
    : m_CriticalSection()
{
    CheckM(transient_area_capacity, "Transient area capacity must be greater than 0");
    CheckM(permanent_area_capacity, "Permanent area capacity must be greater than 0");

    transient_area_capacity = AlignUp(transient_area_capacity, PAGE_SIZE);
    permanent_area_capacity = AlignUp(permanent_area_capacity, PAGE_SIZE);
    Check(   transient_area_capacity <= MAX
          && permanent_area_capacity <= MAX
          && transient_area_capacity <= MAX - permanent_area_capacity
          && permanent_area_capacity <= MAX - transient_area_capacity);

    u64 memory_capacity = transient_area_capacity + permanent_area_capacity;

    m_TransientArea.capacity = transient_area_capacity;
    m_PermanentArea.capacity = permanent_area_capacity;

    m_TransientArea.base = cast<byte *>(VirtualAlloc(null, memory_capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    CheckM(m_TransientArea.base, "Physical memory overflow");

    m_PermanentArea.base = m_TransientArea.base + m_TransientArea.capacity;

    m_TransientArea.size = 0;
    m_PermanentArea.size = 0;
}

Memory::~Memory()
{
    if (m_TransientArea.base)
    {
        DebugResult(VirtualFree(m_TransientArea.base, 0, MEM_RELEASE));

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
    CheckM(bytes, "0 bytes has been passed");

    m_CriticalSection.Enter();

    CheckM(bytes <= m_TransientArea.capacity - m_TransientArea.size,
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
    CheckM(bytes, "0 bytes was passed");

    if (alignment < ENGINE_DEFAULT_ALIGNMENT) alignment = ENGINE_DEFAULT_ALIGNMENT;
    CheckM(IsPowOf2(alignment), "Alignment must be power of 2");

    u64 aligned_bytes = AlignUp(bytes, alignment);

    m_CriticalSection.Enter();

    CheckM(bytes <= m_TransientArea.capacity - m_TransientArea.size,
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
    CheckM(bytes, "0 bytes was passed");

    m_CriticalSection.Enter();

    CheckM(bytes <= m_PermanentArea.capacity - m_PermanentArea.size,
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
    CheckM(bytes, "0 bytes was passed");

    if (alignment < ENGINE_DEFAULT_ALIGNMENT) alignment = ENGINE_DEFAULT_ALIGNMENT;
    CheckM(IsPowOf2(alignment), "Alignment must be power of 2");
    
    u64 aligned_bytes = AlignUp(bytes, alignment);

    m_CriticalSection.Enter();

    CheckM(bytes <= m_PermanentArea.capacity - m_PermanentArea.size,
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
