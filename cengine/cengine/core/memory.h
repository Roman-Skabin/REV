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

CEXTERN void memset_f32(f32 *mem, f32 val, u32 count);

typedef struct Area
{
    u8  *base;
    u64  size;
    u64  cap;
} Area;

typedef struct Memory
{
    Area transient;
    Area permanent;
} Memory;

CEXTERN void CreateMemory(Memory *memory, u64 memory_cap);
CEXTERN void DestroyMemory(Memory *memory);

CEXTERN void *PushToTransientArea(Memory *memory, u64 bytes);
CEXTERN void *PushToTransientAreaAligned(Memory *memory, u64 bytes, u64 alignment);
CEXTERN void  ResetTransientArea(Memory *memory);

CEXTERN void *PushToPermanentArea(Memory *memory, u64 bytes);
CEXTERN void *PushToPermanentAreaAligned(Memory *memory, u64 bytes, u64 alignment);

#define PushToTA(Type, memory, count)             cast(Type *, PushToTransientArea(memory, count * sizeof(Type)))
#define PushToTAA(Type, memory, count, alignment) cast(Type *, PushToTransientAreaAligned(memory, count * sizeof(Type), alignment))

#define PushToPA(Type, memory, count)             cast(Type *, PushToPermanentArea(memory, count * sizeof(Type)))
#define PushToPAA(Type, memory, count, alignment) cast(Type *, PushToPermanentAreaAligned(memory, count * sizeof(Type), alignment))
