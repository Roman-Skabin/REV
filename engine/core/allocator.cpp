//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/allocator.h"
#include "core/memory.h"

enum class BLOCK_STATE
{
    NONE,
    ALLOCATED,
    IN_FREE_LIST,
};
ENUM_CLASS_OPERATORS(BLOCK_STATE)

struct BlockHeader
{
    BLOCK_STATE  block_state;
    u64          data_bytes;
    BlockHeader *prev;
    BlockHeader *next_free;
    #pragma warning(suppress: 4200)
    byte         data[0];
};

Allocator::Allocator(void *base_address, u64 capacity, bool clear_memory, const ConstString& name)
    : m_FreeList(null),
      m_First(null),
      m_LastAllocated(null),
      m_Used(0),
      m_Capacity(AlignUp(capacity, PAGE_SIZE)),
#if DEBUG
      m_AllocationsCount(0),
      m_ReAllocationsCount(0),
      m_DeAllocationsCount(0),
#endif
      m_CriticalSection(),
      m_Name(name),
      m_VallocUsed(false)
{
    CheckM(m_Capacity, "Allocator \"%s\", Allocator's capacity can't be 0", m_Name.Data());

    if (!base_address)
    {
        DebugResultM(base_address = VirtualAlloc(null, m_Capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE), "Allocator \"%s\": Internal error", m_Name.Data());
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
#if DEBUG
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
        if (m_First) DebugResultM(VirtualFree(m_First, 0, MEM_RELEASE), "Allocator \"%s\": Internal error.", m_Name.Data());
    }
#if DEBUG
    if (m_AllocationsCount > m_DeAllocationsCount)
    {
        DebugFC(DEBUG_COLOR::ERROR, "Allocator \"%s\": Memory leak detected:", m_Name.Data());
        DebugFC(DEBUG_COLOR::ERROR, "    Allocations overall: %I64u", m_AllocationsCount);
        DebugFC(DEBUG_COLOR::ERROR, "    Reallocations overall: %I64u", m_ReAllocationsCount);
        DebugFC(DEBUG_COLOR::ERROR, "    Deallocations overall: %I64u", m_DeAllocationsCount);
    }
    else
    {
        DebugFC(DEBUG_COLOR::INFO, "Allocator \"%s\": Stats:", m_Name.Data());
        DebugFC(DEBUG_COLOR::INFO, "    Allocations overall: %I64u", m_AllocationsCount);
        DebugFC(DEBUG_COLOR::INFO, "    Reallocations overall: %I64u", m_ReAllocationsCount);
        DebugFC(DEBUG_COLOR::INFO, "    Deallocations overall: %I64u", m_DeAllocationsCount);
    }
#endif
    ZeroMemory(this, StructFieldOffset(Allocator, m_CriticalSection));
}

