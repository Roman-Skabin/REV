//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/allocator.h"

template<typename T, typename Allocator_t = Allocator>
class Array final
{
public:
    using Type = T;
    static constexpr const u64 npos = U64_MAX;

    explicit Array(Allocator_t *allocator, u64 initial_capacity = 16, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + initial_capacity * sizeof(T), alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->count              = 0;
        m_Header->capacity           = initial_capacity;
        m_Header->alignment_in_bytes = alignment_in_bytes;
    }

    Array(const Array& other)
        : m_Header(null)
    {
        m_Header = cast<Header *>(other.m_Header->allocator->AllocateAligned(sizeof(Header) + other.m_Header->capacity * sizeof(T),
                                                                             other.m_Header->alignment_in_bytes));
        CopyMemory(m_Header, other.m_Header, sizeof(Header) + other.m_Header->count * sizeof(T));
    }

    Array(Array&& other) noexcept
        : m_Header(other.m_Header)
    {
        other.m_Header = null;
    }

    ~Array()
    {
        if (m_Header)
        {
            Clear();
            m_Header->allocator->DeAllocA(m_Header);
        }
    }

    T *Insert(u64 where, u64 count = 1)
    {
        u64 old_count = m_Header->count;

        m_Header->count += count;
        Expand();

        if (where != old_count)
        {
            MoveMemory(m_Header->data + where + count,
                       m_Header->data + where,
                       (old_count - where) * sizeof(T));
        }

        return m_Header->data + where;
    }

    constexpr T *PushFront(u64 count = 1) { return Insert(0,               count); }
    constexpr T *PushBack(u64 count  = 1) { return Insert(m_Header->count, count); }

    void Erase(u64 from, u64 to = npos)
    {
        if (to == npos) to = from + 1;
        CheckM(from < m_Header->count && from < to && to <= m_Header->count, "Bad arguments: from = %I64u, to = %I64u, count = %I64u", from, to, m_Header->count);

        // @NOTE(Roman): Ugh, we gotta do that because of destructors.
        if constexpr (RTTI::is_destructible_v<T>)
        {
            for (T *it = m_Header->data + from; it < m_Header->data + to; ++it)
            {
                it->~T();
            }
        }

        if (from < m_Header->count - 1)
        {
            MoveMemory(m_Header->data + from,
                       m_Header->data + to,
                       (to - from) * sizeof(T));
        }

        ZeroMemory(m_Header->data + m_Header->count - to + from, (to - from) * sizeof(T));

        m_Header->count -= to - from;
        Fit();
    }

    void Clear()
    {
        DestroyAll();
        ZeroMemory(m_Header->data, m_Header->count * sizeof(T));

        m_Header->count = 0;
    }

