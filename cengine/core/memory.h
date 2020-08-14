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

CENGINE_FUN void memset_f32(f32 *mem, f32 val, u64 count);
CENGINE_FUN void memset_f64(f64 *mem, f64 val, u64 count);

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

CENGINE_FUN void CreateMemory(
    in  u64     transient_area_cap,
    in  u64     permanent_area_cap,
    out Memory *memory
);

CENGINE_FUN void DestroyMemory(
    in Memory *memory
);

CENGINE_FUN void *PushToTransientArea(
    in Memory *memory,
    in u64     bytes
);

CENGINE_FUN void *PushToTransientAreaAligned(
    in Memory *memory,
    in u64     bytes,
    in u64     alignment
);

CENGINE_FUN void ResetTransientArea(
    in Memory *memory
);

CENGINE_FUN void *PushToPermanentArea(
    in Memory *memory,
    in u64     bytes
);

CENGINE_FUN void *PushToPermanentAreaAligned(
    in Memory *memory,
    in u64     bytes,
    in u64     alignment
);

#define PushToTA(Type, memory, count)             cast(Type *, PushToTransientArea(memory, (count) * sizeof(Type)))
#define PushToTAA(Type, memory, count, alignment) cast(Type *, PushToTransientAreaAligned(memory, (count) * sizeof(Type), alignment))

#define PushToPA(Type, memory, count)             cast(Type *, PushToPermanentArea(memory, (count) * sizeof(Type)))
#define PushToPAA(Type, memory, count, alignment) cast(Type *, PushToPermanentAreaAligned(memory, (count) * sizeof(Type), alignment))
