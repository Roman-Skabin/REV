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

//
// Memory
//

void CreateMemory(Memory *memory, u64 transient_area_cap, u64 permanent_area_cap)
{
    Check(memory);
    CheckM(transient_area_cap, "Transient area capacity must be greater than 0");
    CheckM(permanent_area_cap, "Permanent area capacity must be greater than 0");

    transient_area_cap = ALIGN_UP(transient_area_cap, PAGE_SIZE);
    permanent_area_cap = ALIGN_UP(permanent_area_cap, PAGE_SIZE);
    Check(U64_MAX - transient_area_cap >= permanent_area_cap);

    u64 memory_cap = transient_area_cap + permanent_area_cap;
    Check(memory_cap <= MAX_MEMORY);

    memory->transient.cap = transient_area_cap;
    memory->permanent.cap = permanent_area_cap;

    memory->transient.base = cast(byte *, VirtualAlloc(0, memory_cap, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    CheckM(memory->transient.base, "Memory creation failed");

    memory->permanent.base = memory->transient.base + memory->transient.cap;

    memory->transient.size = 0;
    memory->permanent.size = 0;
}

void DestroyMemory(Memory *memory)
{
    Check(memory);

    if (memory->transient.base)
    {
        DebugResult(VirtualFree(memory->transient.base, 0, MEM_RELEASE));
    }

    ZeroMemory(memory, sizeof(Memory));
}

void *PushToTransientArea(Memory *memory, u64 bytes)
{
    Check(memory);
    CheckM(bytes, "0 bytes was passed");
    CheckM(memory->transient.size + bytes <= memory->transient.cap, "Transient area overflow");

    byte *retptr = memory->transient.base + memory->transient.size;
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

    byte *retptr = memory->transient.base + memory->transient.size;
    memory->transient.size += bytes;

    return retptr;
}

void ResetTransientArea(Memory *memory)
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

    byte *retptr = memory->permanent.base + memory->permanent.size;
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

    byte *retptr = memory->permanent.base + memory->permanent.size;
    memory->permanent.size += bytes;

    return retptr;
}
