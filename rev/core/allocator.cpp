//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/allocator.h"
#include "memory/memlow.h"
#include "math/math.h"

namespace REV
{

enum class BLOCK_STATE
{
    NONE,
    ALLOCATED,
    IN_FREE_LIST,
};

struct BlockHeader
{
    BLOCK_STATE  block_state;
    u64          data_bytes;
    BlockHeader *prev;
    BlockHeader *next_free;
    byte         data[0];
};

Allocator::Allocator(void *base_address, u64 capacity, bool clear_memory, const ConstString& name)
    : m_FreeList(null),
      m_First(null),
      m_LastAllocated(null),
      m_Used(0),
      m_Capacity(AlignUp(capacity, PAGE_SIZE)),
#if REV_DEBUG
      m_AllocationsCount(0),
      m_ReAllocationsCount(0),
      m_DeAllocationsCount(0),
      m_MaxMemoryUsed(0),
#endif
      m_CriticalSection(),
      m_Name(name),
      m_VallocUsed(false)
{
    REV_CHECK_M(m_Capacity, "Allocator \"%s\", Allocator's capacity can't be 0", m_Name.Data());

    if (!base_address)
    {
    #if REV_PLATFORM_WIN64
        REV_DEBUG_RESULT_M(base_address = VirtualAlloc(null, m_Capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE), "Allocator \"%s\": Internal error", m_Name.Data());
    #else
        base_address = mmap(null, m_Capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        REV_CHECK_M(base_address != MAP_FAILED, "Allocator \"%s\": Internal error", m_Name.Data());
    #endif
        m_VallocUsed = true;
    }
    else
    {
        if (clear_memory) ZeroMemory(base_address, m_Capacity);
        m_VallocUsed = false;
    }

    m_First         = cast<BlockHeader *>(base_address);
    m_LastAllocated = m_First;
}

Allocator::Allocator(Allocator&& other) noexcept
    : m_FreeList(other.m_FreeList),
      m_First(other.m_First),
      m_LastAllocated(other.m_LastAllocated),
      m_Used(other.m_Used),
      m_Capacity(other.m_Capacity),
#if REV_DEBUG
      m_AllocationsCount(other.m_AllocationsCount),
      m_ReAllocationsCount(other.m_ReAllocationsCount),
      m_DeAllocationsCount(other.m_DeAllocationsCount),
#endif
      m_VallocUsed(other.m_VallocUsed),
      m_CriticalSection(RTTI::move(other.m_CriticalSection))
{
    ZeroMemory(&other, sizeof(Allocator));
}

Allocator::~Allocator()
{
    if (m_VallocUsed)
    {
        if (m_First)
        {
        #if REV_PLATFORM_WIN64
            REV_DEBUG_RESULT_M(VirtualFree(m_First, 0, MEM_RELEASE), "Allocator \"%s\": Internal error.", m_Name.Data());
        #else
            REV_DEBUG_RESULT_M(!munmap(m_First, m_Capacity), "Allocator \"%s\": Internal error.", m_Name.Data());
        #endif
        }
    }
#if REV_DEBUG
    if (m_AllocationsCount > m_DeAllocationsCount)
    {
        PrintDebugMessage(DEBUG_COLOR::ERROR, "Allocator \"%s\": Memory leak detected:", m_Name.Data());
        PrintDebugMessage(DEBUG_COLOR::ERROR, "    Allocations overall: %I64u", m_AllocationsCount);
        PrintDebugMessage(DEBUG_COLOR::ERROR, "    Reallocations overall: %I64u", m_ReAllocationsCount);
        PrintDebugMessage(DEBUG_COLOR::ERROR, "    Deallocations overall: %I64u", m_DeAllocationsCount);
    }
    else
    {
        PrintDebugMessage(DEBUG_COLOR::INFO, "Allocator \"%s\": Stats:", m_Name.Data());
        PrintDebugMessage(DEBUG_COLOR::INFO, "    Allocations overall: %I64u", m_AllocationsCount);
        PrintDebugMessage(DEBUG_COLOR::INFO, "    Reallocations overall: %I64u", m_ReAllocationsCount);
        PrintDebugMessage(DEBUG_COLOR::INFO, "    Deallocations overall: %I64u", m_DeAllocationsCount);
    }
    PrintDebugMessage(DEBUG_COLOR::INFO, "    Max memory used: %I64u B = %f KB = %f MB = %f GB", m_MaxMemoryUsed,
                                                                                                 m_MaxMemoryUsed / 1024.0f,
                                                                                                 m_MaxMemoryUsed / 1048576.0f,
                                                                                                 m_MaxMemoryUsed / 1073741824.0f);
    PrintDebugMessage(DEBUG_COLOR::INFO, "    Capacity: %I64u B = %f KB = %f MB = %f GB", m_Capacity,
                                                                                          m_Capacity / 1024.0f,
                                                                                          m_Capacity / 1048576.0f,
                                                                                          m_Capacity / 1073741824.0f);
#endif
    ZeroMemory(this, REV_StructFieldOffset(Allocator, m_CriticalSection));
}

BlockHeader *Allocator::FindBestMatch(u64 bytes)
{
    BlockHeader *first_in_list = null;
    BlockHeader *prev_free     = null;

    for (BlockHeader *it = m_FreeList; it; prev_free = it, it = it->next_free)
    {
        REV_CHECK_M(it->block_state == BLOCK_STATE::IN_FREE_LIST,
                    "Allocator \"%s\": Internal error.\n"
                    "    Blocks in the free list gotta be IN_FREE_LIST state.",
                    m_Name.Data());

        if (it->data_bytes == bytes)
        {
            if (prev_free) prev_free->next_free = it->next_free;
            it->next_free = null;

            it->block_state  = BLOCK_STATE::ALLOCATED;
            m_Used          += it->data_bytes;

            if (it > m_LastAllocated) m_LastAllocated = it;
            return it;
        }
        else if (!first_in_list && it->data_bytes > bytes + sizeof(BlockHeader))
        {
            // @NOTE(Roman): sizeof(BlockHeader) for a new next block
            //               that will be placed between this block and next allocated one.
            //               Also new next block must be at least size of 1 byte,
            //               so we have at least (sizeof(BlockHeader) + 1) for a next block
            //               and <bytes> for this block.
            first_in_list = it;
            // @NOTE(Roman): We do not break here because we are optimistic
            //               and we are still searching for a best match in the if above.
        }
    }

    if (first_in_list)
    {
        BlockHeader *next_block = cast<BlockHeader *>(first_in_list->data + first_in_list->data_bytes);
        REV_CHECK_M(next_block->block_state == BLOCK_STATE::ALLOCATED,
                    "Allocator \"%s\": Internal error.\n"
                    "    We could not have next block in free list\n"
                    "    and we could not have it as a last block (free block before none block)\n"
                    "    because we are merging them on deallocation and/or on reallocation step.",
                    m_Name.Data());

        first_in_list->block_state = BLOCK_STATE::ALLOCATED;

        BlockHeader *new_next_header = cast<BlockHeader *>(first_in_list->data + bytes);
        new_next_header->block_state = BLOCK_STATE::IN_FREE_LIST;
        new_next_header->data_bytes  = first_in_list->data_bytes - bytes - sizeof(BlockHeader);
        new_next_header->prev        = first_in_list;
        new_next_header->next_free   = first_in_list->next_free;

        if (prev_free) prev_free->next_free = new_next_header;

        next_block->prev = new_next_header;

        first_in_list->data_bytes  = bytes;
        m_Used                    += first_in_list->data_bytes + sizeof(BlockHeader);

        if (first_in_list > m_LastAllocated) m_LastAllocated = first_in_list;
        return first_in_list;
    }

    // @NOTE(Roman): Theoretically we could not have free blocks after last allocated block.
    //               so our target block (that we want to find/return) will be next block
    //               right after last allocated block.
    if (m_LastAllocated->block_state == BLOCK_STATE::NONE)
    {
        BlockHeader *header = m_LastAllocated;

        header->block_state = BLOCK_STATE::ALLOCATED;
        header->data_bytes  = bytes;

        m_Used += sizeof(BlockHeader) + header->data_bytes;

        return header;
    }
    else
    {
        BlockHeader *header = cast<BlockHeader *>(m_LastAllocated->data + m_LastAllocated->data_bytes);

        if (BlockInAllocatorRange(header))
        {
            REV_CHECK_M(header->block_state == BLOCK_STATE::NONE,
                        "Allocator \"%s\": Internal error.\n"
                        "    This block gotta be none block because it is not in the free list nor allocated.",
                        m_Name.Data());

            header->block_state = BLOCK_STATE::ALLOCATED;
            header->data_bytes  = bytes;

            header->prev    = m_LastAllocated;
            m_LastAllocated = header;

            m_Used += sizeof(BlockHeader) + header->data_bytes;

            return header;
        }
    }

    return null;
}

void *Allocator::Allocate(u64 bytes)
{
    REV_CHECK_M(bytes, "Allocator \"%s\": bytes can't be 0", m_Name.Data());

    m_CriticalSection.Enter();

    REV_CHECK_M(bytes <= m_Capacity - m_Used,
                "Allocator \"%s\": Memory overflow.\n"
                "    Bytes to allocate: %I64u.\n"
                "    Remain allocator capacity: %I64u.\n"
                "    Maximum bytes can be allocated: %I64u",
                m_Name.Data(),
                bytes,
                m_Capacity - m_Used,
                m_Capacity - m_Used - sizeof(BlockHeader));

    BlockHeader *header = FindBestMatch(bytes);
    REV_CHECK_M(header,
                "Allocator \"%s\": Memory overflow.\n"
                "    Bytes to allocate: %I64u.\n"
                "    Remain allocator capacity: %I64u.\n"
                "    Maximum bytes can be allocated: %I64u\n",
                m_Name.Data(),
                bytes,
                m_Capacity - m_Used,
                m_Capacity - m_Used - sizeof(BlockHeader));

#if REV_DEBUG
    ++m_AllocationsCount;
    m_MaxMemoryUsed = Math::max(m_MaxMemoryUsed, m_Used);
#endif

    m_CriticalSection.Leave();

    return header->data;
}

void Allocator::MergeNearbyBlocksInFreeList(BlockHeader *header)
{
    BlockHeader *next_header = cast<BlockHeader *>(header->data + header->data_bytes);
    BlockHeader *prev_header = header->prev;

    if (BlockInAllocatorRange(next_header))
    {
        BlockHeader *prev_free = null;
        for (BlockHeader *it = header->prev; it; it = it->prev)
        {
            if (it->block_state == BLOCK_STATE::IN_FREE_LIST)
            {
                prev_free = it;
                break;
            }
        }

        if (next_header->block_state == BLOCK_STATE::IN_FREE_LIST)
        {
            REV_CHECK_M(header != m_LastAllocated,
                        "Allocator \"%s\": Internal error.\n"
                        "    If next block is in free list, so we have to have allocated block(s) after it.",
                        m_Name.Data());

            BlockHeader *nexts_next = cast<BlockHeader *>(next_header->data + next_header->data_bytes);
            if (BlockInAllocatorRange(nexts_next))
            {
                REV_CHECK_M(nexts_next->block_state == BLOCK_STATE::ALLOCATED,
                            "Allocator \"%s\": Internal error.\n"
                            "    Next's next block gotta be allocated: it can't be in free list, otherwise next block had to be merged with it\n"
                            "    and it can't be none, otherwise next block had to be none already.",
                            m_Name.Data());

                if (nexts_next->block_state != BLOCK_STATE::NONE)
                {
                    nexts_next->prev = header;
                }
            }

            header->data_bytes += sizeof(BlockHeader) + next_header->data_bytes;
            header->next_free   = next_header->next_free;

            if (prev_free) prev_free->next_free = header;

            m_Used -= sizeof(BlockHeader);
            ZeroMemory(next_header, sizeof(BlockHeader));
        }
        else if (next_header->block_state == BLOCK_STATE::NONE)
        {
            REV_CHECK_M(header >= m_LastAllocated,
                        "Allocator \"%s\": Internal error.\n"
                        "    If next block is none, then our block must be last allocated.\n"
                        "    Or if we're merging previous block, then it gotta be after last allocated block.",
                        m_Name.Data());

            for (BlockHeader *it = header->prev; it; it = it->prev)
            {
                if (it->block_state == BLOCK_STATE::ALLOCATED)
                {
                    m_LastAllocated = it;
                    break;
                }
            }

            if (prev_free) prev_free->next_free = null;

            m_Used -= sizeof(BlockHeader);
            ZeroMemory(header, sizeof(BlockHeader));
        }
    }

    if (prev_header && prev_header->block_state == BLOCK_STATE::IN_FREE_LIST)
    {
        MergeNearbyBlocksInFreeList(prev_header);
    }
}

void Allocator::DeAllocate(void *&mem)
{
    if (mem)
    {
        m_CriticalSection.Enter();

        REV_CHECK_M(MemInAllocatorRange(mem),
                    "Allocator \"%s\": This memory block (0x%p) doesn't belong to this allocator [0x%p; 0x%p].",
                    m_Name.Data(),
                    mem,
                    m_First->data,
                    cast<byte *>(m_First) + m_Capacity - 1);

        BlockHeader *header = cast<BlockHeader *>(cast<byte *>(mem) - sizeof(BlockHeader));
        REV_CHECK_M(header->block_state == BLOCK_STATE::ALLOCATED,
                    "Allocator \"%s\": This memory block (0x%p) is not allocated yet/already",
                    m_Name.Data(),
                    mem);

        header->block_state = BLOCK_STATE::IN_FREE_LIST;
        ZeroMemory(header->data, header->data_bytes);

        m_Used -= header->data_bytes;

        MergeNearbyBlocksInFreeList(header);

    #if REV_DEBUG
        ++m_DeAllocationsCount;
    #endif
    
        mem = null;

        m_CriticalSection.Leave();
    }
}

BlockHeader *Allocator::ReAllocateInplace(BlockHeader *header, u64 bytes)
{
    BlockHeader *res_header  = null;
    BlockHeader *next_header = cast<BlockHeader *>(header->data + header->data_bytes);

    if (BlockInAllocatorRange(next_header))
    {
        BlockHeader *prev_free = null;
        for (BlockHeader *it = header; it; it = it->prev)
        {
            if (it->block_state == BLOCK_STATE::IN_FREE_LIST)
            {
                prev_free = it;
                break;
            }
        }

        if (next_header->block_state == BLOCK_STATE::IN_FREE_LIST)
        {
            if (header->data_bytes + sizeof(BlockHeader) + next_header->data_bytes == bytes)
            {
                m_Used             += bytes - header->data_bytes;
                header->data_bytes  = bytes;

                BlockHeader *nexts_next = cast<BlockHeader *>(next_header->data + next_header->data_bytes);
                if (BlockInAllocatorRange(nexts_next))
                {
                    REV_CHECK_M(nexts_next->block_state != BLOCK_STATE::IN_FREE_LIST,
                                "Allocator \"%s\": Internal error.\n"
                                "    Next's next block can't be free list because next block is already free list.\n"
                                "    There's some error MergeNearbyBlocksInFreeList or ReallocateInplace.",
                                m_Name.Data());

                    if (nexts_next->block_state != BLOCK_STATE::NONE)
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

                m_Used             += bytes - header->data_bytes;
                header->data_bytes  = bytes;

                BlockHeader *new_next_header = cast<BlockHeader *>(header->data + header->data_bytes);
                new_next_header->block_state = BLOCK_STATE::IN_FREE_LIST;
                new_next_header->data_bytes  = new_next_header_bytes;
                new_next_header->prev        = header;
                new_next_header->next_free   = next_save.next_free;

                BlockHeader *nexts_next = cast<BlockHeader *>(next_save.data + next_save.data_bytes);
                if (BlockInAllocatorRange(nexts_next))
                {
                    REV_CHECK_M(nexts_next->block_state != BLOCK_STATE::IN_FREE_LIST,
                                "Allocator \"%s\": Internal error.\n"
                                "    Next's next block can't be free list because next block is already free list.\n"
                                "    There's some error MergeNearbyBlocksInFreeList or ReallocateInplace.",
                                m_Name.Data());

                    if (nexts_next->block_state != BLOCK_STATE::NONE)
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

                m_Used             -= delta_bytse;
                header->data_bytes  = bytes;

                BlockHeader *new_next_header = cast<BlockHeader *>(header->data + header->data_bytes);
                new_next_header->block_state = BLOCK_STATE::IN_FREE_LIST;
                new_next_header->data_bytes  = next_save.data_bytes + delta_bytse;
                new_next_header->prev        = header;
                new_next_header->next_free   = next_save.next_free;
                
                BlockHeader *nexts_next = cast<BlockHeader *>(next_save.data + next_save.data_bytes);
                if (BlockInAllocatorRange(nexts_next))
                {
                    REV_CHECK_M(nexts_next->block_state != BLOCK_STATE::IN_FREE_LIST,
                                "Allocator \"%s\": Internal error.\n"
                                "    Next's next block can't be free list because next block is already free list.\n"
                                "    There's some error MergeNearbyBlocksInFreeList or ReallocateInplace.",
                                m_Name.Data());

                    if (nexts_next->block_state != BLOCK_STATE::NONE)
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
        else if (next_header->block_state == BLOCK_STATE::NONE)
        {
            if (bytes < header->data_bytes)
            {
                m_Used -= header->data_bytes - bytes;

                ZeroMemory(header->data + bytes, header->data_bytes - bytes);

                header->data_bytes = bytes;
                res_header         = header;
            }
            else
            {
                m_Used             += bytes - header->data_bytes;
                header->data_bytes  = bytes;
                res_header          = header;
            }
        }
    }

    return res_header;
}

void *Allocator::ReAllocate(void *&mem, u64 bytes)
{
    if (!mem)
    {
        return Allocate(bytes);
    }

    if (!bytes)
    {
        DeAllocate(mem);
        return null;
    }

    m_CriticalSection.Enter();

    REV_CHECK_M(MemInAllocatorRange(mem),
                "Allocator \"%s\": This memory block (0x%p) doesn't belong to this allocator [0x%p; 0x%p].",
                m_Name.Data(),
                mem,
                m_First->data,
                cast<byte *>(m_First) + m_Capacity - 1);

    BlockHeader *header = cast<BlockHeader *>(cast<byte *>(mem) - sizeof(BlockHeader));

    REV_CHECK_M(header->block_state == BLOCK_STATE::ALLOCATED,
                "Allocator \"%s\": This memory block (0x%p) is not allocated yet/already.",
                m_Name.Data(),
                mem);

    if (header->data_bytes == bytes)
    {
        m_CriticalSection.Leave();
        return mem;
    }

    REV_CHECK_M(bytes < header->data_bytes || bytes - header->data_bytes <= m_Capacity - m_Used,
                "Allocator \"%s\": Memory overflow.\n"
                "    Additional bytes to allocate: %I64u.\n"
                "    Remain allocator capacity: %I64u.\n"
                "    Maximum bytes can be allocated: %I64u",
                m_Name.Data(),
                bytes - header->data_bytes,
                m_Capacity - m_Used,
                m_Capacity - m_Used - sizeof(BlockHeader));

    BlockHeader *new_header = ReAllocateInplace(header, bytes);

    if (!new_header)
    {
        REV_DEBUG_RESULT_M(new_header = FindBestMatch(bytes),
                           "Allocator \"%s\": Memory overflow.\n"
                           "    Bytes to allocate: %I64u.\n"
                           "    Remain allocator capacity: %I64u.\n"
                           "    Maximum bytes can be allocated: %I64u",
                           m_Name.Data(),
                           bytes,
                           m_Capacity - m_Used,
                           m_Capacity - m_Used - sizeof(BlockHeader));

        CopyMemory(new_header->data, header->data, Math::min(new_header->data_bytes, header->data_bytes));

        header->block_state = BLOCK_STATE::IN_FREE_LIST;
        ZeroMemory(header->data, header->data_bytes);

        m_Used -= header->data_bytes;

        MergeNearbyBlocksInFreeList(header);
    }

    mem = null;

#if REV_DEBUG
    ++m_ReAllocationsCount;
    m_MaxMemoryUsed = Math::max(m_MaxMemoryUsed, m_Used);
#endif

    m_CriticalSection.Leave();

    return new_header->data;
}

void *Allocator::AllocateAligned(u64 bytes, u64 alignment)
{
    if (alignment < REV::DEFAULT_ALIGNMENT) alignment = REV::DEFAULT_ALIGNMENT;
    REV_CHECK_M(IsPowOf2(alignment), "Allocator \"%s\": Alignment must be power of 2", m_Name.Data());
    return Allocate(AlignUp(bytes, alignment));
}

void Allocator::DeAllocateAligned(void *&mem)
{
    DeAllocate(mem);
}

void *Allocator::ReAllocateAligned(void *&mem, u64 bytes, u64 alignment)
{
    if (alignment < REV::DEFAULT_ALIGNMENT) alignment = REV::DEFAULT_ALIGNMENT;
    REV_CHECK_M(IsPowOf2(alignment), "Allocator \"%s\": Alignment must be power of 2", m_Name.Data());
    return ReAllocate(mem, AlignUp(bytes, alignment));
}

Allocator& Allocator::operator=(Allocator&& other) noexcept
{
    if (this != &other)
    {
        CopyMemory(this, &other, REV_StructFieldOffset(Allocator, m_CriticalSection));
        m_CriticalSection = RTTI::move(other.m_CriticalSection);

        ZeroMemory(&other, REV_StructFieldOffset(Allocator, m_CriticalSection));
    }
    return *this;
}

bool Allocator::MemInAllocatorRange(void *mem)
{
    m_CriticalSection.Enter();

    bool in = m_First->data <= mem && mem < cast<byte *>(m_First) + m_Capacity;

    m_CriticalSection.Leave();
    return in;
}

bool Allocator::BlockInAllocatorRange(BlockHeader *block)
{
    return m_First <= block && block < cast<BlockHeader *>(cast<byte *>(m_First) + m_Capacity - sizeof(BlockHeader));
}

}
