//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/allocator.h"

template<typename T, typename Allocator_t = Allocator>
class Buffer final
{
public:
    using Type = T;
    static constexpr u64 npos = U64_MAX;

    template<typename ...U>
    using EnableConditionCA = RTTI::conditional_t<RTTI::is_pointer_v<T>,
                                                  RTTI::enable_if_t<RTTI::are_base_of_v<RTTI::remove_pointer_t<T>, RTTI::remove_pointer_t<U>...>>,
                                                  RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_copy_assignable_v<T>>>;
    template<typename ...U>
    using EnableConditionMA = RTTI::conditional_t<RTTI::is_pointer_v<T>,
                                                  RTTI::enable_if_t<RTTI::are_base_of_v<RTTI::remove_pointer_t<T>, RTTI::remove_pointer_t<U>...>>,
                                                  RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_move_assignable_v<T>>>;
    using EnableCA = RTTI::conditional_t<RTTI::is_pointer_v<T>, void, RTTI::enable_if_t<RTTI::is_copy_assignable_v<T>>>;
    using EnableMA = RTTI::conditional_t<RTTI::is_pointer_v<T>, void, RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>>;

    explicit Buffer(Allocator_t *allocator, u64 initial_capacity = 16, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
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

    Buffer(const Buffer& other)
        : m_Header(null)
    {
        m_Header = cast<Header *>(other.m_Header->allocator->AllocateAligned(sizeof(Header) + other.m_Header->capacity * sizeof(T),
                                                                             other.m_Header->alignment_in_bytes));
        CopyMemory(m_Header, other.m_Header, sizeof(Header) + other.m_Header->count * sizeof(T));
    }

    Buffer(Buffer&& other) noexcept
        : m_Header(other.m_Header)
    {
        other.m_Header = null;
    }

    ~Buffer()
    {
        if (m_Header)
        {
            Clear();
            m_Header->allocator->DeAllocA(m_Header);
        }
    }

    template<typename ...U, typename = EnableConditionMA<U...>>
    void Insert(u64 where, U&&... elements)
    {
        u64 old_count = m_Header->count;

        m_Header->count += sizeof...(elements);
        Expand();

        if (where != old_count)
        {
            MoveMemory(m_Header->data + where + sizeof...(elements),
                       m_Header->data + where,
                       (old_count - where) * sizeof(T));
        }

        (..., (m_Header->data[where++] = RTTI::move(elements)));
    }

    template<typename ...U, typename = EnableConditionCA<U...>>
    void Insert(u64 where, const U&... elements)
    {
        u64 old_count = m_Header->count;

        m_Header->count += sizeof...(elements);
        Expand();

        if (where != old_count)
        {
            MoveMemory(m_Header->data + where + sizeof...(elements),
                       m_Header->data + where,
                       (old_count - where) * sizeof(T));
        }

        (..., (m_Header->data[where++] = elements));
    }

    template<u64 count, typename = EnableCA>
    void Insert(u64 where, const T (&elements)[count])
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

        for (u64 i = 0; i < count; ++i)
        {
            m_Header->data[where + i] = elements[i];
        }
    }

    template<typename = EnableCA>
    void Insert(u64 where, T *const elements, u64 count)
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

        for (u64 i = 0; i < count; ++i)
        {
            m_Header->data[where + i] = elements[i];
        }
    }

    template<typename ...U, typename = EnableConditionCA<U...>> constexpr void PushFront(const U&... elements) { Insert(0,               elements...); }
    template<typename ...U, typename = EnableConditionCA<U...>> constexpr void PushBack(const U&... elements)  { Insert(m_Header->count, elements...); }

    template<typename ...U, typename = EnableConditionMA<U...>> constexpr void PushFront(U&&... elements) { Insert(0,               RTTI::forward<U>(elements)...); }
    template<typename ...U, typename = EnableConditionMA<U...>> constexpr void PushBack(U&&... elements)  { Insert(m_Header->count, RTTI::forward<U>(elements)...); }

    template<u64 count, typename = EnableCA> constexpr void PushFront(const T (&elements)[count]) { Insert(0,               elements); }
    template<u64 count, typename = EnableCA> constexpr void PushBack(const T (&elements)[count])  { Insert(m_Header->count, elements); }

    template<typename = EnableCA> constexpr void PushFront(T *const elements, u64 count) { Insert(0,               elements, count); }
    template<typename = EnableCA> constexpr void PushBack(T *const elements, u64 count)  { Insert(m_Header->count, elements, count); }

    template<typename ...ConstructorArgs, typename = EnableMA>
    void Emplace(u64 where, ConstructorArgs&&... args)
    {
        u64 old_count = m_Header->count;

        ++m_Header->count;
        Expand();

        if (where != old_count)
        {
            MoveMemory(m_Header->data + where + 1,
                       m_Header->data + where,
                       (old_count - where) * sizeof(T));
        }

        return m_Header->data[where] = T(RTTI::forward<ConstructorArgs>(args)...);
    }

    template<typename ...ConstructorArgs, typename = EnableMA> constexpr void EmplaceBack(ConstructorArgs&&... args)  { return Emplace(m_Header->count, RTTI::forward<ConstructorArgs>(args)...); }
    template<typename ...ConstructorArgs, typename = EnableMA> constexpr void EmplaceFront(ConstructorArgs&&... args) { return Emplace(0,               RTTI::forward<ConstructorArgs>(args)...); }

    void Erase(u64 from, u64 to = npos)
    {
        if (to == npos) to = from + 1;
        CheckM(from < m_Header->count && from < to && to <= m_Header->count, "Bad arguments: from = %I64u, to = %I64u, count = %I64u", from, to, m_Header->count);

        // @NOTE(Roman): Duh, we gotta do that because of destructors.
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

    constexpr const T& First() const { return *m_Header->data;                     }
    constexpr const T& Last()  const { return m_Header->data[m_Header->count - 1]; }

    constexpr T& First() { return *m_Header->data;                     }
    constexpr T& Last()  { return m_Header->data[m_Header->count - 1]; }

    Buffer& operator=(const Buffer& other)
    {
        if (m_Header != other->m_Header)
        {
            DestroyAll();
            m_Header = cast<Header *>(other.m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                                   sizeof(Header) + other.m_Header->capacity * sizeof(T),
                                                                                   other.m_Header->alignment_in_bytes));
            CopyMemory(m_Header, other.m_Header, sizeof(Header) + other.m_Header->count * sizeof(T));
        }
        return *this;
    }

    Buffer& operator=(Buffer&& other) noexcept
    {
        if (m_Header != other->m_Header)
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
            for (T *it = m_Header->data; it < m_Header->data + m_Header->count; ++it)
            {
                it->~T();
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
