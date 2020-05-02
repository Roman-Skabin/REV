//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#define MAX_MEMORY 0xFFFFFFFFFFFFF000ui64

typedef enum REGULAR_MEMORY_SPECS
{
    CACHE_LINE_SIZE   = 64,
    PAGE_SIZE         = KB(4),
    STACK_SIZE        = KB(4),
    HEAP_SIZE         = GB(1),
    NTFS_CLUSTER_SIZE = KB(4),
    NTFS_SECTOR_SIZE  = 512,
} REGULAR_MEMORY_SPECS;

CENGINE_FUN void memset_f32(f32 *mem, f32 val, u32 count);

typedef struct Area
{
    byte *base;
    u64   size;
    u64   cap;
} Area;

typedef struct Memory
{
    Area transient;
    Area permanent;
} Memory;

CENGINE_FUN void CreateMemory(Memory *memory, u64 transient_area_cap, u64 permanent_area_cap);
CENGINE_FUN void DestroyMemory(Memory *memory);

CENGINE_FUN void *PushToTransientArea(Memory *memory, u64 bytes);
CENGINE_FUN void *PushToTransientAreaAligned(Memory *memory, u64 bytes, u64 alignment);
CENGINE_FUN void  ResetTransientArea(Memory *memory);

CENGINE_FUN void *PushToPermanentArea(Memory *memory, u64 bytes);
CENGINE_FUN void *PushToPermanentAreaAligned(Memory *memory, u64 bytes, u64 alignment);

#define PushToTA(Type, memory, count)             cast(Type *, PushToTransientArea(memory, (count) * sizeof(Type)))
#define PushToTAA(Type, memory, count, alignment) cast(Type *, PushToTransientAreaAligned(memory, (count) * sizeof(Type), alignment))

#define PushToPA(Type, memory, count)             cast(Type *, PushToPermanentArea(memory, (count) * sizeof(Type)))
#define PushToPAA(Type, memory, count, alignment) cast(Type *, PushToPermanentAreaAligned(memory, (count) * sizeof(Type), alignment))
