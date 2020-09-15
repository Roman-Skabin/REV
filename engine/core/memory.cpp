//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/memory.h"

Memory::Area::Area()
    : base(null),
      size(0),
      capacity(0)
{
}

Memory::Area::Area(in Area&& other)
    : base(other.base),
      size(other.size),
      capacity(other.capacity)
{
    other.base     = null;
    other.size     = 0;
    other.capacity = 0;
}

Memory::Area& Memory::Area::operator=(in Area&& other)
{
    if (this != &other)
    {
        base     = other.base;
        size     = other.size;
        capacity = other.capacity;

        other.base     = null;
        other.size     = 0;
        other.capacity = 0;
    }
    return *this;
}

Memory::Memory(in u64 transient_area_capacity, in u64 permanent_area_capacity)
{
    CheckM(transient_area_capacity, "Transient area capacity must be greater than 0");
    CheckM(permanent_area_capacity, "Permanent area capacity must be greater than 0");

    transient_area_capacity = AlignUp(transient_area_capacity, PAGE_SIZE);
    permanent_area_capacity = AlignUp(permanent_area_capacity, PAGE_SIZE);
    Check(   transient_area_capacity <= MAX_MEMORY
          && permanent_area_capacity <= MAX_MEMORY
          && transient_area_capacity <= MAX_MEMORY - permanent_area_capacity
          && permanent_area_capacity <= MAX_MEMORY - transient_area_capacity);

    u64 memory_capacity = transient_area_capacity + permanent_area_capacity;

    m_TransientArea.capacity = transient_area_capacity;
    m_PermanentArea.capacity = permanent_area_capacity;

    m_TransientArea.base = cast<byte *>(VirtualAlloc(null, memory_capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    CheckM(m_TransientArea.base, "Physical memory overflow");

    m_PermanentArea.base = m_TransientArea.base + m_TransientArea.capacity;

    m_TransientArea.size = 0;
    m_PermanentArea.size = 0;
}

Memory::Memory(in Memory&& other)
    : m_TransientArea(RTTI::move(other.m_TransientArea)),
      m_PermanentArea(RTTI::move(other.m_PermanentArea))
{
}

Memory::~Memory()
{
    if (m_TransientArea.base)
    {
        DebugResult(VirtualFree(m_TransientArea.base, 0, MEM_RELEASE));
        m_TransientArea.base = null;
    }
    m_TransientArea.size     = 0;
    m_TransientArea.capacity = 0;

    m_PermanentArea.base     = null;
    m_PermanentArea.size     = 0;
    m_PermanentArea.capacity = 0;
}

void *Memory::PushToTransientArea(in u64 bytes)
{
    CheckM(bytes, "0 bytes has been passed");

    CheckM(bytes <= m_TransientArea.capacity - m_TransientArea.size,
           "Transient area overflow.\n"
           "Bytes to allocate: %I64u.\n"
           "Remain transient area capacity: %I64u.",
           bytes,
           m_TransientArea.capacity - m_TransientArea.size);

    byte *retptr = m_TransientArea.base + m_TransientArea.size;
    m_TransientArea.size += bytes;

    return retptr;
}

void *Memory::PushToTransientAreaAligned(in u64 bytes, opt u64 alignment)
{
    CheckM(bytes, "0 bytes was passed");

    if (alignment < ENGINE_DEFAULT_ALIGNMENT) alignment = ENGINE_DEFAULT_ALIGNMENT;
    CheckM(IsPowOf2(alignment), "Alignment must be power of 2");

    u64 aligned_bytes = AlignUp(bytes, alignment);

    CheckM(bytes <= m_TransientArea.capacity - m_TransientArea.size,
           "Transient area overflow.\n"
           "Bytes: %I64u. Alignment: %I64u. Aligned bytes: %I64u.\n"
           "Remain transient area capacity: %I64u.",
           bytes, alignment, aligned_bytes,
           m_TransientArea.capacity - m_TransientArea.size);

    byte *retptr = m_TransientArea.base + m_TransientArea.size;
    m_TransientArea.size += aligned_bytes;

    return retptr;
}

void Memory::ResetTransientArea()
{
    ZeroMemory(m_TransientArea.base, m_TransientArea.size);
    m_TransientArea.size = 0;
}

void *Memory::PushToPermanentArea(in u64 bytes)
{
    CheckM(bytes, "0 bytes was passed");

    CheckM(bytes <= m_PermanentArea.capacity - m_PermanentArea.size,
           "Permanent area overflow.\n"
           "Bytes to allocate: %I64u.\n"
           "Remain transient area capacity: %I64u.",
           bytes,
           m_PermanentArea.capacity - m_PermanentArea.size);

    byte *retptr = m_PermanentArea.base + m_PermanentArea.size;
    m_PermanentArea.size += bytes;

    return retptr;
}

void *Memory::PushToPermanentAreaAligned(in u64 bytes, opt u64 alignment)
{
    CheckM(bytes, "0 bytes was passed");

    if (alignment < ENGINE_DEFAULT_ALIGNMENT) alignment = ENGINE_DEFAULT_ALIGNMENT;
    CheckM(IsPowOf2(alignment), "Alignment must be power of 2");
    
    u64 aligned_bytes = AlignUp(bytes, alignment);

    CheckM(bytes <= m_PermanentArea.capacity - m_PermanentArea.size,
           "Transient area overflow.\n"
           "Bytes: %I64u. Alignment: %I64u. Aligned bytes: %I64u.\n"
           "Remain transient area capacity: %I64u.",
           bytes, alignment, aligned_bytes,
           m_PermanentArea.capacity - m_PermanentArea.size);

    byte *retptr = m_PermanentArea.base + m_PermanentArea.size;
    m_PermanentArea.size += aligned_bytes;

    return retptr;
}
