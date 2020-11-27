//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"
#include "math/simd.h"
#include "tools/const_string.h"

template<u64 capacity, u64 aligned_capacity = AlignUp(capacity, CACHE_LINE_SIZE)>
class StaticString final
{
public:
    static constexpr u64 npos = U64_MAX;

    StaticString()
        : m_Length(0)
    {
        memset_char(m_Data, '\0', aligned_capacity);
    }

    StaticString(nullptr_t)
        : m_Length(0)
    {
        memset_char(m_Data, '\0', aligned_capacity);
    }

    StaticString(char symbol, u64 count = 1)
        : m_Length(count)
    {
        CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        memset_char(m_Data, symbol, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    StaticString(const char *cstring)
        : m_Length(strlen(cstring))
    {
        CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, cstring, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    StaticString(const char *cstring, u64 length)
        : m_Length(length)
    {
        CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, cstring, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    StaticString(const ConstString& const_string)
        : m_Length(const_string.Length())
    {
        CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, const_string.Data(), m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString(const StaticString<other_capacity, other_aligned_capacity>& other)
        : m_Length(other.m_Length)
    {
        CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, other.m_Data, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString(StaticString<other_capacity, other_aligned_capacity>&& other) noexcept
        : m_Length(other.m_Length)
    {
        CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, other.m_Data, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    ~StaticString()
    {
    }

    constexpr u64 Length() const { return m_Length; }

    constexpr const char *Data() const { return m_Data; }
    constexpr       char *Data()       { return m_Data; }

    constexpr const StaticString& ToString() const { return *this; }
    constexpr       StaticString& ToString()       { return *this; }

    constexpr bool Empty() const { return !m_Length; }

    constexpr const char *begin()   const { return m_Data;            }
    constexpr const char *cbegin()  const { return m_Data;            }
    constexpr const char *rbegin()  const { return m_Data + m_Length; }
    constexpr const char *crbegin() const { return m_Data + m_Length; }

    constexpr const char *end()   const { return m_Data + m_Length; }
    constexpr const char *cend()  const { return m_Data + m_Length; }
    constexpr const char *rend()  const { return m_Data;            }
    constexpr const char *crend() const { return m_Data;            }

    constexpr char *begin()  { return m_Data;            }
    constexpr char *rbegin() { return m_Data + m_Length; }

    constexpr char *end()  { return m_Data + m_Length; }
    constexpr char *rend() { return m_Data;            }

    constexpr char First() const { return *m_Data;              }
    constexpr char Last()  const { return m_Data[m_Length - 1]; }

    constexpr char& First() { return *m_Data;              }
    constexpr char& Last()  { return m_Data[m_Length - 1]; }

    void Clear()
    {
        memset_char(m_Data, '\0', m_Length);
        m_Length = 0;
    }

    void Reverse()
    {
        char *first = m_Data;
        char *last  = m_Data + m_Length - 1;

        while (first < last)
        {
            char temp = *first;
            *first++  = *last;
            *last--   = temp;
        }
    }

    s8 Compare(const char *cstring) const
    {
        const char *left = m_Data;

        while (*left++ == *cstring++)
        {
        }

        if (*left < *cstring) return -1;
        if (*left > *cstring) return  1;
        return 0;
    }

    s8 Compare(const ConstString& const_string) const
    {
        const char *left  = m_Data;
        const char *right = const_string.Data();

        while (*left++ == *right++)
        {
        }

        if (*left < *right) return -1;
        if (*left > *right) return  1;
        return 0;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    s8 Compare(const StaticString<other_capacity, other_aligned_capacity>& static_string) const
    {
        const char *left  = m_Data;
        const char *right = static_string.m_Data;

        while (*left++ == *right++)
        {
        }

        if (*left < *right) return -1;
        if (*left > *right) return  1;
        return 0;
    }

    bool Equals(const char *cstring) const
    {
        return m_Length == strlen(cstring) && !Compare(cstring);
    }

    bool Equals(const char *cstring, u64 length) const
    {
        return m_Length == length && !Compare(cstring);
    }

    bool Equals(const ConstString& const_string) const
    {
        return m_Length == const_string.Length() && !Compare(const_string);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    bool Equals(const StaticString<other_capacity, other_aligned_capacity>& static_string) const
    {
        if constexpr (aligned_capacity == 16 && other_aligned_capacity == 16)
        {
            return m_Length == static_string.m_Length && Math::mm_equals(m_Data, static_string.m_Data);
        }
        else if constexpr (aligned_capacity == 32 && other_aligned_capacity == 32)
        {
            return m_Length == static_string.m_Length && Math::mm256_equals(m_Data, static_string.m_Data);
        }
        else if constexpr (aligned_capacity == 64 && other_aligned_capacity == 64)
        {
            return m_Length == static_string.m_Length && Math::mm512_equals(m_Data, static_string.m_Data);
        }
        else
        {
            return m_Length == static_string.m_Length && !Compare(static_string);
        }
    }

    StaticString& Insert(u64 where, char symbol, u64 count = 1)
    {
        u64 entire_length = m_Length + count;
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

        u64 old_length = m_Length;

        m_Length += count;

        if (where < old_length)
        {
            MoveMemory(m_Data + where + count,
                       m_Data + where,
                       old_length - where);
        }

        memset_char(m_Data + where, symbol, count);
        return *this;
    }

    StaticString& Insert(u64 where, const char *cstring)
    {
        u64 length = strlen(cstring);

        u64 entire_length = m_Length + length;
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

        u64 old_length = m_Length;

        m_Length += length;

        if (where < old_length)
        {
            MoveMemory(m_Data + where + length,
                       m_Data + where,
                       old_length - where);
        }

        CopyMemory(m_Data + where, cstring, length);
        return *this;
    }

    StaticString& Insert(u64 where, const char *cstring, u64 length)
    {
        u64 entire_length = m_Length + length;
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

        u64 old_length = m_Length;

        m_Length += length;

        if (where < old_length)
        {
            MoveMemory(m_Data + where + length,
                       m_Data + where,
                       old_length - where);
        }

        CopyMemory(m_Data + where, cstring, length);
        return *this;
    }

    StaticString& Insert(u64 where, const ConstString& const_string)
    {
        u64 entire_length = m_Length + const_string.Length();
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

        u64 old_length = m_Length;

        m_Length += const_string.Length();

        if (where < old_length)
        {
            MoveMemory(m_Data + where + const_string.Length(),
                       m_Data + where,
                       old_length - where);
        }

        CopyMemory(m_Data + where, const_string.Data(), const_string.Length());
        return *this;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& Insert(u64 where, const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        u64 entire_length = m_Length + static_string.m_Length;
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

        u64 old_length = m_Length;

        m_Length += static_string.m_Length;

        if (where < old_length)
        {
            MoveMemory(m_Data + where + static_string.m_Length,
                       m_Data + where,
                       old_length - where);
        }

        CopyMemory(m_Data + where, static_string.m_Data, static_string.m_Length);
        return *this;
    }

    StaticString& Erase(u64 from)
    {
        u64 to = from + 1;
        CheckM(from < m_Length && from < to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);

        u64 delta = to - from;

        if (from < m_Length - 1)
        {
            MoveMemory(m_Data   + from,
                       m_Data   + to,
                       m_Length - to);
        }

        memset_char(m_Data + m_Length - delta, '\0', delta);
        m_Length -= delta;
        return *this;
    }

    StaticString& Erase(u64 from, u64 to)
    {
        CheckM(from < m_Length && from <= to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);
        if (from != to)
        {
            u64 delta = to - from;

            if (from < m_Length - 1)
            {
                MoveMemory(m_Data   + from,
                           m_Data   + to,
                           m_Length - to);
            }

            memset_char(m_Data + m_Length - delta, '\0', delta);
            m_Length -= delta;
        }
        return *this;
    }

    StaticString& Replace(u64 from, u64 to, char symbol, u64 count = 1)
    {
        Erase(from, to);
        Insert(from, symbol, count);
        return *this;
    }

    StaticString& Replace(u64 from, u64 to, const char *cstring)
    {
        Erase(from, to);
        Insert(from, cstring);
        return *this;
    }

    StaticString& Replace(u64 from, u64 to, const char *cstring, u64 length)
    {
        Erase(from, to);
        Insert(from, cstring, length);
        return *this;
    }

    StaticString& Replace(u64 from, u64 to, const ConstString& const_string)
    {
        Erase(from, to);
        Insert(from, const_string);
        return *this;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& Replace(u64 from, u64 to, const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        Erase(from, to);
        Insert(from, static_string);
        return *this;
    }

    StaticString& PushFront(char symbol, u64 count = 1)
    {
        return Insert(0, symbol, count);
    }

    StaticString& PushFront(const char *cstring)
    {
        return Insert(0, cstring);
    }

    StaticString& PushFront(const char *cstring, u64 length)
    {
        return Insert(0, cstring, length);
    }

    StaticString& PushFront(const ConstString& const_string)
    {
        return Insert(0, const_string);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& PushFront(const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        return Insert(0, static_string);
    }

    StaticString& PushBack(char symbol, u64 count = 1)
    {
        return Insert(m_Length, symbol, count);
    }

    StaticString& PushBack(const char *cstring)
    {
        return Insert(m_Length, cstring);
    }

    StaticString& PushBack(const char *cstring, u64 length)
    {
        return Insert(m_Length, cstring, length);
    }

    StaticString& PushBack(const ConstString& const_string)
    {
        return Insert(m_Length, const_string);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& PushBack(const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        return Insert(m_Length, static_string);
    }

    StaticString SubString(u64 from) const
    {
        CheckM(from <= m_Length, "Bad arguments: from = %I64u, length = %I64u", from, m_Length);
        if (from == 0)
        {
            return *this;
        }
        else if (from == m_Length)
        {
            return StaticString();
        }
        else
        {
            return StaticString(m_Data + from, m_Length - from);
        }
    }

    StaticString SubString(u64 from, u64 to) const
    {
        CheckM(from <= to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);
        if (from == 0 && to == m_Length)
        {
            return *this;
        }
        else if (from == to)
        {
            return StaticString();
        }
        else
        {
            return StaticString(m_Data + from, to - from);
        }
    }

    u64 Find(char what, u64 offset = 0) const
    {
        CheckM(offset < m_Length, "Offset out of bounds.");

        for (const char *it = m_Data + offset; it; ++it)
        {
            if (*it == what)
            {
                return it - m_Data;
            }
        }

        return npos;
    }

    u64 Find(const char *cstring, u64 offset = 0) const
    {
        CheckM(offset < m_Length, "Offset out of bounds.");

        u64 cstring_length = strlen(cstring);

        const char *_end = m_Data + m_Length;

        for (const char *it = m_Data + offset; it; ++it)
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
                return it - m_Data;
            }
        }

        return npos;
    }

    u64 Find(const char *cstring, u64 cstring_length, u64 offset = 0) const
    {
        CheckM(offset < m_Length, "Offset out of bounds.");

        const char *_end = m_Data + m_Length;

        for (const char *it = m_Data + offset; it; ++it)
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
                return it - m_Data;
            }
        }

        return npos;
    }

    u64 Find(const ConstString& const_string, u64 offset = 0) const
    {
        CheckM(offset < m_Length, "Offset out of bounds.");

        const char *_end = m_Data + m_Length;

        for (const char *it = m_Data + offset; it; ++it)
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
                return it - m_Data;
            }
        }

        return npos;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    u64 Find(const StaticString<other_capacity, other_aligned_capacity>& static_string, u64 offset = 0) const
    {
        CheckM(offset < m_Length, "Offset out of bounds.");

        const char *_end = m_Data + m_Length;

        for (const char *it = m_Data + offset; it; ++it)
        {
            const char *sub_end = it + static_string.m_Length;

            if (sub_end > _end)
            {
                return npos;
            }

            const char *sub  = it;
            const char *cstr = static_string.m_Data;

            while (*sub++ == *cstr++)
            {
            }

            if (sub == sub_end)
            {
                return it - m_Data;
            }
        }

        return npos;
    }

    u64 RFind(char what, u64 roffset = 0) const
    {
        CheckM(roffset < m_Length, "Offset out of bounds.");

        const char *_end = m_Data + m_Length - 1 - roffset;

        for (const char *it = _end; it > m_Data; --it)
        {
            if (*it == what)
            {
                return it - m_Data;
            }
        }

        if (what == *m_Data) return 0;

        return npos;
    }

    StaticString& operator=(nullptr_t)
    {
        memset_char(m_Data, '\0', m_Length);
        m_Length = 0;
        return *this;
    }

    StaticString& operator=(char symbol)
    {
        memset_char(m_Data + 1, '\0', m_Length - 1);
        m_Length = 1;
        *m_Data  = symbol;
        return *this;
    }

    StaticString& operator=(const char *cstring)
    {
        u64 length = strlen(cstring);
        CheckM(length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

        memset_char(m_Data + length, '\0', m_Length - length);

        m_Length = length;
        CopyMemory(m_Data, cstring, m_Length);

        return *this;
    }

    StaticString& operator=(const ConstString& const_string)
    {
        CheckM(const_string.Length() < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

        memset_char(m_Data + const_string.Length(), '\0', m_Length - const_string.Length());

        m_Length = const_string.Length();
        CopyMemory(m_Data, const_string.Data(), m_Length);

        return *this;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& operator=(const StaticString<other_capacity, other_aligned_capacity>& other)
    {
        if (this != &other)
        {
            CheckM(other.m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

            memset_char(m_Data + other.m_Length, '\0', m_Length - other.m_Length);

            m_Length = other.m_Length;
            CopyMemory(m_Data, other.m_Data, m_Length);
        }
        return *this;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& operator=(StaticString<other_capacity, other_aligned_capacity>&& other) noexcept
    {
        if (this != &other)
        {
            CheckM(other.m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

            memset_char(m_Data + other.m_Length, '\0', m_Length - other.m_Length);

            m_Length = other.m_Length;
            CopyMemory(m_Data, other.m_Data, m_Length);
        }
        return *this;
    }

    StaticString& operator+=(char right)
    {
        return PushBack(right);
    }

    StaticString& operator+=(const char *right)
    {
        return PushBack(right);
    }

    StaticString& operator+=(const ConstString& right)
    {
        return PushBack(right);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& operator+=(const StaticString<other_capacity, other_aligned_capacity>& right)
    {
        return PushBack(right);
    }

    char operator[](u64 index) const
    {
        CheckM(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
        return m_Data[index];
    }

    char& operator[](u64 index)
    {
        CheckM(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
        return m_Data[index];
    }

private:
    u64  m_Length;
    char m_Data[aligned_capacity];

    template<u64 cap, u64 acap>
    friend class StaticString;
};

template<u64 capacity, u64 aligned_capacity>
StaticString(const StaticString<capacity, aligned_capacity>&) -> StaticString<capacity, aligned_capacity>;

template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() == strlen(right)  && !left.Compare(right); }
template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() == right.Length() && !left.Compare(right); }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Equals(right);                                      }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator==(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  == right.Length() && !right.Compare(left); }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator==(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() == right.Length() && !right.Compare(left); }

template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator!=(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() != strlen(right)  || left.Compare(right); }
template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator!=(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() != right.Length() || left.Compare(right); }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator!=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return !left.Equals(right);                                    }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator!=(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  != right.Length() || right.Compare(left); }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator!=(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() != right.Length() || right.Compare(left); }

template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator<(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() <= strlen(right)  && left.Compare(right) < 0; }
template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator<(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator<(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator<(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  <= right.Length() && right.Compare(left) < 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator<(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() <= right.Length() && right.Compare(left) < 0; }

template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator>(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() >= strlen(right)  && left.Compare(right) > 0; }
template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator>(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator>(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator>(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  >= right.Length() && right.Compare(left) > 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator>(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() >= right.Length() && right.Compare(left) > 0; }

template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator<=(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() <= strlen(right)  && left.Compare(right) <= 0; }
template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator<=(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator<=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator<=(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  <= right.Length() && right.Compare(left) <= 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator<=(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() <= right.Length() && right.Compare(left) <= 0; }

template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator>=(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() >= strlen(right)  && left.Compare(right) >= 0; }
template<u64 l_capacity, u64 l_aligned_capacity>                                         INLINE bool operator>=(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator>=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator>=(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  >= right.Length() && right.Compare(left) >= 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> INLINE bool operator>=(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() >= right.Length() && right.Compare(left) >= 0; }
