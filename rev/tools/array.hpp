//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "core/allocator.h"
#include "tools/const_array.hpp"

namespace REV
{
    template<typename T>
    class Array final
    {
    public:
        using Type = T;
        static constexpr const u64 npos = REV_U64_MAX;
    
        explicit Array(Allocator *allocator, u64 initial_capacity = 16, u64 alignment_in_bytes = REV::DEFAULT_ALIGNMENT)
            : m_Header(null)
        {
            REV_CHECK(allocator);
            if (alignment_in_bytes < REV::DEFAULT_ALIGNMENT) alignment_in_bytes = REV::DEFAULT_ALIGNMENT;
    
            m_Header                     = cast(Header *, allocator->AllocateAligned(sizeof(Header) + initial_capacity * sizeof(T), alignment_in_bytes));
            m_Header->allocator          = allocator;
            m_Header->count              = 0;
            m_Header->capacity           = initial_capacity;
            m_Header->alignment_in_bytes = alignment_in_bytes;
        }

        Array(const ConstArray<T>& const_array, Allocator *allocator, u64 alignment_in_bytes = REV::DEFAULT_ALIGNMENT)
            : m_Header(null)
        {
            REV_CHECK(allocator);
            if (alignment_in_bytes < REV::DEFAULT_ALIGNMENT) alignment_in_bytes = REV::DEFAULT_ALIGNMENT;

            u64 capacity = 2 * const_array.Count();

            m_Header = cast(Header *, allocator->AllocateAligned(sizeof(Header) + capacity * sizeof(T), alignment_in_bytes));
            m_Header->allocator          = allocator;
            m_Header->count              = const_array.Count();
            m_Header->capacity           = capacity;
            m_Header->alignment_in_bytes = alignment_in_bytes;

            CopyMemory(m_Header->data, const_array.Data(), m_Header->count * sizeof(T));
        }
    
        Array(const Array& other)
            : m_Header(null)
        {
            m_Header = cast(Header *, other.m_Header->allocator->AllocateAligned(sizeof(Header) + other.m_Header->capacity * sizeof(T),
                                                                                 other.m_Header->alignment_in_bytes));
            CopyMemory(m_Header, other.m_Header, sizeof(Header) + other.m_Header->count * sizeof(T));
        }
    
        REV_INLINE Array(Array&& other) noexcept
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
    
        REV_INLINE T *PushFront(u64 count = 1) { return Insert(0,               count); }
        REV_INLINE T *PushBack(u64 count  = 1) { return Insert(m_Header->count, count); }
    
        void Erase(u64 from, u64 to = npos)
        {
            if (to == npos) to = from + 1;
            REV_CHECK_M(from < m_Header->count && from < to && to <= m_Header->count, "Bad arguments: from = %I64u, to = %I64u, count = %I64u", from, to, m_Header->count);

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
                MoveMemory(m_Header->data   + from,
                           m_Header->data   + to,
                           (m_Header->count - to) * sizeof(T));
            }

            u64 delta = to - from;
            ZeroMemory(m_Header->data + m_Header->count - delta, delta * sizeof(T));

            m_Header->count -= delta;
            Fit();
        }

