//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"
#include "core/allocator.h"
#include "tools/const_string.h"
#include "tools/static_string.hpp"

template<typename Allocator_t = Allocator>
class String final
{
public:
    static constexpr u64 npos = U64_MAX;

    explicit String(Allocator_t *allocator, u64 initial_capacity = 16, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + initial_capacity, alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = 0;
        m_Header->capacity           = initial_capacity;
        m_Header->alignment_in_bytes = alignment_in_bytes;
    }

    String(Allocator_t *allocator, char symbol, u64 count, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * count, alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = count;
        m_Header->capacity           = 2 * count;
        m_Header->alignment_in_bytes = alignment_in_bytes;

        memset_char(m_Header->data, symbol, count);
    }

    String(Allocator_t *allocator, const char *cstring, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;
        u64 length = strlen(cstring);

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * length, alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = length;
        m_Header->capacity           = 2 * length;
        m_Header->alignment_in_bytes = alignment_in_bytes;

        CopyMemory(m_Header->data, cstring, length);
    }

    String(Allocator_t *allocator, const char *cstring, u64 length, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * length, alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = length;
        m_Header->capacity           = 2 * length;
        m_Header->alignment_in_bytes = alignment_in_bytes;

        CopyMemory(m_Header->data, cstring, length);
    }

