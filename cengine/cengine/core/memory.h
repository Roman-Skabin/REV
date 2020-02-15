//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

typedef enum REGULAR_MEMORY_SPECS
{
    CACHE_LINE_SIZE   = 64,
    PAGE_SIZE         = KB(4),
    STACK_SIZE        = KB(4),
    HEAP_SIZE         = GB(1),
    NTFS_CLUSTER_SIZE = KB(4),
    NTFS_SECTOR_SIZE  = 512,
} REGULAR_MEMORY_SPECS;

INLINE void memset_f32(f32 *mem, f32 val, u32 count)
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

typedef struct Area
{
    u8  *base;
    u32  size;
    u32  cap;
} Area;

typedef struct Memory
{
    Area transient;
    Area permanent;
} Memory;

CEXTERN void CreateMemory(Memory *memory, u32 memory_cap);
CEXTERN void DestroyMemory(Memory *memory);

CEXTERN void *PushToTransientArea(Memory *memory, u32 bytes);
CEXTERN void *PushToTransientAreaAligned(Memory *memory, u32 bytes, u32 alignment);
CEXTERN void  ResetTransientArea(Memory *memory);

CEXTERN void *PushToPermanentArea(Memory *memory, u32 bytes);
CEXTERN void *PushToPermanentAreaAligned(Memory *memory, u32 bytes, u32 alignment);

#define PushToTA(Type, memory, count)             cast(Type *, PushToTransientArea(memory, count * sizeof(Type)))
#define PushToTAA(Type, memory, count, alignment) cast(Type *, PushToTransientAreaAligned(memory, count * sizeof(Type), alignment))

#define PushToPA(Type, memory, count)             cast(Type *, PushToPermanentArea(memory, count * sizeof(Type)))
#define PushToPAA(Type, memory, count, alignment) cast(Type *, PushToPermanentAreaAligned(memory, count * sizeof(Type), alignment))