    void Reserve(u64 capacity)
    {
        if (m_Header->count <= capacity && m_Header->capacity != capacity)
        {
            m_Header = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                             sizeof(Header) + capacity * sizeof(T),
                                                                             m_Header->alignment_in_bytes));
            m_Header->capacity = capacity;
        }
    }

    void Reverse()
    {
        T *_first = m_Header->data;
        T *_last  = m_Header->data + m_Header->count - 1;

        while (_first < _last)
        {
            T temp    = RTTI::move(*_first);
            *_first++ = RTTI::move(*_last);
            *_last--  = RTTI::move(temp);
        }
    }

    u64 Find(const T& what) const
    {
        T *_begin = m_Header->data;
        T *_end   = m_Header->data + m_Header->count;
        
        for (T *it = _begin; it < _end; ++it)
        {
            if (*it == what)
            {
                return it - _begin;
            }
        }

        return npos;
    }

    u64 RFind(const T& what) const
    {
        T *first = m_Header->data;
        T *last  = m_Header->data + m_Header->count - 1;

        for (T *it = last; it > first; --it)
        {
            if (*it == what)
            {
                return it - first;
            }
        }

        return *first == what ? 0 : npos;
    }

    constexpr u64 Count()     const { return m_Header->count;             }
    constexpr u64 Capacity()  const { return m_Header->capacity;          }
    constexpr u64 Alignment() const { return m_Header->alinment_in_bytes; }

    constexpr const T *Data() const { return m_Header->data; }
    constexpr       T *Data()       { return m_Header->data; }

    constexpr bool Empty() const { return !m_Header->count; }

    constexpr const T *begin()   const { return m_Header->data;                   }
    constexpr const T *cbegin()  const { return m_Header->data;                   }
    constexpr const T *rbegin()  const { return m_Header->data + m_Header->count; }
    constexpr const T *crbegin() const { return m_Header->data + m_Header->count; }

    constexpr const T *end()   const { return m_Header->data + m_Header->count; }
    constexpr const T *cend()  const { return m_Header->data + m_Header->count; }
    constexpr const T *rend()  const { return m_Header->data;                   }
    constexpr const T *crend() const { return m_Header->data;                   }

    constexpr T *begin()  { return m_Header->data;                   }
    constexpr T *rbegin() { return m_Header->data + m_Header->count; }

    constexpr T *end()  { return m_Header->data + m_Header->count; }
    constexpr T *rend() { return m_Header->data;                   }

    constexpr const T& First() const { return *m_Header->data; }
    constexpr const T& Last()  const { return m_Header->count ? m_Header->data[m_Header->count - 1] : *m_Header->data; }

    constexpr T& First() { return *m_Header->data; }
    constexpr T& Last()  { return m_Header->count ? m_Header->data[m_Header->count - 1] : *m_Header->data; }

    constexpr const T *pFirst() const { return m_Header->data; }
    constexpr const T *pLast()  const { return m_Header->count ? m_Header->data + m_Header->count - 1 : m_Header->data; }

    constexpr T *pFirst() { return m_Header->data; }
    constexpr T *pLast()  { return m_Header->count ? m_Header->data + m_Header->count - 1 : m_Header->data; }

    constexpr const T *GetPointer(u64 index) const
    {
        CheckM(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
        return m_Header->data + index;
    }

    constexpr T *GetPointer(u64 index)
    {
        CheckM(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
        return m_Header->data + index;
    }

    Array& operator=(const Array& other)
    {
        if (m_Header != other.m_Header)
        {
            DestroyAll();
            m_Header = cast<Header *>(other.m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                                   sizeof(Header) + other.m_Header->capacity * sizeof(T),
                                                                                   other.m_Header->alignment_in_bytes));
            CopyMemory(m_Header, other.m_Header, sizeof(Header) + other.m_Header->count * sizeof(T));
        }
        return *this;
    }

    Array& operator=(Array&& other) noexcept
    {
        if (m_Header != other.m_Header)
        {
            DestroyAll();
            if (m_Header) m_Header->allocator->DeAllocA(m_Header);
            m_Header       = other.m_Header;
            other.m_Header = null;
        }
        return *this;
    }

    const T& operator[](u64 index) const
    {
        CheckM(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
        return m_Header->data[index];
    }

    T& operator[](u64 index)
    {
        CheckM(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
        return m_Header->data[index];
    }

private:
    void Expand()
    {
        if (m_Header->count > m_Header->capacity)
        {
            m_Header->capacity *= 2;
            m_Header            = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                                        sizeof(Header) + m_Header->capacity * sizeof(T),
                                                                                        m_Header->alignment_in_bytes));
        }
    }

    void Fit()
    {
        if (2 * m_Header->count < m_Header->capacity)
        {
            m_Header->capacity = 2 * m_Header->count;
            m_Header           = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                                       sizeof(Header) + m_Header->capacity * sizeof(T),
                                                                                       m_Header->alignment_in_bytes));
        }
    }

    void DestroyAll()
    {
        if constexpr (RTTI::is_destructible_v<T>)
        {
            if (m_Header)
            {
                for (T *it = m_Header->data; it < m_Header->data + m_Header->count; ++it)
                {
                    it->~T();
                }
            }
        }
    }

private:
    struct Header
    {
        Allocator_t *allocator;
        u64          count;
        u64          capacity;
        u64          alignment_in_bytes;
        #pragma warning(suppress: 4200)
        T            data[0];
    };

    Header *m_Header;
};
