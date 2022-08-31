// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/allocator.h"
#include "tools/function.hpp"

namespace REV
{
    template<typename T, u64 capacity>
    struct SLinkedChunkListChunk
    {
        using Type = T;

        static_assert(capacity > 0);

        SLinkedChunkListChunk *next = null;
        T                      data[capacity];
    };

    template<typename T, u64 capacity>
    struct DLinkedChunkListChunk
    {
        using Type = T;

        static_assert(capacity > 0);

        DLinkedChunkListChunk *next = null;
        DLinkedChunkListChunk *prev = null;
        T                      data[capacity];
    };

    template<typename T, u64 chunk_capacity = 16, bool doubly_linked_chunks = false>
    class ChunkList
    {
    public:
        using Type                 = T;
        using Chunk                = RTTI::conditional_t<doubly_linked_chunks, DLinkedChunkListChunk<T, chunk_capacity>, SLinkedChunkListChunk<T, chunk_capacity>>;
        using ForEachCallback      = void(T& elem, u64 chunk_index, u64 elem_index);
        using ForEachConstCallback = void(const T& elem, u64 chunk_index, u64 elem_index);

        ChunkList(Allocator *allocator, u64 initial_capacity = chunk_capacity);
        template<u64 other_chunk_capacity, bool other_doubly_linked>
        ChunkList(const ChunkList<T, other_chunk_capacity, other_doubly_linked>& other);
        ChunkList(ChunkList&& other);

        ~ChunkList();

        T *PushBack();

        void Clear();

              T *Get(u64 index);
        const T *Get(u64 index) const;

        void ForEach(Function<ForEachCallback>&& callback);
        void ForEach(Function<ForEachConstCallback>&& callback) const;

        u64 Count() const;
        u64 Capacity() const;
        u64 ChunksCount() const;

        ChunkList& operator=(nullptr_t);
        template<u64 other_chunk_capacity, bool other_doubly_linked>
        ChunkList& operator=(const ChunkList<T, other_chunk_capacity, other_doubly_linked>& other);
        ChunkList& operator=(ChunkList&& other);

    private:
        void Init(u64 initial_capacity);

        template<u64 other_chunk_capacity, bool other_doubly_linked>
        void CopyOther(const ChunkList<T, other_chunk_capacity, other_doubly_linked>& other);

    private:
        Allocator *m_Allocator;
        Chunk     *m_FirstChunk;
        Chunk     *m_LastChunk;
        u64        m_Count;
        u64        m_Capacity;
        u64        m_ChunksCount;

        template<typename, u64, bool> friend class ChunkList;
    };

    template<typename T, u64 chunk_capacity, bool doubly_linked_chunks> ChunkList(const ChunkList<T, chunk_capacity, doubly_linked_chunks>&) -> ChunkList<T, chunk_capacity, doubly_linked_chunks>;
    template<typename T, u64 chunk_capacity, bool doubly_linked_chunks> ChunkList(ChunkList<T, chunk_capacity, doubly_linked_chunks>&&)      -> ChunkList<T, chunk_capacity, doubly_linked_chunks>;
}

#include "tools/chunk_list.ipp"
