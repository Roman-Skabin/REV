//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/allocator.h"

#pragma warning(push)
#pragma warning(disable: 4133)

#if DEBUG
    u64 gAllocationsPerFrame   = 0;
    u64 gReAllocationsPerFrame = 0;
    u64 gDeAllocationsPerFrame = 0;
#endif

typedef enum BLOCK_STATE
{
    BLOCK_STATE_NONE,
    BLOCK_STATE_ALLOCATED,
    BLOCK_STATE_IN_FREE_LIST,
} BLOCK_STATE;

struct BlockHeader
{
    BLOCK_STATE  block_state;
    s32          reserved;
    u64          data_bytes;
    BlockHeader *prev;
    // @NOTE(Roman): My be null. Used only in free list
    // @Issue(Roman): Do we need BlockHeader *prev_free?
    // @TODO(Roman): BlockHeader *next_free;
    byte         data[0];
};

void CreateAllocator(
    in  Allocator *allocator,
    opt void      *base_address,
    in  u64        capacity,
    in  b32        clear_memory)
{
    Check(allocator);
    CheckM(capacity, "Allocator's capacity can't be 0");

    if (!base_address)
    {
        DebugResult(allocator->base = VirtualAlloc(null, capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        allocator->reserved = true;
    }
    else
    {
        if (clear_memory) ZeroMemory(base_address, capacity);
        allocator->base = base_address;
    }

    allocator->used = 0;
    allocator->cap  = capacity;
}

void DestroyAllocator(
    in Allocator *allocator)
{
    Check(allocator);
    if (allocator->reserved)
    {
        DebugResult(VirtualFree(allocator->base, 0, MEM_RELEASE));
    }
    ZeroMemory(allocator, sizeof(Allocator));
}

internal BlockHeader *FindBestMatch(
    in Allocator *allocator,
    in u64        bytes)
{
    BlockHeader *best_in_list  = null;
    BlockHeader *first_in_list = null;
    BlockHeader *header        = null;
    BlockHeader *prev_header   = null;

    for (header = allocator->base;
         header < allocator->base + allocator->cap - sizeof(BlockHeader) && header->block_state != BLOCK_STATE_NONE;
         prev_header = header, header = header->data + header->data_bytes)
    {
        if (header->block_state == BLOCK_STATE_IN_FREE_LIST)
        {
            if (header->data_bytes == bytes)
            {
                best_in_list = header;
                break;
            }
            else if (!first_in_list
                 &&  (   cast(BlockHeader *, header->data + header->data_bytes)->block_state == BLOCK_STATE_NONE
                      || header->data_bytes > bytes + sizeof(BlockHeader)))
            {
                first_in_list = header;
            }
        }
    }

    if (best_in_list)
    {
        best_in_list->block_state  = BLOCK_STATE_ALLOCATED;
        allocator->used           += best_in_list->data_bytes;
        return best_in_list;
    }

    if (first_in_list)
    {
        if (cast(BlockHeader *, first_in_list->data + first_in_list->data_bytes)->block_state == BLOCK_STATE_NONE)
        {
            first_in_list->block_state = BLOCK_STATE_ALLOCATED;
            first_in_list->data_bytes  = bytes;
            allocator->used += first_in_list->data_bytes;
            return first_in_list;
        }
        if (first_in_list->data_bytes > bytes + sizeof(BlockHeader))
        {
            first_in_list->block_state = BLOCK_STATE_ALLOCATED;

            BlockHeader *new_next_header = first_in_list->data + bytes;
            new_next_header->block_state = BLOCK_STATE_IN_FREE_LIST;
            new_next_header->data_bytes  = first_in_list->data_bytes - bytes - sizeof(BlockHeader);
            new_next_header->prev        = first_in_list;

            cast(BlockHeader *, first_in_list->data + first_in_list->data_bytes)->prev = new_next_header;

            first_in_list->data_bytes  = bytes;
            allocator->used           += first_in_list->data_bytes + sizeof(BlockHeader);

            return first_in_list;
        }
        FailedM("Internal allocator error");
    }

    if (header)
    {
        header->block_state  = BLOCK_STATE_ALLOCATED;
        header->data_bytes   = bytes;
        header->prev         = prev_header;
        allocator->used     += sizeof(BlockHeader) + header->data_bytes;
        return header;
    }

    return null;
}

void *Allocate(
    in Allocator *allocator,
    in u64        bytes)
{
    Check(allocator);
    CheckM(bytes, "bytes can't be 0")

    CheckM(bytes <= allocator->cap - allocator->used,
           "Memory overflow.\n"
           "Bytes to allocate: %I64u.\n"
           "Remain allocator capacity: %I64u.\n"
           "Maximum bytes can be allocated: %I64u",
           bytes,
           allocator->cap - allocator->used,
           allocator->cap - allocator->used - sizeof(BlockHeader));

    BlockHeader *header = FindBestMatch(allocator, bytes);
    CheckM(header,
           "Memory overflow.\n"
           "Bytes to allocate: %I64u.\n"
           "Remain allocator capacity: %I64u.\n"
           "Maximum bytes can be allocated: %I64u\n",
           bytes,
           allocator->cap - allocator->used,
           allocator->cap - allocator->used - sizeof(BlockHeader));

    #if DEBUG
        ++gAllocationsPerFrame;
    #endif

    return header->data;
}

internal void MergeNearbyBlocksInFreeList(
    in Allocator   *allocator,
    in BlockHeader *header)
{
    BlockHeader *next_header = header->data + header->data_bytes;
    if (next_header->block_state == BLOCK_STATE_IN_FREE_LIST)
    {
        header->data_bytes += sizeof(BlockHeader) + next_header->data_bytes;
        allocator->used    -= sizeof(BlockHeader);
        ZeroMemory(next_header, sizeof(BlockHeader));
    }
    else if (next_header->block_state == BLOCK_STATE_NONE)
    {
        allocator->used -= sizeof(BlockHeader);
        ZeroMemory(header, sizeof(BlockHeader));
    }

    BlockHeader *prev_header = header->prev;
    if (prev_header && prev_header->block_state == BLOCK_STATE_IN_FREE_LIST)
    {
        prev_header->data_bytes += sizeof(BlockHeader) + header->data_bytes;
        allocator->used         -= sizeof(BlockHeader);
        if (header->block_state == BLOCK_STATE_NONE)
        {
            allocator->used -= sizeof(BlockHeader);
            ZeroMemory(prev_header, sizeof(BlockHeader));
        }
        ZeroMemory(header, sizeof(BlockHeader));
    }
}

void DeAllocate(
    in Allocator  *allocator,
    in void      **mem)
{
    Check(allocator);
    if (mem && *mem)
    {
        Check(allocator->base->data <= *mem && *mem < allocator->base + allocator->cap);

        BlockHeader *header = cast(byte *, *mem) - sizeof(BlockHeader);
        Check(header->block_state == BLOCK_STATE_ALLOCATED);

        header->block_state = BLOCK_STATE_IN_FREE_LIST;
        ZeroMemory(header->data, header->data_bytes);

        allocator->used -= header->data_bytes;

        MergeNearbyBlocksInFreeList(allocator, header);

        #if DEBUG
            ++gDeAllocationsPerFrame;
        #endif

        *mem = null;
    }
}

void *ReAllocate(
    in  Allocator  *allocator,
    opt void      **mem,
    opt u64         bytes)
{
    if (!mem || !*mem)
    {
        return Allocate(allocator, bytes);
    }

    if (!bytes)
    {
        DeAllocate(allocator, mem);
        return null;
    }

    Check(allocator);
    CheckM(allocator->base->data <= *mem && *mem < allocator->base + allocator->cap,
           "This memory block (%p) doesn't belong to this allocator [%p; %p].",
           mem,
           allocator->base->data,
           allocator->base + allocator->cap - 1);

    BlockHeader *header = cast(byte *, *mem) - sizeof(BlockHeader);

    CheckM(header->block_state == BLOCK_STATE_ALLOCATED,
           "This memory block (%p) is not allocated yet/already",
           mem);

    CheckM(cast(s64, header->data_bytes - bytes) <= 0 ? true : header->data_bytes - bytes <= allocator->cap - allocator->used,
           "Memory overflow.\n"
           "Additional bytes to allocate: %I64u.\n"
           "Remain allocator capacity: %I64u.\n"
           "Maximum bytes can be allocated: %I64u",
           header->data_bytes - bytes,
           allocator->cap - allocator->used,
           allocator->cap - allocator->used - sizeof(BlockHeader));

    if (header->data_bytes == bytes)
    {
        return *mem;
    }

    BlockHeader *next_header = header->data + header->data_bytes;
    BlockHeader *new_header  = null;

    if (next_header->block_state == BLOCK_STATE_IN_FREE_LIST
    && header->data_bytes + sizeof(BlockHeader) + next_header->data_bytes == bytes)
    {
        allocator->used    += bytes - header->data_bytes;
        header->data_bytes  = bytes;
        BlockHeader *nexts_next = next_header->data + next_header->data_bytes;
        if (nexts_next->block_state != BLOCK_STATE_NONE)
        {
            nexts_next->prev = header;
        }
        ZeroMemory(next_header, sizeof(BlockHeader));
        new_header = header;
    }
    else if (next_header->block_state == BLOCK_STATE_NONE)
    {
        allocator->used    += bytes - header->data_bytes;
        header->data_bytes  = bytes;
        new_header          = header;
    }

    if (!new_header)
    {
        if (new_header = FindBestMatch(allocator, bytes))
        {
            CopyMemory(new_header->data, header->data, min(new_header->data_bytes, header->data_bytes));
            header->block_state = BLOCK_STATE_IN_FREE_LIST;
            ZeroMemory(header->data, header->data_bytes);
            allocator->used -= header->data_bytes;
            MergeNearbyBlocksInFreeList(allocator, header);
        }
        CheckM(new_header,
               "Memory overflow.\n"
               "Bytes to allocate: %I64u.\n"
               "Remain allocator capacity: %I64u.\n"
               "Maximum bytes can be allocated: %I64u",
               bytes,
               allocator->cap - allocator->used,
               allocator->cap - allocator->used - sizeof(BlockHeader));
    }

    *mem = null;

    #if DEBUG
        ++gReAllocationsPerFrame;
    #endif

    return new_header->data;
}

void *AllocateAligned(
    in  Allocator *allocator,
    in  u64        bytes,
    opt u64        alignment)
{
    if (alignment < CENGINE_DEFAULT_ALIGNMENT) alignment = CENGINE_DEFAULT_ALIGNMENT;
    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");
    u64 aligned_bytes = ALIGN_UP(bytes, alignment);
    return Allocate(allocator, aligned_bytes);
}

void DeAllocateAligned(
    in Allocator  *allocator,
    in void      **mem)
{
    DeAllocate(allocator, mem);
}

void *ReAllocateAligned(
    in  Allocator  *allocator,
    opt void      **mem,
    opt u64         bytes,
    opt u64         alignment)
{
    if (alignment < CENGINE_DEFAULT_ALIGNMENT) alignment = CENGINE_DEFAULT_ALIGNMENT;
    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");
    u64 aligned_bytes = ALIGN_UP(bytes, alignment);
    return ReAllocate(allocator, mem, aligned_bytes);
}

#pragma warning(pop)
