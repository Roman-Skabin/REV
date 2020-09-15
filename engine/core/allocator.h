//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

struct BlockHeader;

// @TODO(Roman): Allocator gotta has a name?
class ENGINE_IMPEXP Allocator final
{
public:
    Allocator(
        in_opt void *base_address,
        in     u64   capacity,
        in     b32   clear_memory // @NOTE(Roman): There is a sence to use it only if base_address != null.
    );
    Allocator(in Allocator&& other) noexcept;
    ~Allocator();

    void *Allocate(in u64 bytes);
    void  DeAllocate(in_out_opt void *&mem);
    void *ReAllocate(
        in_out_opt void *&mem,  // @NOTE(Roman): if mem = null then Allocate returns.
        in_opt     u64    bytes // @NOTE(Roman): if bytes = 0 then mem will be deallocated.
    );

    void *AllocateAligned(in u64 bytes, in_opt u64 alignment);
    void  DeAllocateAligned(in_out_opt void *&mem);
    void *ReAllocateAligned(
        in_out_opt void *&mem,      // @NOTE(Roman): if mem = null then AllocateAligned returns.
        in_opt     u64    bytes,    // @NOTE(Roman): if bytes = 0 then mem will be deallocated.
        in_opt     u64    alignment // @NOTE(Roman): if alignment < ENGINE_DEFAULT_ALIGNMENT then alignment = ENGINE_DEFAULT_ALIGNMENT
    );

    template<typename T> constexpr T    *Alloc(in u64 count = 1)                       { return cast<T *>(Allocate(count * sizeof(T)));                              }
    template<typename T> constexpr void  DeAlloc(in_out_opt T *&mem)                   { DeAllocate(cast<void *&>(mem));                                             }
    template<typename T> constexpr T    *ReAlloc(in_out_opt T *&mem, in_opt u64 count) { return cast<T *>(ReAllocate(cast<void *&>(mem), count * sizeof(T))); }
    
    template<typename T> constexpr T    *AllocA(in u64 count = 1, in_opt u64 alignment = 0)                       { return cast<T *>(AllocateAligned(count * sizeof(T), alignment));                              }
    template<typename T> constexpr void  DeAllocA(in_out_opt T *&mem)                                             { DeAllocateAligned(cast<void *&>(mem));                                                        }
    template<typename T> constexpr T    *ReAllocA(in_out_opt T *&mem, in_opt u64 count, in_opt u64 alignment = 0) { return cast<T *>(ReAllocateAligned(cast<void *&>(mem), count * sizeof(T), alignment)); }

    template<typename T> constexpr bool ContainsPointer(in T *ptr) const { return MemInAllocatorRange(this, cast<void *>(ptr)); }

    Allocator& operator=(in Allocator&& other) noexcept;

private:
    BlockHeader *FindBestMatch(in u64 bytes);
    void MergeNearbyBlocksInFreeList(in BlockHeader *header);
    BlockHeader *ReAllocateInplace(in BlockHeader *header, in u64 bytes);

    Allocator(in const Allocator&) = delete;
    Allocator& operator=(in const Allocator&) = delete;

    bool MemInAllocatorRange(in void *mem);
    bool BlockInAllocatorRange(in BlockHeader *block);

private:
    BlockHeader *m_FreeList;
    BlockHeader *m_First;
    BlockHeader *m_LastAllocated;
    u64          m_Used;
    u64          m_Capacity;
#if DEBUG
    u64          m_AllocationsCount;
    u64          m_ReAllocationsCount;
    u64          m_DeAllocationsCount;
#endif
    b32          m_VallocUsed;
};
