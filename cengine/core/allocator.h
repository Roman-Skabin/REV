//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#if DEBUG
    CENGINE_DATA u64 gAllocationsPerFrame;
    CENGINE_DATA u64 gReAllocationsPerFrame;
    CENGINE_DATA u64 gDeAllocationsPerFrame;
#endif

typedef struct BlockHeader BlockHeader;

typedef struct Allocator
{
    // @NOTE(Roman): Just saved pointers to destroyed blocks.
    // @TODO(Roman): BlockHeader *free_list;

    BlockHeader *base;
    u64          used;
    u64          cap;
    b32          reserved;
} Allocator;

CENGINE_FUN void CreateAllocator(Allocator *allocator, void *base_address, u64 capacity, b32 clear_memory);
CENGINE_FUN void DestroyAllocator(Allocator *allocator);

CENGINE_FUN void *  Allocate(Allocator *allocator, u64 bytes);
CENGINE_FUN void  DeAllocate(Allocator *allocator, void **mem);
CENGINE_FUN void *ReAllocate(Allocator *allocator, void **mem, u64 bytes);

CENGINE_FUN void *  AllocateAligned(Allocator *allocator, u64 bytes, u64 alignment);
CENGINE_FUN void  DeAllocateAligned(Allocator *allocator, void **mem);
CENGINE_FUN void *ReAllocateAligned(Allocator *allocator, void **mem, u64 bytes, u64 alignment);

#define   Alloc(Type, allocator, count)      cast(Type *, Allocate(allocator, sizeof(Type) * (count)))
#define DeAlloc(allocator, mem)              DeAllocate(allocator, &(mem))
#define ReAlloc(Type, allocator, mem, count) cast(Type *, ReAllocate(allocator, &(mem), sizeof(Type) * (count)))

#define   AllocA(Type, allocator, count, alignment)      cast(Type *, AllocateAligned(allocator, sizeof(Type) * (count), alignment))
#define DeAllocA(allocator, mem)                         DeAllocateAligned(allocator, &(mem))
#define ReAllocA(Type, allocator, mem, count, alignment) cast(Type *, ReAllocateAligned(allocator, &(mem), sizeof(Type) * (count), alignment))