BlockHeader *Allocator::FindBestMatch(u64 bytes)
{
    BlockHeader *best_in_list  = null;
    BlockHeader *first_in_list = null;
    BlockHeader *prev_free     = null;

    for (BlockHeader *it = m_FreeList;
         it;
         prev_free = it, it = it->next_free)
    {
        CheckM(it->block_state == BLOCK_STATE::IN_FREE_LIST, "Allocator \"%s\": Internal error", m_Name.Data());

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

        best_in_list->block_state  = BLOCK_STATE::ALLOCATED;
        m_Used                    += best_in_list->data_bytes;

        if (best_in_list > m_LastAllocated) m_LastAllocated = best_in_list;
        return best_in_list;
    }

    if (first_in_list)
    {
        if (cast<BlockHeader *>(first_in_list->data + first_in_list->data_bytes)->block_state == BLOCK_STATE::NONE)
        {
            if (prev_free) prev_free->next_free = first_in_list->next_free;
            first_in_list->next_free = null;

            first_in_list->block_state  = BLOCK_STATE::ALLOCATED;
            first_in_list->data_bytes   = bytes;
            m_Used                     += first_in_list->data_bytes;
        }
        else
        {
            first_in_list->block_state = BLOCK_STATE::ALLOCATED;

            BlockHeader *new_next_header = cast<BlockHeader *>(first_in_list->data + bytes);
            new_next_header->block_state = BLOCK_STATE::IN_FREE_LIST;
            new_next_header->data_bytes  = first_in_list->data_bytes - bytes - sizeof(BlockHeader);
            new_next_header->prev        = first_in_list;
            new_next_header->next_free   = first_in_list->next_free;

            if (prev_free) prev_free->next_free = new_next_header;

            cast<BlockHeader *>(first_in_list->data + first_in_list->data_bytes)->prev = new_next_header;

            first_in_list->data_bytes  = bytes;
            m_Used                    += first_in_list->data_bytes + sizeof(BlockHeader);
        }

        if (first_in_list > m_LastAllocated) m_LastAllocated = first_in_list;
        return first_in_list;
    }

    BlockHeader *header      = null;
    BlockHeader *prev_header = null;

    for (header = m_LastAllocated;
         header < cast<BlockHeader *>(cast<byte *>(m_First) + m_Capacity - sizeof(BlockHeader)) && header->block_state != BLOCK_STATE::NONE;
         prev_header = header, header = cast<BlockHeader *>(header->data + header->data_bytes))
    {
    }

    if (header)
    {
        CheckM(header->block_state == BLOCK_STATE::NONE, "Allocator \"%s\": Internal error", m_Name.Data());

        header->block_state = BLOCK_STATE::ALLOCATED;
        header->data_bytes  = bytes;
        header->prev        = prev_header;

        m_LastAllocated = header;
        m_Used += sizeof(BlockHeader) + header->data_bytes;

        return header;
    }

    return null;
}

