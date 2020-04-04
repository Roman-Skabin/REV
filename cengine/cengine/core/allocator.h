//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#if DEBUG
    extern u64 gAllocationsPerFrame;
    extern u64 gReAllocationsPerFrame;
    extern u64 gDeAllocationsPerFrame;
#endif

typedef struct BlockHeader BlockHeader;

typedef struct Allocator
{
    BlockHeader *base;
    u64          used;
    u64          cap;
    b32          reserved;
} Allocator;

CEXTERN void CreateAllocator(Allocator *allocator, void *base_address, u64 capacity, b32 clear_memory);
CEXTERN void DestroyAllocator(Allocator *allocator);

CEXTERN void *  Allocate(Allocator *allocator, u64 bytes);
CEXTERN void  DeAllocate(Allocator *allocator, void **mem);
CEXTERN void *ReAllocate(Allocator *allocator, void **mem, u64 bytes);

CEXTERN void *  AllocateAligned(Allocator *allocator, u64 bytes, u64 alignment);
CEXTERN void  DeAllocateAligned(Allocator *allocator, void **mem);
CEXTERN void *ReAllocateAligned(Allocator *allocator, void **mem, u64 bytes, u64 alignment);

#define   Alloc(Type, allocator, count)      cast(Type *, Allocate(allocator, sizeof(Type) * (count)))
#define DeAlloc(allocator, mem)              DeAllocate(allocator, &(mem))
#define ReAlloc(Type, allocator, mem, count) cast(Type *, ReAllocate(allocator, &(mem), sizeof(Type) * (count)))

#define   AllocA(Type, allocator, count, alignment)      cast(Type *, AllocateAligned(allocator, sizeof(Type) * (count), alignment))
#define DeAllocA(allocator, mem)                         DeAllocateAligned(allocator, &(mem))
#define ReAllocA(Type, allocator, mem, count, alignment) cast(Type *, ReAllocateAligned(allocator, &(mem), sizeof(Type) * (count), alignment))
