//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/memory.h"

void memset_f32(f32 *mem, f32 val, u32 count)
{
    __m256 *mm256_mem = cast(__m256 *, mem);
    __m256  mm256_val = _mm256_set1_ps(val);

    u32 index = 0;
    while (index + 8 <= count)
    {
        *mm256_mem++ = mm256_val;
        index += 8;
    }

    if (index < count)
    {
        __m128 *mm128_mem = cast(__m128 *, mm256_mem);

        if (index + 4 <= count)
        {
            *mm128_mem++ = _mm_set_ps1(val);
            index += 4;
        }

        if (index < count)
        {
            mem = cast(f32 *, mm128_mem);

            while (index < count)
            {
                *mem++ = val;
                ++index;
            }
        }
    }
}

void CreateMemory(Memory *memory, u64 memory_cap)
{
    Check(memory);
    CheckM(memory_cap, "Memory capacity must be greater than 0");

    memory_cap = memory_cap >= MAX_MEMORY ? MAX_MEMORY : ALIGN_UP(memory_cap, PAGE_SIZE);

    /**/ if (memory_cap == PAGE_SIZE) memory->permanent.cap = KB(2);
    else if (memory_cap <= MB(1)    ) memory->permanent.cap = PAGE_SIZE;
    else                              memory->permanent.cap = cast(u64, 0.2*memory_cap);

    memory->transient.cap  = memory_cap - memory->permanent.cap;

    memory->transient.base = cast(u8 *, VirtualAlloc(0, memory_cap, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    CheckM(memory->transient.base, "Allocation failed");

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

void *PushToTransientArea(Memory *memory, u64 bytes)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");
    CheckM(memory->transient.size + bytes <= memory->transient.cap, "Transient area overflow");

    void *retptr = memory->transient.base + memory->transient.size;
    memory->transient.size += bytes;

    return retptr;
}

void *PushToTransientAreaAligned(Memory *memory, u64 bytes, u64 alignment)
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

void *PushToPermanentArea(Memory *memory, u64 bytes)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");
    CheckM(memory->permanent.size + bytes <= memory->permanent.cap, "Permanent area overflow");

    void *retptr = memory->permanent.base + memory->permanent.size;
    memory->permanent.size += bytes;

    return retptr;
}

void *PushToPermanentAreaAligned(Memory *memory, u64 bytes, u64 alignment)
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
