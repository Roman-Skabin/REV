// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/common.h"
#include "tools/critical_section.hpp"
#include "tools/const_string.h"

namespace REV
{
    struct BlockHeader;

    class REV_API Allocator final
    {
    public:
        Allocator(
            void              *base_address,
            u64                capacity,
            bool               clear_memory, // @NOTE(Roman): There is a sence to use it only if base_address != null.
            const ConstString& name
        );
        Allocator(Allocator&& other) noexcept;
        ~Allocator();

        void *Allocate(u64 bytes);
        void  DeAllocate(void *&mem);
        void *ReAllocate(
            void *&mem,  // @NOTE(Roman): if mem = null then Allocate returns.
            u64    bytes // @NOTE(Roman): if bytes = 0 then mem will be deallocated.
        );

        void *AllocateAligned(u64 bytes, u64 alignment);
        void  DeAllocateAligned(void *&mem);
        void *ReAllocateAligned(
            void *&mem,      // @NOTE(Roman): if mem = null then AllocateAligned returns.
            u64    bytes,    // @NOTE(Roman): if bytes = 0 then mem will be deallocated.
            u64    alignment // @NOTE(Roman): if alignment < REV::DEFAULT_ALIGNMENT then alignment = REV::DEFAULT_ALIGNMENT
        );

        template<typename T> REV_INLINE T    *Alloc(u64 count = 1)        { return cast(T *, Allocate(count * sizeof(T)));                       }
        template<typename T> REV_INLINE void  DeAlloc(T *&mem)            { DeAllocate(cast(void *&, mem));                                      }
        template<typename T> REV_INLINE T    *ReAlloc(T *&mem, u64 count) { return cast(T *, ReAllocate(cast(void *&, mem), count * sizeof(T))); }
        
        template<typename T> REV_INLINE T    *AllocA(u64 count = 1, u64 alignment = 0)        { return cast(T *, AllocateAligned(count * sizeof(T), alignment));                       }
        template<typename T> REV_INLINE void  DeAllocA(T *&mem)                               { DeAllocateAligned(cast(void *&, mem));                                                 }
        template<typename T> REV_INLINE T    *ReAllocA(T *&mem, u64 count, u64 alignment = 0) { return cast(T *, ReAllocateAligned(cast(void *&, mem), count * sizeof(T), alignment)); }

        template<typename T> REV_INLINE bool ContainsPointer(T *ptr) const { return MemInAllocatorRange(this, cast(void *, ptr)); }

        Allocator& operator=(Allocator&& other) noexcept;

    private:
        BlockHeader *FindBestMatch(u64 bytes);
        void MergeNearbyBlocksInFreeList(BlockHeader *header);
        BlockHeader *ReAllocateInplace(BlockHeader *header, u64 bytes);

        Allocator(const Allocator&) = delete;
        Allocator& operator=(const Allocator&) = delete;

        bool MemInAllocatorRange(void *mem);
        bool BlockInAllocatorRange(BlockHeader *block);

    private:
        BlockHeader            *m_FreeList;
        BlockHeader            *m_First;
        BlockHeader            *m_LastAllocated;
        u64                     m_Used;
        u64                     m_Capacity;
    #if REV_DEBUG
        u64                     m_AllocationsCount;
        u64                     m_ReAllocationsCount;
        u64                     m_DeAllocationsCount;
        u64                     m_MaxMemoryUsed;
    #endif
        CriticalSection<false>  m_CriticalSection;
        ConstString             m_Name;
        bool                    m_VallocUsed;
    };
}
