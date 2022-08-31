// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

namespace REV
{

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
REV_INLINE ChunkList<T, chunk_capacity, doubly_linked_chunks>::ChunkList(Allocator *allocator, u64 initial_capacity)
    : m_Allocator(allocator),
      m_FirstChunk(null),
      m_LastChunk(null),
      m_Count(0),
      m_Capacity(0),
      m_ChunksCount(0)
{
    Init(initial_capacity);
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
template<u64 other_chunk_capacity, bool other_doubly_linked>
REV_INLINE ChunkList<T, chunk_capacity, doubly_linked_chunks>::ChunkList(const ChunkList<T, other_chunk_capacity, other_doubly_linked>& other)
    : m_Allocator(other.m_Allocator),
      m_FirstChunk(null),
      m_LastChunk(null),
      m_Count(0),
      m_Capacity(0),
      m_ChunksCount(0)
{
    Init(other.m_Capacity);
    CopyOther(other);
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
REV_INLINE ChunkList<T, chunk_capacity, doubly_linked_chunks>::ChunkList(ChunkList&& other)
    : m_Allocator(other.m_Allocator),
      m_FirstChunk(other.m_FirstChunk),
      m_LastChunk(other.m_LastChunk),
      m_Count(other.m_Count),
      m_Capacity(other.m_Capacity),
      m_ChunksCount(other.m_ChunksCount)
{
    other.m_Allocator = null;
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
REV_INLINE ChunkList<T, chunk_capacity, doubly_linked_chunks>::~ChunkList()
{
    if (m_Allocator)
    {
        Clear();
        m_Allocator = null;
    }
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
T *ChunkList<T, chunk_capacity, doubly_linked_chunks>::PushBack()
{
    REV_CHECK_M(m_Capacity, "Init first");

    u64 chunk_index = m_Count / chunk_capacity;
    if (chunk_index == m_ChunksCount)
    {
        Chunk *new_chunk = m_Allocator->Alloc<Chunk>();

        if constexpr (doubly_linked_chunks)
        {
            new_chunk->prev = m_LastChunk;
        }

        m_LastChunk->next = new_chunk;
        m_LastChunk       = new_chunk;

        if (!m_FirstChunk)
        {
            m_FirstChunk = m_LastChunk;
        }

        ++m_ChunksCount;
        ++m_Count;

        return m_LastChunk->data;
    }

    REV_CHECK(chunk_index < m_ChunksCount);
    u64 index_in_chunk = m_Count % chunk_capacity;

    ++m_Count;

    return m_LastChunk->data + index_in_chunk;
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
void ChunkList<T, chunk_capacity, doubly_linked_chunks>::Clear()
{
    Chunk *chunk_it = m_FirstChunk;
    while (chunk_it)
    {
        if constexpr (!RTTI::is_trivially_destructible_v<T>)
        {
            // @TODO(Roman): for each till count, not chunk_capacity
            for (T& it : chunk_it->data)
            {
                it.~T();
            }
        }

        Chunk *next_chunk = chunk_it->next;
        m_Allocator->DeAlloc(chunk_it);
        chunk_it = next_chunk;
    }

    m_FirstChunk  = null;
    m_LastChunk   = null;
    m_Count       = 0;
    m_Capacity    = 0;
    m_ChunksCount = 0;
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
REV_INLINE T *ChunkList<T, chunk_capacity, doubly_linked_chunks>::Get(u64 index)
{
    REV_CHECK_M(m_Count, "ChunkList is empty");
    REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);

    u64 chunk_index = index / chunk_capacity;
    u64 elem_index  = index % chunk_capacity;

    Chunk *chunk = m_FirstChunk;
    while (chunk && chunk_index)
    {
        --chunk_index;
        chunk = chunk->next;
    }

    return chunk ? chunk->data + elem_index : null;
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
REV_INLINE const T *ChunkList<T, chunk_capacity, doubly_linked_chunks>::Get(u64 index) const
{
    REV_CHECK_M(m_Count, "ChunkList is empty");
    REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);

    u64 chunk_index = index / chunk_capacity;
    u64 elem_index  = index % chunk_capacity;

    const Chunk *chunk = m_FirstChunk;
    while (chunk && chunk_index)
    {
        --chunk_index;
        chunk = chunk->next;
    }

    return chunk ? chunk->data + elem_index : null;
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
void ChunkList<T, chunk_capacity, doubly_linked_chunks>::ForEach(Function<ForEachCallback>&& callback)
{
    u64 chunk_index       = 0;
    u64 elem_global_index = 0;

    for (Chunk *chunk = m_FirstChunk; chunk; chunk = chunk->next)
    {
        u64 elem_count_in_chunk = Math::min(m_Count - elem_global_index, chunk_capacity);

        T *it_end = chunk->data + elem_count_in_chunk;
        for (T *it = chunk->data; it < it_end; ++it)
        {
            u64 elem_local_index = elem_global_index % chunk_capacity;

            callback(*it, chunk_index, elem_global_index, elem_local_index);

            ++elem_global_index;
        }

        ++chunk_index;
    }

    REV_CHECK(chunk_index       == m_ChunksCount);
    REV_CHECK(elem_global_index == m_Count);
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
void ChunkList<T, chunk_capacity, doubly_linked_chunks>::ForEach(Function<ForEachConstCallback>&& callback) const
{
    u64 chunk_index       = 0;
    u64 elem_global_index = 0;

    for (const Chunk *chunk = m_FirstChunk; chunk; chunk = chunk->next)
    {
        u64 elem_count_in_chunk = Math::min(m_Count - elem_global_index, chunk_capacity);

        const T *it_end = chunk->data + elem_count_in_chunk;
        for (const T *it = chunk->data; it < it_end; ++it)
        {
            u64 elem_local_index = elem_global_index % chunk_capacity;

            callback(*it, chunk_index, elem_global_index, elem_local_index);

            ++elem_global_index;
        }

        ++chunk_index;
    }

    REV_CHECK(chunk_index       == m_ChunksCount);
    REV_CHECK(elem_global_index == m_Count);
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
REV_INLINE ChunkList<T, chunk_capacity, doubly_linked_chunks>& ChunkList<T, chunk_capacity, doubly_linked_chunks>::operator=(nullptr_t)
{
    Clear();
    return *this;
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
template<u64 other_chunk_capacity, bool other_doubly_linked>
REV_INLINE ChunkList<T, chunk_capacity, doubly_linked_chunks>& ChunkList<T, chunk_capacity, doubly_linked_chunks>::operator=(const ChunkList<T, other_chunk_capacity, other_doubly_linked>& other)
{
    if (this != &other)
    {
        Clear();
        Init(other.m_Capacity);
        CopyOther(other);
    }
    return *this;
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
REV_INLINE ChunkList<T, chunk_capacity, doubly_linked_chunks>& ChunkList<T, chunk_capacity, doubly_linked_chunks>::operator=(ChunkList&& other)
{
    if (this != &other)
    {
        m_Allocator   = other.m_Allocator;
        m_FirstChunk  = other.m_FirstChunk;
        m_LastChunk   = other.m_LastChunk;
        m_Count       = other.m_Count;
        m_Capacity    = other.m_Capacity;
        m_ChunksCount = other.m_ChunksCount;

        other.m_Allocator = null;
    }
    return *this;
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
void ChunkList<T, chunk_capacity, doubly_linked_chunks>::Init(u64 initial_capacity)
{
    u64 chunk_index    = initial_capacity / chunk_capacity;
    u64 index_in_chunk = initial_capacity % chunk_capacity;

    m_ChunksCount = chunk_index + (index_in_chunk ? 1 : 0);
    if (m_ChunksCount)
    {
        m_FirstChunk = m_Allocator->Alloc<Chunk>();
        m_LastChunk  = m_FirstChunk;

        for (u64 i = 1; i < m_ChunksCount; ++i)
        {
            Chunk *new_chunk = m_Allocator->Alloc<Chunk>();

            if constexpr (doubly_linked_chunks)
            {
                new_chunk->prev = m_LastChunk;
            }

            m_LastChunk->next = new_chunk;
            m_LastChunk       = new_chunk;
        }

        m_Capacity = m_ChunksCount * chunk_capacity;
    }
}

template<typename T, u64 chunk_capacity, bool doubly_linked_chunks>
template<u64 other_chunk_capacity, bool other_doubly_linked>
void ChunkList<T, chunk_capacity, doubly_linked_chunks>::CopyOther(const ChunkList<T, other_chunk_capacity, other_doubly_linked>& other)
{
    REV_CHECK_M(m_Capacity, "Init first");
    static_assert(RTTI::is_copy_assignable_v<T>, "T is not copyable");

    using OtherChunkList = ChunkList<T, other_chunk_capacity, other_doubly_linked>;

    Chunk *chunk_it    = m_FirstChunk;
    u64    data_offset = 0;

    const OtherChunkList::Chunk *other_chunk_it    = other.m_FirstChunk;
    u64                          other_data_offset = 0;

    if constexpr (RTTI::is_trivially_copy_assignable_v<T>)
    {
        while (chunk_it && other_chunk_it)
        {
            u64 data_count_rest = chunk_capacity - data_offset;

            u64 other_data_count      = other_chunk_it == other.m_LastChunk ? other.m_Count % other_chunk_capacity : other_chunk_capacity;
            u64 other_data_count_rest = other_data_count - other_data_offset;

            u64 data_count_to_copy = Math::min(data_count_rest, other_data_count_rest);

            CopyMemory(chunk_it->data       + data_offset,
                    other_chunk_it->data + other_data_offset,
                    data_count_to_copy   * sizeof(T));

            m_Count += data_count_to_copy;

            if (data_count_rest == other_data_count_rest)
            {
                chunk_it    = chunk_it->next;
                data_offset = 0;

                other_chunk_it    = other_chunk_it->next;
                other_data_offset = 0;
            }
            if (data_count_rest < other_data_count_rest)
            {
                chunk_it    = chunk_it->next;
                data_offset = 0;

                other_data_offset += data_count_to_copy;
            }
            else
            {
                REV_CHECK(data_count_rest > other_data_count_rest);

                data_offset += data_count_to_copy;

                other_chunk_it    = other_chunk_it->next;
                other_data_offset = 0;
            }
        }
    }
    else
    {
        // @NOTE(Roman): Copy one-by-one because we are not trivially copyable and we have to call T::operator=(const T&)
        while (chunk_it && other_chunk_it)
        {
            chunk_it->data[data_offset++] = other_chunk_it->data[other_data_offset++];

            ++m_Count;

            if (data_offset >= chunk_capacity)
            {
                chunk_it    = chunk_it->next;
                data_offset = 0;
            }

            u64 other_data_count = other_chunk_it == other.m_LastChunk ? other.m_Count % other_chunk_capacity : other_chunk_capacity;
            if (other_data_offset >= other_data_count)
            {
                other_chunk_it    = other_chunk_it->next;
                other_data_offset = 0;
            }
        }
    }

    REV_CHECK(m_Count == other.m_Count);
}

}
