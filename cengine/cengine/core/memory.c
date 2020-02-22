//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/memory.h"

void CreateMemory(Memory *memory, u32 memory_cap)
{
    Check(memory);

    u64 _memory_cap = ALIGN_UP(cast(u64, memory_cap), PAGE_SIZE);
    CheckM(_memory_cap && _memory_cap <= S32_MAX, "You want too much memory :)");

    memory_cap = cast(u32, _memory_cap);

    /**/ if (memory_cap == PAGE_SIZE) memory->permanent.cap = KB(2);
    else if (memory_cap <= MB(1)    ) memory->permanent.cap = PAGE_SIZE;
    else                              memory->permanent.cap = cast(u32, 0.2*memory_cap);

    memory->transient.cap = memory_cap - memory->permanent.cap;

    memory->transient.base = cast(u8 *, VirtualAlloc(0, memory_cap, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(memory->transient.base);

    memory->permanent.base = memory->transient.base + memory->transient.cap;

    memory->transient.size = 0;
    memory->permanent.size = 0;
}

void DestroyMemory(Memory *memory)
{
    Check(memory);

    if (memory->transient.base)
    {
        DebugResult(b32, VirtualFree(memory->transient.base, 0, MEM_RELEASE));
    }

    ZeroMemory(memory, sizeof(Memory));
}

void *PushToTransientArea(Memory *memory, u32 bytes)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");
    CheckM(memory->transient.size + bytes <= memory->transient.cap, "Transient area overflow");

    void *retptr = memory->transient.base + memory->transient.size;
    memory->transient.size += bytes;

    return retptr;
}

void *PushToTransientAreaAligned(Memory *memory, u32 bytes, u32 alignment)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");
    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");
    
    bytes = ALIGN_UP(bytes, alignment);
    CheckM(memory->transient.size + bytes <= memory->transient.cap, "Transient area overflow");

    void *retptr = memory->transient.base + memory->transient.size;
    memory->transient.size += bytes;

    return retptr;
}

void  ResetTransientArea(Memory *memory)
{
    Check(memory);
    ZeroMemory(memory->transient.base, memory->transient.size);
    memory->transient.size = 0;
}

void *PushToPermanentArea(Memory *memory, u32 bytes)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");
    CheckM(memory->permanent.size + bytes <= memory->permanent.cap, "Permanent area overflow");

    void *retptr = memory->permanent.base + memory->permanent.size;
    memory->permanent.size += bytes;

    return retptr;
}

void *PushToPermanentAreaAligned(Memory *memory, u32 bytes, u32 alignment)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");
    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");
    
    bytes = ALIGN_UP(bytes, alignment);
    CheckM(memory->permanent.size + bytes <= memory->permanent.cap, "Permanent area overflow");

    void *retptr = memory->permanent.base + memory->permanent.size;
    memory->permanent.size += bytes;

    return retptr;
}