    String(Allocator_t *allocator, const ConstString& const_string, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * const_string.Length(), alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = const_string.Length();
        m_Header->capacity           = 2 * const_string.Length();
        m_Header->alignment_in_bytes = alignment_in_bytes;

        CopyMemory(m_Header->data, const_string.Data(), const_string.Length());
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    String(Allocator_t *allocator, const StaticString<ss_capacity, ss_aligned_capacity>& static_string, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + ss_capacity, alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = static_string.Length();
        m_Header->capacity           = ss_capacity;
        m_Header->alignment_in_bytes = alignment_in_bytes;

        CopyMemory(m_Header->data, static_string.Data(), static_string.Length());
    }

    String(const String& other)
    {
        m_Header = cast<Header *>(other.m_Header->allocator->AllocateAligned(sizeof(Header) + other.m_Header->capacity,
                                                                             other.m_Header->alignment_in_bytes));
        CopyMemory(m_Header, other.m_Header, sizeof(Header) + other.m_Header->length);
    }

    String(String&& other) noexcept
        : m_Header(other.m_Header)
    {
        other.m_Header = null;
    }

    ~String()
    {
        if (m_Header)
        {
            m_Header->allocator->DeAllocA(m_Header);
        }
    }

    constexpr u64 Length()    const { return m_Header->length;             }
    constexpr u64 Capacity()  const { return m_Header->capacity;           }
    constexpr u64 Alignment() const { return m_Header->alignment_in_bytes; }

    constexpr const char *Data() const { return m_Header->data; }
    constexpr       char *Data()       { return m_Header->data; }

    constexpr const String& ToString() const { return *this; }
    constexpr       String& ToString()       { return *this; }

    constexpr bool Empty() const { return !m_Header->length; }

    constexpr const Allocator_t *GetAllocator() const { return m_Header->allocator; }

    constexpr const char *begin()   const { return m_Header->data;                    }
    constexpr const char *cbegin()  const { return m_Header->data;                    }
    constexpr const char *rbegin()  const { return m_Header->data + m_Header->length; }
    constexpr const char *crbegin() const { return m_Header->data + m_Header->length; }

    constexpr const char *end()   const { return m_Header->data + m_Header->length; }
    constexpr const char *cend()  const { return m_Header->data + m_Header->length; }
    constexpr const char *rend()  const { return m_Header->data;                    }
    constexpr const char *crend() const { return m_Header->data;                    }

    constexpr char *begin()  { return m_Header->data;                    }
    constexpr char *rbegin() { return m_Header->data + m_Header->length; }

    constexpr char *end()  { return m_Header->data + m_Header->length; }
    constexpr char *rend() { return m_Header->data;                    }

    constexpr char First() const { return *m_Header->data;                      }
    constexpr char Last()  const { return m_Header->data[m_Header->length - 1]; }

    constexpr char& First() { return *m_Header->data;                      }
    constexpr char& Last()  { return m_Header->data[m_Header->length - 1]; }

    void Clear()
    {
        memset_char(m_Header->data, '\0', m_Header->length);
        m_Header->length = 0;
    }

    void Reserve(u64 capacity)
    {
        if (m_Header->length <= capacity && m_Header->capacity != capacity)
        {
            m_Header = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                             sizeof(Header) + capacity,
                                                                             m_Header->alignment_in_bytes));
            m_Header->capacity = capacity;
        }
    }

    void Reverse()
    {
        char *first = m_Header->data;
        char *last  = m_Header->data + m_Header->length - 1;

        while (first < last)
        {
            char temp = *first;
            *first++  = *last;
            *last--   = temp;
        }
    }

    s8 Compare(const char *cstring) const
    {
        const char *left = m_Header->data;

        while (*left++ == *cstring++)
        {
        }

        if (*left < *cstring) return -1;
        if (*left > *cstring) return  1;
        return 0;
    }

    s8 Compare(const ConstString& const_string) const
    {
        const char *left  = m_Header->data;
        const char *right = const_string.Data();

        while (*left++ == *right++)
        {
        }

        if (*left < *right) return -1;
        if (*left > *right) return  1;
        return 0;
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    s8 Compare(const StaticString<ss_capacity, ss_aligned_capacity>& static_string) const
    {
        const char *left  = m_Header->data;
        const char *right = static_string.Data();

        while (*left++ == *right++)
        {
        }

        if (*left < *right) return -1;
        if (*left > *right) return  1;
        return 0;
    }

    s8 Compare(const String& other) const
    {
        const char *left  = m_Header->data;
        const char *right = other.m_Header->data;

        while (*left++ == *right++)
        {
        }

        if (*left < *right) return -1;
        if (*left > *right) return  1;
        return 0;
    }

    bool Equals(const char *cstring) const
    {
        return m_Header->length == strlen(cstring) && !Compare(cstring);
    }

    bool Equals(const char *cstring, u64 length) const
    {
        return m_Header->length == length && !Compare(cstring);
    }

    bool Equals(const ConstString& const_string) const
    {
        return m_Header->length == const_string.Length() && !Compare(const_string);
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    bool Equals(const StaticString<ss_capacity, ss_aligned_capacity>& static_string) const
    {
        return m_Header->length == static_string.Length()
            && !Compare(static_string);
    }

    bool Equals(const String& other) const
    {
        return m_Header->length == other.m_Header->length
            && !Compare(other);
    }

    String& Insert(u64 where, char symbol, u64 count = 1)
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;

        m_Header->length += count;
        Expand();

        if (where < old_length)
        {
            MoveMemory(m_Header->data + where + count,
                       m_Header->data + where,
                       old_length - where);
        }

        memset_char(m_Header->data + where, symbol, count);
        return *this;
    }

    String& Insert(u64 where, const char *cstring)
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;
        u64 length     = strlen(cstring);

        m_Header->length += length;
        Expand();

        if (where < old_length)
        {
            MoveMemory(m_Header->data + where + length,
                       m_Header->data + where,
                       old_length - where);
        }

        CopyMemory(m_Header->data + where, cstring, length);
        return *this;
    }

    String& Insert(u64 where, const char *cstring, u64 length)
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;

        m_Header->length += length;
        Expand();

        if (where < old_length)
        {
            MoveMemory(m_Header->data + where + length,
                       m_Header->data + where,
                       old_length - where);
        }

        CopyMemory(m_Header->data + where, cstring, length);
        return *this;
    }

    String& Insert(u64 where, const ConstString& const_string)
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;

        m_Header->length += const_string.Length();
        Expand();

        if (where < old_length)
        {
            MoveMemory(m_Header->data + where + const_string.Length(),
                       m_Header->data + where,
                       old_length - where);
        }

        CopyMemory(m_Header->data + where, const_string.Data(), const_string.Length());
        return *this;
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    String& Insert(u64 where, const StaticString<ss_capacity, ss_aligned_capacity>& static_string)
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;

        m_Header->length += static_string.Length();
        Expand();

        if (where < old_length)
        {
            MoveMemory(m_Header->data + where + static_string.Length(),
                       m_Header->data + where,
                       old_length - where);
        }

        CopyMemory(m_Header->data + where, static_string.Data(), static_string.Length());
        return *this;
    }

    String& Insert(u64 where, const String& string)
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;

        m_Header->length += string.m_Header->length;
        Expand();

        if (where < old_length)
        {
            MoveMemory(m_Header->data + where + string.m_Header->length,
                       m_Header->data + where,
                       old_length - where);
        }

        CopyMemory(m_Header->data + where, string.m_Header->data, string.m_Header->length);
        return *this;
    }

    String& Erase(u64 from)
    {
        u64 to = from + 1;
        CheckM(from < m_Header->length && from < to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);

        u64 delta = to - from;

        if (from < m_Header->length - 1)
        {
            MoveMemory(m_Header->data + from,
                       m_Header->data + to,
                       m_Header->length - to);
        }

        memset_char(m_Header->data + m_Header->length - delta, '\0', delta);
        m_Header->length -= delta;
        return *this;
    }

    String& Erase(u64 from, u64 to)
    {
        CheckM(from < m_Header->length && from < to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);

        u64 delta = to - from;

        if (from < m_Header->length - 1)
        {
            MoveMemory(m_Header->data + from,
                       m_Header->data + to,
                       m_Header->length - to);
        }

        memset_char(m_Header->data + m_Header->length - delta, '\0', delta);
        m_Header->length -= delta;
        return *this;
    }

    String& Replace(u64 from, u64 to, char symbol, u64 count = 1)
    {
        Erase(from, to);
        Insert(from, symbol, count);
        return *this;
    }

    String& Replace(u64 from, u64 to, const char *cstring)
    {
        Erase(from, to);
        Insert(from, cstring);
        return *this;
    }

    String& Replace(u64 from, u64 to, const char *cstring, u64 length)
    {
        Erase(from, to);
        Insert(from, cstring, length);
        return *this;
    }

    String& Replace(u64 from, u64 to, const ConstString& const_string)
    {
        Erase(from, to);
        Insert(from, const_string);
        return *this;
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    String& Replace(u64 from, u64 to, const StaticString<ss_capacity, ss_aligned_capacity>& static_string)
    {
        Erase(from, to);
        Insert(from, static_string);
        return *this;
    }

    String& Replace(u64 from, u64 to, const String& string)
    {
        Erase(from, to);
        Insert(from, string);
        return *this;
    }

    String& PushFront(char symbol, u64 count = 1)
    {
        return Insert(0, symbol, count);
    }

    String& PushFront(const char *cstring)
    {
        return Insert(0, cstring);
    }

    String& PushFront(const char *cstring, u64 length)
    {
        return Insert(0, cstring, length);
    }

    String& PushFront(const ConstString& const_string)
    {
        return Insert(0, const_string);
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    String& PushFront(const StaticString<ss_capacity, ss_aligned_capacity>& static_string)
    {
        return Insert(0, static_string);
    }

    String& PushFront(const String& string)
    {
        return Insert(0, string);
    }
    
    String& PushBack(char symbol, u64 count = 1)
    {
        return Insert(m_Header->length, symbol, count);
    }

    String& PushBack(const char *cstring)
    {
        return Insert(m_Header->length, cstring);
    }

    String& PushBack(const char *cstring, u64 length)
    {
        return Insert(m_Header->length, cstring, length);
    }

    String& PushBack(const ConstString& const_string)
    {
        return Insert(m_Header->length, const_string);
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    String& PushBack(const StaticString<ss_capacity, ss_aligned_capacity>& static_string)
    {
        return Insert(m_Header->length, static_string);
    }

    String& PushBack(const String& string)
    {
        return Insert(m_Header->length, string);
    }

    static String Concat(const String& left, char right, u64 count = 1)
    {
        return RTTI::move(String(left).PushBack(right, count));
    }

    static String Concat(const String& left, const char *right, u64 right_length)
    {
        return RTTI::move(String(left).PushBack(right, right_length));
    }

    static String Concat(const String& left, const ConstString& right)
    {
        return RTTI::move(String(left).PushBack(right));
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    static String Concat(const String& left, const StaticString<ss_capacity, ss_aligned_capacity>& right)
    {
        return RTTI::move(String(left).PushBack(right));
    }

    static String Concat(const String& left, const String& right)
    {
        return RTTI::move(String(left).PushBack(right));
    }

    static String Concat(const String& left, String&& right)
    {
        return RTTI::move(right.PushFront(left));
    }

    static String Concat(String&& left, char right, u64 count = 1)
    {
        return RTTI::move(left.PushBack(right, count));
    }

    static String Concat(String&& left, const char *right, u64 right_length)
    {
        return RTTI::move(left.PushBack(right, right_length));
    }

    static String Concat(String&& left, const ConstString& right)
    {
        return RTTI::move(left.PushBack(right));
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    static String Concat(String&& left, const StaticString<ss_capacity, ss_aligned_capacity>& right)
    {
        return RTTI::move(left.PushBack(right));
    }

    static String Concat(String&& left, const String& right)
    {
        return RTTI::move(left.PushBack(right));
    }

    static String Concat(String&& left, String&& right)
    {
        return RTTI::move(left.PushBack(right));
    }

    static String Concat(char left, const String& right)
    {
        return RTTI::move(String(right).PushFront(left));
    }

    static String Concat(const char *left, u64 left_length, const String& right)
    {
        return RTTI::move(String(right).PushFront(left, left_length));
    }

    static String Concat(const ConstString& left, const String& right)
    {
        return RTTI::move(String(right).PushFront(left));
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    static String Concat(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String& right)
    {
        return RTTI::move(String(right).PushFront(left));
    }

    static String Concat(char left, String&& right)
    {
        return RTTI::move(right.PushFront(left));
    }

    static String Concat(const char *left, u64 left_length, String&& right)
    {
        return RTTI::move(right.PushFront(left, left_length));
    }

    static String Concat(const ConstString& left, String&& right)
    {
        return RTTI::move(right.PushFront(left));
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    static String Concat(const StaticString<ss_capacity, ss_aligned_capacity>& left, String&& right)
    {
        return RTTI::move(right.PushFront(left));
    }

    String SubString(u64 from) const &
    {
        CheckM(from <= m_Header->length, "Bad arguments: from = %I64u, length = %I64u", from, m_Header->length);
        if (from == 0)
        {
            return *this;
        }
        else if (from == m_Header->length)
        {
            return RTTI::move(String(m_Header->allocator, 16, m_Header->alignment_in_bytes));
        }
        else
        {
            return RTTI::move(String(m_Header->allocator, m_Header->data + from, m_Header->length - from, m_Header->alignment_in_bytes));
        }
    }

    String SubString(u64 from, u64 to) const &
    {
        CheckM(from <= to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);
        if (from == 0 && to == m_Header->length)
        {
            return *this;
        }
        else if (from == to)
        {
            return RTTI::move(String(m_Header->allocator, 16, m_Header->alignment_in_bytes));
        }
        else
        {
            return RTTI::move(String(m_Header->allocator, m_Header->data + from, to - from, m_Header->alignment_in_bytes));
        }
    }

    String SubString(u64 from) &&
    {
        CheckM(from <= m_Header->length, "Bad arguments: from = %I64u, length = %I64u", from, m_Header->length);
        if (from == 0)
        {
            return RTTI::move(*this);
        }
        else if (from == m_Header->length)
        {
            return RTTI::move(Clear());
        }
        else
        {
            return RTTI::move(Erase(0, from));
        }
    }

    String SubString(u64 from, u64 to) &&
    {
        CheckM(from <= to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);
        if (from == 0 && to == m_Header->length)
        {
            return RTTI::move(*this);
        }
        else if (from == to)
        {
            return RTTI::move(Clear());
        }
        else
        {
            return RTTI::move(Erase(to, m_Header->length).Erase(0, from));
        }
    }

    u64 Find(char symbol, u64 offset = 0) const
    {
        CheckM(offset < m_Header->length, "Offset out of bounds.");

        for (const char *it = m_Header->data + offset; it; ++it)
        {
            if (*it == symbol)
            {
                return it - m_Header->data;
            }
        }
        return npos;
    }

    u64 Find(const char *cstring, u64 offset = 0) const
    {
        CheckM(offset < m_Header->length, "Offset out of bounds.");

        u64 cstring_length = strlen(cstring);

        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data + offset; it; ++it)
        {
            const char *sub_end = it + cstring_length;

            if (sub_end > _end)
            {
                return npos;
            }

            const char *sub  = it;
            const char *cstr = cstring;

            while (*sub++ == *cstr++)
            {
            }

            if (sub == sub_end)
            {
                return it - m_Header->data;
            }
        }

        return npos;
    }

    u64 Find(const char *cstring, u64 cstring_length, u64 offset = 0) const
    {
        CheckM(offset < m_Header->length, "Offset out of bounds.");

        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data + offset; it; ++it)
        {
            const char *sub_end = it + cstring_length;

            if (sub_end > _end)
            {
                return npos;
            }

            const char *sub  = it;
            const char *cstr = cstring;

            while (*sub++ == *cstr++)
            {
            }

            if (sub == sub_end)
            {
                return it - m_Header->data;
            }
        }

        return npos;
    }

    u64 Find(const ConstString& const_string, u64 offset = 0) const
    {
        CheckM(offset < m_Header->length, "Offset out of bounds.");

        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data + offset; it; ++it)
        {
            const char *sub_end = it + const_string.Length();

            if (sub_end > _end)
            {
                return npos;
            }

            const char *sub  = it;
            const char *cstr = const_string.Data();

            while (*sub++ == *cstr++)
            {
            }

            if (sub == sub_end)
            {
                return it - m_Header->data;
            }
        }

        return npos;
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    u64 Find(const StaticString<ss_capacity, ss_aligned_capacity>& static_string, u64 offset = 0) const
    {
        CheckM(offset < m_Header->length, "Offset out of bounds.");

        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data + offset; it; ++it)
        {
            const char *sub_end = it + static_string.Length();

            if (sub_end > _end)
            {
                return npos;
            }

            const char *sub  = it;
            const char *cstr = static_string.Data();

            while (*sub++ == *cstr++)
            {
            }

            if (sub == sub_end)
            {
                return it - m_Header->data;
            }
        }

        return npos;
    }

    u64 Find(const String& string, u64 offset = 0) const
    {
        CheckM(offset < m_Header->length, "Offset out of bounds.");

        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data + offset; it; ++it)
        {
            const char *sub_end = it + string.m_Header->length;

            if (sub_end > _end)
            {
                return npos;
            }

            const char *sub  = it;
            const char *cstr = string.m_Header->data;

            while (*sub++ == *cstr++)
            {
            }

            if (sub == sub_end)
            {
                return it - m_Header->data;
            }
        }

        return npos;
    }

    String& operator=(nullptr_t)
    {
        memset_char(m_Header->data, '\0', m_Header->length);

        m_Header->length = 0;
        Fit();

        return *this;
    }

    String& operator=(char symbol)
    {
        memset_char(m_Header->data + 1, '\0', m_Header->length - 1);

        m_Header->length = 1;
        Fit();

        *m_Header->data = symbol;
        return *this;
    }

    String& operator=(const char *cstring)
    {
        u64 cstring_length = strlen(cstring);

        memset_char(m_Header->data + cstring_length, '\0', m_Header->length - cstring_length);

        m_Header->length = cstring_length;
        Fit();

        CopyMemory(m_Header->data, cstring, cstring_length);
        return *this;
    }

    String& operator=(const ConstString& const_string)
    {
        memset_char(m_Header->data + const_string.Length(), '\0', m_Header->length - const_string.Length());

        m_Header->length = const_string.Length();
        Fit();

        CopyMemory(m_Header->data, const_string.Data(), const_string.Length());
        return *this;
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    String& operator=(const StaticString<ss_capacity, ss_aligned_capacity>& static_string)
    {
        memset_char(m_Header->data + static_string.Length(), '\0', m_Header->length - static_string.Length());

        m_Header->length = static_string.Length();
        Fit();

        CopyMemory(m_Header->data, static_string.Data(), static_string.Length());
        return *this;
    }

    String& operator=(const String& other)
    {
        if (this != &other)
        {
            memset_char(m_Header->data + other.m_Header->length, '\0', m_Header->length - other.m_Header->length);

            m_Header->length = other.m_Header->length;
            Fit();

            CopyMemory(m_Header->data, other.m_Header->data, other.m_Header->length);
        }
        return *this;
    }

    String& operator=(String&& other) noexcept
    {
        if (this != &other)
        {
            m_Header->allocator->DeAllocA(m_Header);

            m_Header       = other.m_Header;
            other.m_Header = null;
        }
        return *this;
    }

    String& operator+=(char right)
    {
        return PushBack(right);
    }

    String& operator+=(const char *right)
    {
        return PushBack(right);
    }

    String& operator+=(const ConstString& right)
    {
        return PushBack(right);
    }

    template<u64 ss_capacity, u64 ss_aligned_capacity>
    String& operator+=(const StaticString<ss_capacity, ss_aligned_capacity>& right)
    {
        return PushBack(right);
    }

    String& operator+=(const String& right)
    {
        return PushBack(right);
    }

    template<typename T>
    String& operator+=(const T& right)
    {
        return PushBack(right.ToString());
    }

    char operator[](u64 index) const
    {
        CheckM(index < m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length - 1, index);
        return m_Header->data[index];
    }

    char& operator[](u64 index)
    {
        CheckM(index < m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length - 1, index);
        return m_Header->data[index];
    }

private:
    void Expand()
    {
        if (m_Header->length > m_Header->capacity)
        {
            m_Header->capacity *= 2;
            m_Header            = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                                        sizeof(Header) + m_Header->capacity,
                                                                                        m_Header->alignment_in_bytes));
        }
    }

    void Fit()
    {
        if (2 * m_Header->length < m_Header->capacity)
        {
            m_Header->capacity = 2 * m_Header->length;
            m_Header           = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                                       sizeof(Header) + m_Header->capacity,
                                                                                       m_Header->alignment_in_bytes));
        }
    }

