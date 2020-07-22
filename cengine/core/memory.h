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
    IN  u64     transient_area_cap,
    IN  u64     permanent_area_cap,
    OUT Memory *memory
);

CENGINE_FUN void DestroyMemory(
    IN Memory *memory
);

CENGINE_FUN void *PushToTransientArea(
    IN Memory *memory,
    IN u64     bytes
);

CENGINE_FUN void *PushToTransientAreaAligned(
    IN Memory *memory,
    IN u64     bytes,
    IN u64     alignment
);

CENGINE_FUN void ResetTransientArea(
    IN Memory *memory
);

CENGINE_FUN void *PushToPermanentArea(
    IN Memory *memory,
    IN u64     bytes
);

CENGINE_FUN void *PushToPermanentAreaAligned(
    IN Memory *memory,
    IN u64     bytes,
    IN u64     alignment
);

#define PushToTA(Type, memory, count)             cast(Type *, PushToTransientArea(memory, (count) * sizeof(Type)))
#define PushToTAA(Type, memory, count, alignment) cast(Type *, PushToTransientAreaAligned(memory, (count) * sizeof(Type), alignment))

#define PushToPA(Type, memory, count)             cast(Type *, PushToPermanentArea(memory, (count) * sizeof(Type)))
#define PushToPAA(Type, memory, count, alignment) cast(Type *, PushToPermanentAreaAligned(memory, (count) * sizeof(Type), alignment))