        REV_INLINE void EraseFront() { Erase(0); }
        REV_INLINE void EraseBack()  { Erase(m_Header->count - 1); }

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
                m_Header = cast(Header *, m_Header->allocator->ReAllocateAligned(cast(void *&, m_Header),
                                                                                 sizeof(Header) + capacity * sizeof(T),
                                                                                 m_Header->alignment_in_bytes));
                m_Header->capacity = capacity;
            }
        }
    
        void Reverse()
        {
            T *_first = m_Header->data;
            T *_last  = pLast();
    
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
            T *last  = pLast();
    
            for (T *it = last; it > first; --it)
            {
                if (*it == what)
                {
                    return it - first;
                }
            }
    
            return *first == what ? 0 : npos;
        }
    
        REV_INLINE u64 Count()     const { return m_Header->count;             }
        REV_INLINE u64 Capacity()  const { return m_Header->capacity;          }
        REV_INLINE u64 Alignment() const { return m_Header->alinment_in_bytes; }
    
        REV_INLINE const T *Data() const { return m_Header->data; }
        REV_INLINE       T *Data()       { return m_Header->data; }
    
        REV_INLINE bool Empty() const { return !m_Header->count; }
    
        REV_INLINE const T *begin()   const { return m_Header->data;                   }
        REV_INLINE const T *cbegin()  const { return m_Header->data;                   }
        REV_INLINE const T *rbegin()  const { return m_Header->data + m_Header->count; }
        REV_INLINE const T *crbegin() const { return m_Header->data + m_Header->count; }
    
        REV_INLINE const T *end()   const { return m_Header->data + m_Header->count; }
        REV_INLINE const T *cend()  const { return m_Header->data + m_Header->count; }
        REV_INLINE const T *rend()  const { return m_Header->data;                   }
        REV_INLINE const T *crend() const { return m_Header->data;                   }
    
        REV_INLINE T *begin()  { return m_Header->data;                   }
        REV_INLINE T *rbegin() { return m_Header->data + m_Header->count; }
    
        REV_INLINE T *end()  { return m_Header->data + m_Header->count; }
        REV_INLINE T *rend() { return m_Header->data;                   }
    
        REV_INLINE const T& First() const { return *m_Header->data; }
        REV_INLINE const T& Last()  const { return m_Header->count ? m_Header->data[m_Header->count - 1] : *m_Header->data; }
    
        REV_INLINE T& First() { return *m_Header->data; }
        REV_INLINE T& Last()  { return m_Header->count ? m_Header->data[m_Header->count - 1] : *m_Header->data; }
    
        REV_INLINE const T *pFirst() const { return m_Header->data; }
        REV_INLINE const T *pLast()  const { return m_Header->count ? m_Header->data + m_Header->count - 1 : m_Header->data; }
    
        REV_INLINE T *pFirst() { return m_Header->data; }
        REV_INLINE T *pLast()  { return m_Header->count ? m_Header->data + m_Header->count - 1 : m_Header->data; }
    
        REV_INLINE const T *GetPointer(u64 index) const
        {
            REV_CHECK_M(m_Header->count, "Array is empty");
            REV_CHECK_M(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
            return m_Header->data + index;
        }
    
        REV_INLINE T *GetPointer(u64 index)
        {
            REV_CHECK_M(m_Header->count, "Array is empty");
            REV_CHECK_M(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
            return m_Header->data + index;
        }

        Array& operator=(const ConstArray<T>& const_array)
        {
            u64 new_count = const_array.Count();

            Clear();
            if (new_count > m_Header->capacity)
            {
                m_Header->capacity = 2 * new_count;

                m_Header = cast(Header *, m_Header->allocator->ReAllocateAligned(cast(void *&, m_Header),
                                                                                 sizeof(Header) + m_Header->capacity * sizeof(T),
                                                                                 m_Header->alignment_in_bytes));
            }

            CopyMemory(m_Header->data, const_array.Data(), new_count * sizeof(T));

            m_Header->count = new_count;
            return *this;
        }
    
        Array& operator=(const Array& other)
        {
            if (m_Header != other.m_Header)
            {
                DestroyAll();
                m_Header = cast(Header *, other.m_Header->allocator->ReAllocateAligned(cast(void *&, m_Header),
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
    
        REV_INLINE const T& operator[](u64 index) const
        {
            REV_CHECK_M(m_Header->count, "Array is empty");
            REV_CHECK_M(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
            return m_Header->data[index];
        }
    
        REV_INLINE T& operator[](u64 index)
        {
            REV_CHECK_M(m_Header->count, "Array is empty");
            REV_CHECK_M(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
            return m_Header->data[index];
        }

        REV_INLINE const T *operator+(u64 index) const
        {
            REV_CHECK_M(m_Header->count, "Array is empty");
            REV_CHECK_M(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
            return m_Header->data + index;
        }

        REV_INLINE T *operator+(u64 index)
        {
            REV_CHECK_M(m_Header->count, "Array is empty");
            REV_CHECK_M(index < m_Header->count, "Expected max index: %I64u, got: %I64u", m_Header->count - 1, index);
            return m_Header->data + index;
        }
    
    private:
        void Expand()
        {
            if (m_Header->count > m_Header->capacity)
            {
                m_Header->capacity *= 2;
                m_Header            = cast(Header *, m_Header->allocator->ReAllocateAligned(cast(void *&, m_Header),
                                                                                            sizeof(Header) + m_Header->capacity * sizeof(T),
                                                                                            m_Header->alignment_in_bytes));
            }
        }
    
        void Fit()
        {
            if (2 * m_Header->count < m_Header->capacity)
            {
                m_Header->capacity = 2 * m_Header->count;
                m_Header           = cast(Header *, m_Header->allocator->ReAllocateAligned(cast(void *&, m_Header),
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
            Allocator *allocator;
            u64        count;
            u64        capacity;
            u64        alignment_in_bytes;
            T          data[0];
        };
    
        Header *m_Header;
    };
}