private:
    struct Header
    {
        Allocator_t *allocator;
        u64          length;
        u64          capacity;
        u64          alignment_in_bytes;
        #pragma warning(suppress: 4200)
        char         data[0];
    };

    Header *m_Header;
};

template<typename Allocator_t, typename ...T> String(Allocator_t *, T...)        -> String<Allocator_t>;
template<typename Allocator_t>                String(const String<Allocator_t>&) -> String<Allocator_t>;
template<typename Allocator_t>                String(String<Allocator_t>&&)      -> String<Allocator_t>;

template<typename LAllocator>                                           INLINE bool operator==(const String<LAllocator>& left, const char                                           *right) { return left.Length() == strlen(right)  && !left.Compare(right); }
template<typename LAllocator>                                           INLINE bool operator==(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() == right.Length() && !left.Compare(right); }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator==(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() == right.Length() && !left.Compare(right); }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator==(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() == right.Length() && !left.Compare(right); }

template<typename RAllocator>                                           INLINE bool operator==(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  == right.Length() && !right.Compare(left); }
template<typename RAllocator>                                           INLINE bool operator==(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() == right.Length() && !right.Compare(left); }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator==(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() == right.Length() && !right.Compare(left); }

template<typename LAllocator>                                           INLINE bool operator!=(const String<LAllocator>& left, const char                                           *right) { return left.Length() != strlen(right)  || left.Compare(right); }
template<typename LAllocator>                                           INLINE bool operator!=(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() != right.Length() || left.Compare(right); }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator!=(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() != right.Length() || left.Compare(right); }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator!=(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() != right.Length() || left.Compare(right); }

