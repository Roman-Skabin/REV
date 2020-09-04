//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

typedef struct BlockHeader BlockHeader;

typedef struct Allocator
{
    BlockHeader *free_list;
    BlockHeader *first;
    BlockHeader *last_allocated;
    u64          used;
    u64          cap;
#if DEBUG
    u64          allocations_count;
    u64          reallocations_count;
    u64          deallocations_count;
#endif
    b32          valloc_used;
} Allocator;

CENGINE_FUN void CreateAllocator(
    in  Allocator *allocator,
    opt void      *base_address,
    in  u64        capacity,
    in  b32        clear_memory // @NOTE(Roman): There is a sence to use it only if base_address != null.
);

CENGINE_FUN void DestroyAllocator(
    in Allocator *allocator
);

CENGINE_FUN void *Allocate(
    in Allocator *allocator,
    in u64        bytes
);

CENGINE_FUN void DeAllocate(
    in Allocator  *allocator,
    in void      **mem
);

CENGINE_FUN void *ReAllocate(
    in  Allocator  *allocator,
    opt void      **mem,       // @NOTE(Roman): if mem or *mem = null then Allocate returns.
    opt u64         bytes      // @NOTE(Roman): if bytes = 0 then *mem will be deallocated.
);

CENGINE_FUN void *AllocateAligned(
    in  Allocator *allocator,
    in  u64        bytes,
    opt u64        alignment
);

CENGINE_FUN void DeAllocateAligned(
    in Allocator  *allocator,
    in void      **mem
);

CENGINE_FUN void *ReAllocateAligned(
    in  Allocator  *allocator,
    opt void      **mem,       // @NOTE(Roman): if mem or *mem = null then Allocate returns.
    opt u64         bytes,     // @NOTE(Roman): if bytes = 0 then *mem will be deallocated.
    opt u64         alignment
);

#define   Alloc(Type, allocator, count)      cast(Type *, Allocate(allocator, sizeof(Type) * (count)))
#define DeAlloc(allocator, mem)              DeAllocate(allocator, &(mem))
#define ReAlloc(Type, allocator, mem, count) cast(Type *, ReAllocate(allocator, &(mem), sizeof(Type) * (count)))

#define   AllocA(Type, allocator, count, alignment)      cast(Type *, AllocateAligned(allocator, sizeof(Type) * (count), alignment))
#define DeAllocA(allocator, mem)                         DeAllocateAligned(allocator, &(mem))
#define ReAllocA(Type, allocator, mem, count, alignment) cast(Type *, ReAllocateAligned(allocator, &(mem), sizeof(Type) * (count), alignment))
