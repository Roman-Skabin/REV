//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/allocator.h"
#include "core/memory.h"

#pragma warning(push)
#pragma warning(disable: 4133)

typedef enum BLOCK_STATE
{
    BLOCK_STATE_NONE,
    BLOCK_STATE_ALLOCATED,
    BLOCK_STATE_IN_FREE_LIST,
} BLOCK_STATE;

struct BlockHeader
{
    BLOCK_STATE  block_state;
    u64          data_bytes;
    BlockHeader *prev;
    BlockHeader *next_free;
    byte         data[0];
};

#define MemInAllocatorRange(allocator, mem)     ((allocator)->first->data <= (mem)   && (mem)   < cast(byte *, (allocator)->first) + (allocator)->cap)
#define BlockInAllocatorRange(allocator, block) ((allocator)->first       <= (block) && (block) < cast(byte *, (allocator)->first) + (allocator)->cap - sizeof(BlockHeader))

void CreateAllocator(
    in  Allocator *allocator,
    opt void      *base_address,
    in  u64        capacity,
    in  b32        clear_memory)
{
    Check(allocator);
    CheckM(capacity, "Allocator's capacity can't be 0");

    allocator->free_list = null;
    allocator->used      = 0;
    allocator->cap       = ALIGN_UP(capacity, PAGE_SIZE);

    if (!base_address)
    {
        DebugResult(base_address = VirtualAlloc(null, allocator->cap, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        allocator->valloc_used = true;
    }
    else
    {
        if (clear_memory) ZeroMemory(base_address, allocator->cap);
        allocator->valloc_used = false;
    }

    allocator->first           = base_address;
    allocator->last_allocated  = allocator->first;

#if DEBUG
    allocator->allocations_count   = 0;
    allocator->reallocations_count = 0;
    allocator->deallocations_count = 0;
#endif
}

void DestroyAllocator(
    in Allocator *allocator)
{
    Check(allocator);
    if (allocator->valloc_used)
    {
        DebugResult(VirtualFree(allocator->first, 0, MEM_RELEASE));
    }
#if DEBUG
    else if (allocator->allocations_count > allocator->deallocations_count)
    {
        DebugFC(DEBUG_IN_CONSOLE, DEBUG_COLOR_ERROR, "Memory leak detected!!!:\n");
        DebugF(DEBUG_IN_CONSOLE, "    Allocations overall: %I64u\n", allocator->allocations_count);
        DebugF(DEBUG_IN_CONSOLE, "    Reallocations overall: %I64u\n", allocator->reallocations_count);
        DebugF(DEBUG_IN_CONSOLE, "    Deallocations overall: %I64u\n", allocator->deallocations_count);
    }
#endif
    ZeroMemory(allocator, sizeof(Allocator));
}

internal BlockHeader *FindBestMatch(
    in Allocator *allocator,
    in u64        bytes)
{
    BlockHeader *best_in_list  = null;
    BlockHeader *first_in_list = null;
    BlockHeader *prev_free     = null;

    for (BlockHeader *it = allocator->free_list;
         it;
         prev_free = it, it = it->next_free)
    {
        Check(it->block_state == BLOCK_STATE_IN_FREE_LIST);

        if (it->data_bytes == bytes)
        {
            best_in_list = it;
            break;
        }
        else if (!first_in_list && it->data_bytes > bytes + sizeof(BlockHeader))
        {
            first_in_list = it;
        }
    }

    if (best_in_list)
    {
        if (prev_free) prev_free->next_free = best_in_list->next_free;
        best_in_list->next_free = null;

        best_in_list->block_state  = BLOCK_STATE_ALLOCATED;
        allocator->used           += best_in_list->data_bytes;

        if (best_in_list > allocator->last_allocated) allocator->last_allocated = best_in_list;
        return best_in_list;
    }

    if (first_in_list)
    {
        if (cast(BlockHeader *, first_in_list->data + first_in_list->data_bytes)->block_state == BLOCK_STATE_NONE)
        {
            if (prev_free) prev_free->next_free = first_in_list->next_free;
            first_in_list->next_free = null;

            first_in_list->block_state  = BLOCK_STATE_ALLOCATED;
            first_in_list->data_bytes   = bytes;
            allocator->used            += first_in_list->data_bytes;
        }
        else
        {
            first_in_list->block_state = BLOCK_STATE_ALLOCATED;

            BlockHeader *new_next_header = first_in_list->data + bytes;
            new_next_header->block_state = BLOCK_STATE_IN_FREE_LIST;
            new_next_header->data_bytes  = first_in_list->data_bytes - bytes - sizeof(BlockHeader);
            new_next_header->prev        = first_in_list;
            new_next_header->next_free   = first_in_list->next_free;

            if (prev_free) prev_free->next_free = new_next_header;

            cast(BlockHeader *, first_in_list->data + first_in_list->data_bytes)->prev = new_next_header;

            first_in_list->data_bytes  = bytes;
            allocator->used           += first_in_list->data_bytes + sizeof(BlockHeader);
        }

        if (first_in_list > allocator->last_allocated) allocator->last_allocated = first_in_list;
        return first_in_list;
    }

    BlockHeader *header      = null;
    BlockHeader *prev_header = null;

    for (header = allocator->last_allocated;
         header < cast(byte *, allocator->first) + allocator->cap - sizeof(BlockHeader) && header->block_state != BLOCK_STATE_NONE;
         prev_header = header, header = header->data + header->data_bytes)
    {
    }

    if (header)
    {
        Check(header->block_state == BLOCK_STATE_NONE);

        header->block_state  = BLOCK_STATE_ALLOCATED;
        header->data_bytes   = bytes;
        header->prev         = prev_header;

        allocator->last_allocated  = header;
        allocator->used += sizeof(BlockHeader) + header->data_bytes;

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
    ++allocator->allocations_count;
#endif

    return header->data;
}

internal void MergeNearbyBlocksInFreeList(
    in Allocator   *allocator,
    in BlockHeader *header)
{
    BlockHeader *next_header = header->data + header->data_bytes;
    BlockHeader *prev_header = header->prev;

    if (BlockInAllocatorRange(allocator, next_header))
    {
        BlockHeader *prev_free = null;
        for (BlockHeader *it = header->prev; it; it = it->prev)
        {
            if (it->block_state == BLOCK_STATE_IN_FREE_LIST)
            {
                prev_free = it;
                break;
            }
        }

        if (next_header->block_state == BLOCK_STATE_IN_FREE_LIST)
        {
            BlockHeader *nexts_next = next_header->data + next_header->data_bytes;
            if (BlockInAllocatorRange(allocator, nexts_next))
            {
                CheckM(nexts_next->block_state != BLOCK_STATE_IN_FREE_LIST,
                       "Next's next block can't be in free list because next block is already in free list. "
                       "There's some error in MergeNearbyBlocksInFreeList or in ReallocateInplace.");

                if (nexts_next->block_state != BLOCK_STATE_NONE)
                {
                    nexts_next->prev = header;
                }
            }

            header->data_bytes += sizeof(BlockHeader) + next_header->data_bytes;
            header->next_free   = next_header->next_free;

            if (prev_free) prev_free->next_free  = header;

            allocator->used -= sizeof(BlockHeader);

            ZeroMemory(next_header, sizeof(BlockHeader));

            if (header == allocator->last_allocated)
            {
                for (BlockHeader *it = header->prev; it; it = it->prev)
                {
                    if (it->block_state == BLOCK_STATE_ALLOCATED)
                    {
                        allocator->last_allocated = it;
                        break;
                    }
                }
            }
        }
        else if (next_header->block_state == BLOCK_STATE_NONE)
        {
            if (header == allocator->last_allocated)
            {
                for (BlockHeader *it = header->prev; it; it = it->prev)
                {
                    if (it->block_state == BLOCK_STATE_ALLOCATED)
                    {
                        allocator->last_allocated = it;
                        break;
                    }
                }
            }
            
            if (prev_free) prev_free->next_free = null;

            allocator->used -= sizeof(BlockHeader);
            ZeroMemory(header, sizeof(BlockHeader));
        }
    }

    if (prev_header && prev_header->block_state == BLOCK_STATE_IN_FREE_LIST)
    {
        MergeNearbyBlocksInFreeList(allocator, prev_header);
    }
}

void DeAllocate(
    in Allocator  *allocator,
    in void      **mem)
{
    Check(allocator);
    if (mem && *mem)
    {
        CheckM(allocator->first->data <= *mem && *mem < cast(byte *, allocator->first) + allocator->cap,
               "This memory block (0x%p) doesn't belong to this allocator [0x%p; 0x%p].",
               mem,
               allocator->first->data,
               cast(byte *, allocator->first) + allocator->cap - 1);

        BlockHeader *header = cast(byte *, *mem) - sizeof(BlockHeader);
        CheckM(header->block_state == BLOCK_STATE_ALLOCATED,
               "This memory block (0x%p) is not allocated yet/already",
               mem);

        header->block_state = BLOCK_STATE_IN_FREE_LIST;
        ZeroMemory(header->data, header->data_bytes);

        allocator->used -= header->data_bytes;

        MergeNearbyBlocksInFreeList(allocator, header);

    #if DEBUG
        ++allocator->deallocations_count;
    #endif

        *mem = null;
    }
}

internal BlockHeader *ReAllocateInplace(
    in Allocator   *allocator,
    in BlockHeader *header,
    in u64          bytes)
{
    BlockHeader *res_header  = null;
    BlockHeader *next_header = header->data + header->data_bytes;

    if (BlockInAllocatorRange(allocator, next_header))
    {
        BlockHeader *prev_free = null;
        for (BlockHeader *it = header; it; it = it->prev)
        {
            if (it->block_state == BLOCK_STATE_IN_FREE_LIST)
            {
                prev_free = it;
                break;
            }
        }

        if (next_header->block_state == BLOCK_STATE_IN_FREE_LIST)
        {
            if (header->data_bytes + sizeof(BlockHeader) + next_header->data_bytes == bytes)
            {
                allocator->used    += bytes - header->data_bytes;
                header->data_bytes  = bytes;

                BlockHeader *nexts_next = next_header->data + next_header->data_bytes;
                if (BlockInAllocatorRange(allocator, nexts_next))
                {
                    CheckM(nexts_next->block_state != BLOCK_STATE_IN_FREE_LIST,
                           "Next's next block can't be in free list because next block is already in free list. "
                           "There's some error in MergeNearbyBlocksInFreeList or in ReallocateInplace.");

                    if (nexts_next->block_state != BLOCK_STATE_NONE)
                    {
                        nexts_next->prev = header;
                    }
                }

                if (prev_free)
                {
                    prev_free->next_free = next_header->next_free;
                }

                ZeroMemory(next_header, sizeof(BlockHeader));
                res_header = header;
            }
            else if (bytes > header->data_bytes && header->data_bytes + next_header->data_bytes > bytes)
            {
                u64         new_next_header_bytes = header->data_bytes + next_header->data_bytes - bytes;
                BlockHeader next_save             = *next_header;

                allocator->used    += bytes - header->data_bytes;
                header->data_bytes  = bytes;

                BlockHeader *new_next_header = header->data + header->data_bytes;
                new_next_header->block_state = BLOCK_STATE_IN_FREE_LIST;
                new_next_header->data_bytes  = new_next_header_bytes;
                new_next_header->prev        = header;
                new_next_header->next_free   = next_save.next_free;

                BlockHeader *nexts_next = next_save.data + next_save.data_bytes;
                if (BlockInAllocatorRange(allocator, nexts_next))
                {
                    CheckM(nexts_next->block_state != BLOCK_STATE_IN_FREE_LIST,
                           "Next's next block can't be in free list because next block is already in free list. "
                           "There's some error in MergeNearbyBlocksInFreeList or in ReallocateInplace.");

                    if (nexts_next->block_state != BLOCK_STATE_NONE)
                    {
                        nexts_next->prev = new_next_header;
                    }
                }

                if (prev_free)
                {
                    prev_free->next_free = new_next_header;
                }

                ZeroMemory(next_header, new_next_header - next_header);
                res_header = header;
            }
            else if (header->data_bytes > bytes)
            {
                u64         delta_bytse = header->data_bytes - bytes;
                BlockHeader next_save   = *next_header;

                allocator->used    -= delta_bytse;
                header->data_bytes  = bytes;

                BlockHeader *new_next_header = header->data + header->data_bytes;
                new_next_header->block_state = BLOCK_STATE_IN_FREE_LIST;
                new_next_header->data_bytes  = next_save.data_bytes + delta_bytse;
                new_next_header->prev        = header;
                new_next_header->next_free   = next_save.next_free;
                
                BlockHeader *nexts_next = next_save.data + next_save.data_bytes;
                if (BlockInAllocatorRange(allocator, nexts_next))
                {
                    CheckM(nexts_next->block_state != BLOCK_STATE_IN_FREE_LIST,
                           "Next's next block can't be in free list because next block is already in free list. "
                           "There's some error in MergeNearbyBlocksInFreeList or in ReallocateInplace.");

                    if (nexts_next->block_state != BLOCK_STATE_NONE)
                    {
                        nexts_next->prev = new_next_header;
                    }
                }

                if (prev_free)
                {
                    prev_free->next_free = new_next_header;
                }

                ZeroMemory(new_next_header->data, delta_bytse);
                res_header = header;
            }
        }
        else if (next_header->block_state == BLOCK_STATE_NONE)
        {
            if (bytes < header->data_bytes)
            {
                allocator->used -= header->data_bytes - bytes;

                ZeroMemory(header->data + bytes, header->data_bytes - bytes);

                header->data_bytes = bytes;
                res_header         = header;
            }
            else
            {
                allocator->used    += bytes - header->data_bytes;
                header->data_bytes  = bytes;
                res_header          = header;
            }
        }
    }

    return res_header;
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
    CheckM(MemInAllocatorRange(allocator, *mem),
           "This memory block (0x%p) doesn't belong to this allocator [0x%p; 0x%p].",
           mem,
           allocator->first->data,
           cast(byte *, allocator->first) + allocator->cap - 1);

    BlockHeader *header = cast(byte *, *mem) - sizeof(BlockHeader);

    CheckM(header->block_state == BLOCK_STATE_ALLOCATED,
           "This memory block (0x%p) is not allocated yet/already.",
           mem);

    if (header->data_bytes == bytes)
    {
        return *mem;
    }

    CheckM(bytes < header->data_bytes || bytes - header->data_bytes <= allocator->cap - allocator->used,
           "Memory overflow.\n"
           "Additional bytes to allocate: %I64u.\n"
           "Remain allocator capacity: %I64u.\n"
           "Maximum bytes can be allocated: %I64u",
           bytes - header->data_bytes,
           allocator->cap - allocator->used,
           allocator->cap - allocator->used - sizeof(BlockHeader));

    BlockHeader *new_header = ReAllocateInplace(allocator, header, bytes);

    if (!new_header)
    {
        DebugResultM(new_header = FindBestMatch(allocator, bytes),
                     "Memory overflow.\n"
                     "Bytes to allocate: %I64u.\n"
                     "Remain allocator capacity: %I64u.\n"
                     "Maximum bytes can be allocated: %I64u",
                     bytes,
                     allocator->cap - allocator->used,
                     allocator->cap - allocator->used - sizeof(BlockHeader));

        CopyMemory(new_header->data, header->data, min(new_header->data_bytes, header->data_bytes));

        header->block_state = BLOCK_STATE_IN_FREE_LIST;
        ZeroMemory(header->data, header->data_bytes);

        allocator->used -= header->data_bytes;

        MergeNearbyBlocksInFreeList(allocator, header);
    }

    *mem = null;

#if DEBUG
    ++allocator->reallocations_count;
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
    return Allocate(allocator, ALIGN_UP(bytes, alignment));
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
    return ReAllocate(allocator, mem, ALIGN_UP(bytes, alignment));
}

#pragma warning(pop)