template<typename RAllocator>                                           INLINE bool operator!=(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  != right.Length() || right.Compare(left); }
template<typename RAllocator>                                           INLINE bool operator!=(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() != right.Length() || right.Compare(left); }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator!=(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() != right.Length() || right.Compare(left); }

template<typename LAllocator>                                           INLINE bool operator<(const String<LAllocator>& left, const char                                           *right) { return left.Length() <= strlen(right)  && left.Compare(right) < 0; }
template<typename LAllocator>                                           INLINE bool operator<(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator<(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator<(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }

template<typename RAllocator>                                           INLINE bool operator<(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  <= right.Length() && right.Compare(left) > 0; }
template<typename RAllocator>                                           INLINE bool operator<(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() <= right.Length() && right.Compare(left) > 0; }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator<(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() <= right.Length() && right.Compare(left) > 0; }

template<typename LAllocator>                                           INLINE bool operator>(const String<LAllocator>& left, const char                                           *right) { return left.Length() >= strlen(right)  && left.Compare(right) > 0; }
template<typename LAllocator>                                           INLINE bool operator>(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator>(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator>(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }

template<typename RAllocator>                                           INLINE bool operator>(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  >= right.Length() && right.Compare(left) < 0; }
template<typename RAllocator>                                           INLINE bool operator>(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() >= right.Length() && right.Compare(left) < 0; }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator>(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() >= right.Length() && right.Compare(left) < 0; }

template<typename LAllocator>                                           INLINE bool operator<=(const String<LAllocator>& left, const char                                           *right) { return left.Length() <= strlen(right)  && left.Compare(right) <= 0; }
template<typename LAllocator>                                           INLINE bool operator<=(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator<=(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator<=(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }

template<typename RAllocator>                                           INLINE bool operator<=(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  <= right.Length() && right.Compare(left) >= 0; }
template<typename RAllocator>                                           INLINE bool operator<=(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() <= right.Length() && right.Compare(left) >= 0; }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator<=(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() <= right.Length() && right.Compare(left) >= 0; }

template<typename LAllocator>                                           INLINE bool operator>=(const String<LAllocator>& left, const char                                           *right) { return left.Length() >= strlen(right)  && left.Compare(right) >= 0; }
template<typename LAllocator>                                           INLINE bool operator>=(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator>=(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator>=(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }

template<typename RAllocator>                                           INLINE bool operator>=(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  >= right.Length() && right.Compare(left) <= 0; }
template<typename RAllocator>                                           INLINE bool operator>=(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() >= right.Length() && right.Compare(left) <= 0; }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator>=(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() >= right.Length() && right.Compare(left) <= 0; }

template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>& left,       char                                            right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>& left, const char                                           *right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>& left, const ConstString&                                    right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE String<Allocator> operator+(const String<Allocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>& left, const String<Allocator>&                              right) { return String<Allocator>::Concat(left, right); }

template<typename Allocator>                                           INLINE String<Allocator> operator+(      char                                            left, const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const char                                           *left, const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const ConstString&                                    left, const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE String<Allocator> operator+(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }

template<typename Allocator>                                           INLINE String<Allocator> operator+(String<Allocator>&& left,       char                                            right) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(String<Allocator>&& left, const char                                           *right) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(String<Allocator>&& left, const ConstString&                                    right) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE String<Allocator> operator+(String<Allocator>&& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(String<Allocator>&& left, const String<Allocator>&                              right) { return String<Allocator>::Concat(RTTI::move(left), right); }

template<typename Allocator>                                           INLINE String<Allocator> operator+(      char                                            left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const char                                           *left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const ConstString&                                    left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE String<Allocator> operator+(const StaticString<ss_capacity, ss_aligned_capacity>& left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>&                              left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