void *Allocator::Allocate(u64 bytes)
{
    CheckM(bytes, "Allocator \"%s\": bytes can't be 0", m_Name.Data());

    m_CriticalSection.Enter();

    CheckM(bytes <= m_Capacity - m_Used,
           "Allocator \"%s\": Memory overflow.\n"
           "    Bytes to allocate: %I64u.\n"
           "    Remain allocator capacity: %I64u.\n"
           "    Maximum bytes can be allocated: %I64u",
           m_Name.Data(),
           bytes,
           m_Capacity - m_Used,
           m_Capacity - m_Used - sizeof(BlockHeader));

    BlockHeader *header = FindBestMatch(bytes);
    CheckM(header,
           "Allocator \"%s\": Memory overflow.\n"
           "    Bytes to allocate: %I64u.\n"
           "    Remain allocator capacity: %I64u.\n"
           "    Maximum bytes can be allocated: %I64u\n",
           m_Name.Data(),
           bytes,
           m_Capacity - m_Used,
           m_Capacity - m_Used - sizeof(BlockHeader));

#if DEBUG
    ++m_AllocationsCount;
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
            BlockHeader *nexts_next = cast<BlockHeader *>(next_header->data + next_header->data_bytes);
            if (BlockInAllocatorRange(nexts_next))
            {
                CheckM(nexts_next->block_state != BLOCK_STATE::IN_FREE_LIST,
                       "Allocator \"%s\": Internal error.\n"
                       "    Next's next block can't be free list because next block is already free list.\n"
                       "    There's some error MergeNearbyBlocksInFreeList or ReallocateInplace.",
                       m_Name.Data());

                if (nexts_next->block_state != BLOCK_STATE::NONE)
                {
                    nexts_next->prev = header;
                }
            }

            header->data_bytes += sizeof(BlockHeader) + next_header->data_bytes;
            header->next_free   = next_header->next_free;

            if (prev_free) prev_free->next_free  = header;

            m_Used -= sizeof(BlockHeader);

            ZeroMemory(next_header, sizeof(BlockHeader));

            if (header == m_LastAllocated)
            {
                for (BlockHeader *it = header->prev; it; it = it->prev)
                {
                    if (it->block_state == BLOCK_STATE::ALLOCATED)
                    {
                        m_LastAllocated = it;
                        break;
                    }
                }
            }
        }
        else if (next_header->block_state == BLOCK_STATE::NONE)
        {
            if (header == m_LastAllocated)
            {
                for (BlockHeader *it = header->prev; it; it = it->prev)
                {
                    if (it->block_state == BLOCK_STATE::ALLOCATED)
                    {
                        m_LastAllocated = it;
                        break;
                    }
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

        CheckM(MemInAllocatorRange(mem),
               "Allocator \"%s\": This memory block (0x%p) doesn't belong to this allocator [0x%p; 0x%p].",
               m_Name.Data(),
               mem,
               m_First->data,
               cast<byte *>(m_First) + m_Capacity - 1);

        BlockHeader *header = cast<BlockHeader *>(cast<byte *>(mem) - sizeof(BlockHeader));
        CheckM(header->block_state == BLOCK_STATE::ALLOCATED,
               "Allocator \"%s\": This memory block (0x%p) is not allocated yet/already",
               m_Name.Data(),
               mem);

        header->block_state = BLOCK_STATE::IN_FREE_LIST;
        ZeroMemory(header->data, header->data_bytes);

        m_Used -= header->data_bytes;

        MergeNearbyBlocksInFreeList(header);

    #if DEBUG
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
                    CheckM(nexts_next->block_state != BLOCK_STATE::IN_FREE_LIST,
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
                    CheckM(nexts_next->block_state != BLOCK_STATE::IN_FREE_LIST,
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
                    CheckM(nexts_next->block_state != BLOCK_STATE::IN_FREE_LIST,
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

    CheckM(MemInAllocatorRange(mem),
           "Allocator \"%s\": This memory block (0x%p) doesn't belong to this allocator [0x%p; 0x%p].",
           m_Name.Data(),
           mem,
           m_First->data,
           cast<byte *>(m_First) + m_Capacity - 1);

    BlockHeader *header = cast<BlockHeader *>(cast<byte *>(mem) - sizeof(BlockHeader));

    CheckM(header->block_state == BLOCK_STATE::ALLOCATED,
           "Allocator \"%s\": This memory block (0x%p) is not allocated yet/already.",
           m_Name.Data(),
           mem);

    if (header->data_bytes == bytes)
    {
        m_CriticalSection.Leave();
        return mem;
    }

    CheckM(bytes < header->data_bytes || bytes - header->data_bytes <= m_Capacity - m_Used,
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
        DebugResultM(new_header = FindBestMatch(bytes),
                     "Allocator \"%s\": Memory overflow.\n"
                     "    Bytes to allocate: %I64u.\n"
                     "    Remain allocator capacity: %I64u.\n"
                     "    Maximum bytes can be allocated: %I64u",
                     m_Name.Data(),
                     bytes,
                     m_Capacity - m_Used,
                     m_Capacity - m_Used - sizeof(BlockHeader));

        CopyMemory(new_header->data, header->data, __min(new_header->data_bytes, header->data_bytes));

        header->block_state = BLOCK_STATE::IN_FREE_LIST;
        ZeroMemory(header->data, header->data_bytes);

        m_Used -= header->data_bytes;

        MergeNearbyBlocksInFreeList(header);
    }

    mem = null;

#if DEBUG
    ++m_ReAllocationsCount;
#endif

    m_CriticalSection.Leave();

    return new_header->data;
}

void *Allocator::AllocateAligned(u64 bytes, u64 alignment)
{
    if (alignment < ENGINE_DEFAULT_ALIGNMENT) alignment = ENGINE_DEFAULT_ALIGNMENT;
    CheckM(IsPowOf2(alignment), "Allocator \"%s\": Alignment must be power of 2", m_Name.Data());
    return Allocate(AlignUp(bytes, alignment));
}

void Allocator::DeAllocateAligned(void *&mem)
{
    DeAllocate(mem);
}

void *Allocator::ReAllocateAligned(void *&mem, u64 bytes, u64 alignment)
{
    if (alignment < ENGINE_DEFAULT_ALIGNMENT) alignment = ENGINE_DEFAULT_ALIGNMENT;
    CheckM(IsPowOf2(alignment), "Allocator \"%s\": Alignment must be power of 2", m_Name.Data());
    return ReAllocate(mem, AlignUp(bytes, alignment));
}

Allocator& Allocator::operator=(Allocator&& other) noexcept
{
    if (this != &other)
    {
        CopyMemory(this, &other, StructFieldOffset(Allocator, m_CriticalSection));
        m_CriticalSection = RTTI::move(other.m_CriticalSection);

        ZeroMemory(&other, StructFieldOffset(Allocator, m_CriticalSection));
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
