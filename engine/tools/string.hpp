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

    String(Allocator_t *allocator, const char *cstring, u64 length = npos, u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;
        if (length == npos) length = strlen(cstring);

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * length, alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = length;
        m_Header->capacity           = 2 * length;
        m_Header->alignment_in_bytes = alignment_in_bytes;

        CopyMemory(m_Header->data, cstring, length);
    }

    template<u64 count>
    String(Allocator_t *allocator, const char (&array)[count], u64 alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        Check(allocator);
        if (alignment_in_bytes < ENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = ENGINE_DEFAULT_ALIGNMENT;

        constexpr u64 array_length = count - 1;

        m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * array_length, alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = array_length;
        m_Header->capacity           = 2 * array_length;
        m_Header->alignment_in_bytes = alignment_in_bytes;

        CopyMemory(m_Header->data, array, array_length);
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

    constexpr bool Empty() const { return !m_Header->length; }

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

    constexpr const char& First() const { return *m_Header->data;                      }
    constexpr const char& Last()  const { return m_Header->data[m_Header->length - 1]; }

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

    bool Equals(const char *cstring, u64 length = npos) const
    {
        if (length == npos) length = strlen(cstring);

        return m_Header->length == length
            && !Compare(cstring);
    }

    template<u64 count>
    bool Equals(const char (&array)[count]) const
    {
        return m_Header->length == count - 1
            && !Compare(array);
    }

    bool Equals(const ConstString& const_string) const
    {
        return m_Header->length == const_string.Length()
            && !Compare(const_string);
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

    String& Insert(u64 where, char symbol)
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;

        ++m_Header->length;
        Expand();

        if (where < old_length)
        {
            MoveMemory(m_Header->data + where + 1,
                       m_Header->data + where,
                       old_length - where);
        }

        m_Header->data[where] = symbol;
        return *this;
    }

    String& Insert(u64 where, const char *cstring, u64 length = npos)
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;
        if (length == npos) length = strlen(cstring);

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

    template<u64 count>
    String& Insert(u64 where, const char (&array)[count])
    {
        CheckM(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

        u64 old_length = m_Header->length;
        constexpr u64 length = count - 1;

        m_Header->length += length;
        Expand();

        if (where < old_length)
        {
            MoveMemory(m_Header->data + where + length,
                       m_Header->data + where,
                       old_length - where);
        }

        CopyMemory(m_Header->data + where, array, length);
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

    String& Erase(u64 from, u64 to = npos)
    {
        if (to == npos) to = from + 1;
        CheckM(from < m_Header->length && from < to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);

        u64 delta = to - from;

        if (from < m_Header->length - 1)
        {
            MoveMemory(m_Header->data + from,
                       m_Header->data + to,
                       delta);
        }

        memset_char(m_Header->data + m_Header->length - delta, '\0', delta);

        m_Header->length -= delta;
        Fit();

        return *this;
    }

    String& PushFront(char symbol)
    {
        return Insert(0, symbol);
    }

    String& PushFront(const char *cstring, u64 length = npos)
    {
        return Insert(0, cstring, length);
    }

    template<u64 count>
    String& PushFront(const char (&array)[count])
    {
        return Insert(0, array);
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
    
    String& PushBack(char symbol)
    {
        return Insert(m_Header->length, symbol);
    }

    String& PushBack(const char *cstring, u64 length = npos)
    {
        return Insert(m_Header->length, cstring, length);
    }

    template<u64 count>
    String& PushBack(const char (&array)[count])
    {
        return Insert(m_Header->length, array);
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

    static String Concat(const String& left, char right)
    {
        return RTTI::move(String(left).PushBack(right));
    }

    static String Concat(const String& left, const char *right, u64 right_length)
    {
        return RTTI::move(String(left).PushBack(right, right_length));
    }

    template<u64 count>
    static String Concat(const String& left, const char (&right)[count])
    {
        return RTTI::move(String(left).PushBack(right));
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

    static String Concat(String&& left, char right)
    {
        return RTTI::move(left.PushBack(right));
    }

    static String Concat(String&& left, const char *right, u64 right_length)
    {
        return RTTI::move(left.PushBack(right, right_length));
    }

    template<u64 count>
    static String Concat(String&& left, const char (&right)[count])
    {
        return RTTI::move(left.PushBack(right));
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

    template<u64 count>
    static String Concat(const char (&left)[count], const String& right)
    {
        return RTTI::move(String(right).PushFront(left));
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

    template<u64 count>
    static String Concat(const char (&left)[count], String&& right)
    {
        return RTTI::move(right.PushFront(left));
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

    String SubString(u64 from, u64 to = npos) const &
    {
        return RTTI::move(String(*this).Erase(from, to));
    }

    String SubString(u64 from, u64 to = npos) &&
    {
        return RTTI::move(Erase(from, to));
    }

    u64 Find(char symbol) const
    {
        for (const char *it = m_Header->data; it; ++it)
        {
            if (*it == symbol)
            {
                return it - m_Header->data;
            }
        }
        return npos;
    }

    u64 Find(const char *cstring, u64 cstring_length = npos) const
    {
        if (cstring_length == npos) cstring_length = strlen(cstring);

        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data; it; ++it)
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

    template<u64 count>
    u64 Find(const char (&array)[count]) const
    {
        constexpr u64 length = count - 1;

        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data; it; ++it)
        {
            const char *sub_end = it + length;

            if (sub_end > _end)
            {
                return npos;
            }

            const char *sub  = it;
            const char *parr = array;

            while (*sub++ == *parr++)
            {
            }

            if (sub == sub_end)
            {
                return it - m_Header->data;
            }
        }

        return npos;
    }

    u64 Find(const ConstString& const_string) const
    {
        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data; it; ++it)
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
    u64 Find(const StaticString<ss_capacity, ss_aligned_capacity>& static_string) const
    {
        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data; it; ++it)
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

    u64 Find(const String& string) const
    {
        const char *_end = m_Header->data + m_Header->length;

        for (const char *it = m_Header->data; it; ++it)
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

    String& operator+=(char right)
    {
        return PushBack(right);
    }

    String& operator+=(const char *right)
    {
        return PushBack(right);
    }

    template<u64 count>
    String& operator+=(const char (&right)[count])
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

    template<u64 count>
    String& operator=(const char (&array)[count])
    {
        constexpr u64 array_length = count - 1;

        memset_char(m_Header->data + array_length, '\0', m_Header->length - array_length);

        m_Header->length = array_length;
        Fit();

        CopyMemory(m_Header->data, array, array_length);
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

template<typename Allocator_t, typename ...T>
String(Allocator_t, T...) -> String<Allocator_t>;

template<typename LAllocator>                                           INLINE bool operator==(const String<LAllocator>& left, const char                                           *right) { return left.Length() == strlen(right)  && !left.Compare(right); }
template<typename LAllocator, u64 count>                                INLINE bool operator==(const String<LAllocator>& left, const char                                  (&right)[count]) { return left.Length() == count - 1      && !left.Compare(right); }
template<typename LAllocator>                                           INLINE bool operator==(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() == right.Length() && !left.Compare(right); }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator==(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() == right.Length() && !left.Compare(right); }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator==(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() == right.Length() && !left.Compare(right); }

template<typename RAllocator>                                           INLINE bool operator==(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  == right.Length() && !right.Compare(left); }
template<typename RAllocator, u64 count>                                INLINE bool operator==(const char                                  (&left)[count], const String<RAllocator>& right) { return count - 1     == right.Length() && !right.Compare(left); }
template<typename RAllocator>                                           INLINE bool operator==(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() == right.Length() && !right.Compare(left); }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator==(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() == right.Length() && !right.Compare(left); }

template<typename LAllocator>                                           INLINE bool operator!=(const String<LAllocator>& left, const char                                           *right) { return left.Length() != strlen(right)  || left.Compare(right); }
template<typename LAllocator, u64 count>                                INLINE bool operator!=(const String<LAllocator>& left, const char                                  (&right)[count]) { return left.Length() != count - 1      || left.Compare(right); }
template<typename LAllocator>                                           INLINE bool operator!=(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() != right.Length() || left.Compare(right); }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator!=(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() != right.Length() || left.Compare(right); }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator!=(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() != right.Length() || left.Compare(right); }

template<typename RAllocator>                                           INLINE bool operator!=(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  != right.Length() || right.Compare(left); }
template<typename RAllocator, u64 count>                                INLINE bool operator!=(const char                                  (&left)[count], const String<RAllocator>& right) { return count - 1     != right.Length() || right.Compare(left); }
template<typename RAllocator>                                           INLINE bool operator!=(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() != right.Length() || right.Compare(left); }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator!=(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() != right.Length() || right.Compare(left); }

template<typename LAllocator>                                           INLINE bool operator<(const String<LAllocator>& left, const char                                           *right) { return left.Length() <= strlen(right)  && left.Compare(right) < 0; }
template<typename LAllocator, u64 count>                                INLINE bool operator<(const String<LAllocator>& left, const char                                  (&right)[count]) { return left.Length() <= count - 1      && left.Compare(right) < 0; }
template<typename LAllocator>                                           INLINE bool operator<(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator<(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator<(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }

template<typename RAllocator>                                           INLINE bool operator<(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  <= right.Length() && right.Compare(left) > 0; }
template<typename RAllocator, u64 count>                                INLINE bool operator<(const char                                  (&left)[count], const String<RAllocator>& right) { return count - 1     <= right.Length() && right.Compare(left) > 0; }
template<typename RAllocator>                                           INLINE bool operator<(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() <= right.Length() && right.Compare(left) > 0; }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator<(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() <= right.Length() && right.Compare(left) > 0; }

template<typename LAllocator>                                           INLINE bool operator>(const String<LAllocator>& left, const char                                           *right) { return left.Length() >= strlen(right)  && left.Compare(right) > 0; }
template<typename LAllocator, u64 count>                                INLINE bool operator>(const String<LAllocator>& left, const char                                  (&right)[count]) { return left.Length() >= count - 1      && left.Compare(right) > 0; }
template<typename LAllocator>                                           INLINE bool operator>(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator>(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator>(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }

template<typename RAllocator>                                           INLINE bool operator>(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  >= right.Length() && right.Compare(left) < 0; }
template<typename RAllocator, u64 count>                                INLINE bool operator>(const char                                  (&left)[count], const String<RAllocator>& right) { return count - 1     >= right.Length() && right.Compare(left) < 0; }
template<typename RAllocator>                                           INLINE bool operator>(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() >= right.Length() && right.Compare(left) < 0; }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator>(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() >= right.Length() && right.Compare(left) < 0; }

template<typename LAllocator>                                           INLINE bool operator<=(const String<LAllocator>& left, const char                                           *right) { return left.Length() <= strlen(right)  && left.Compare(right) <= 0; }
template<typename LAllocator, u64 count>                                INLINE bool operator<=(const String<LAllocator>& left, const char                                  (&right)[count]) { return left.Length() <= count - 1      && left.Compare(right) <= 0; }
template<typename LAllocator>                                           INLINE bool operator<=(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator<=(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator<=(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }

template<typename RAllocator>                                           INLINE bool operator<=(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  <= right.Length() && right.Compare(left) >= 0; }
template<typename RAllocator, u64 count>                                INLINE bool operator<=(const char                                  (&left)[count], const String<RAllocator>& right) { return count - 1     <= right.Length() && right.Compare(left) >= 0; }
template<typename RAllocator>                                           INLINE bool operator<=(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() <= right.Length() && right.Compare(left) >= 0; }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator<=(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() <= right.Length() && right.Compare(left) >= 0; }

template<typename LAllocator>                                           INLINE bool operator>=(const String<LAllocator>& left, const char                                           *right) { return left.Length() >= strlen(right)  && left.Compare(right) >= 0; }
template<typename LAllocator, u64 count>                                INLINE bool operator>=(const String<LAllocator>& left, const char                                  (&right)[count]) { return left.Length() >= count - 1      && left.Compare(right) >= 0; }
template<typename LAllocator>                                           INLINE bool operator>=(const String<LAllocator>& left, const ConstString&                                    right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }
template<typename LAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator>=(const String<LAllocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }
template<typename LAllocator, typename RAllocator>                      INLINE bool operator>=(const String<LAllocator>& left, const String<RAllocator>&                             right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }

template<typename RAllocator>                                           INLINE bool operator>=(const char                                           *left, const String<RAllocator>& right) { return strlen(left)  >= right.Length() && right.Compare(left) <= 0; }
template<typename RAllocator, u64 count>                                INLINE bool operator>=(const char                                  (&left)[count], const String<RAllocator>& right) { return count - 1     >= right.Length() && right.Compare(left) <= 0; }
template<typename RAllocator>                                           INLINE bool operator>=(const ConstString&                                    left, const String<RAllocator>& right) { return left.Length() >= right.Length() && right.Compare(left) <= 0; }
template<typename RAllocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE bool operator>=(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<RAllocator>& right) { return left.Length() >= right.Length() && right.Compare(left) <= 0; }

template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>& left,       char                                            right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>& left, const char                                           *right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator, u64 count>                                INLINE String<Allocator> operator+(const String<Allocator>& left, const char                                  (&right)[count]) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>& left, const ConstString&                                    right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE String<Allocator> operator+(const String<Allocator>& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>& left, const String<Allocator>&                              right) { return String<Allocator>::Concat(left, right); }

template<typename Allocator>                                           INLINE String<Allocator> operator+(      char                                            left, const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const char                                           *left, const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator, u64 count>                                INLINE String<Allocator> operator+(const char                                  (&left)[count], const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const ConstString&                                    left, const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }
template<typename Allocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE String<Allocator> operator+(const StaticString<ss_capacity, ss_aligned_capacity>& left, const String<Allocator>& right) { return String<Allocator>::Concat(left, right); }

template<typename Allocator>                                           INLINE String<Allocator> operator+(String<Allocator>&& left,       char                                            right) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(String<Allocator>&& left, const char                                           *right) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator, u64 count>                                INLINE String<Allocator> operator+(String<Allocator>&& left, const char                                  (&right)[count]) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(String<Allocator>&& left, const ConstString&                                    right) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE String<Allocator> operator+(String<Allocator>&& left, const StaticString<ss_capacity, ss_aligned_capacity>& right) { return String<Allocator>::Concat(RTTI::move(left), right); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(String<Allocator>&& left, const String<Allocator>&                              right) { return String<Allocator>::Concat(RTTI::move(left), right); }

template<typename Allocator>                                           INLINE String<Allocator> operator+(      char                                            left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const char                                           *left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator, u64 count>                                INLINE String<Allocator> operator+(const char                                  (&left)[count], String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const ConstString&                                    left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator, u64 ss_capacity, u64 ss_aligned_capacity> INLINE String<Allocator> operator+(const StaticString<ss_capacity, ss_aligned_capacity>& left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
template<typename Allocator>                                           INLINE String<Allocator> operator+(const String<Allocator>&                              left, String<Allocator>&& right) { return String<Allocator>::Concat(left, RTTI::move(right)); }
