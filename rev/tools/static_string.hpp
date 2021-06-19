//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"
#include "math/simd.h"
#include "tools/const_string.h"

namespace REV
{

template<u64 capacity, u64 aligned_capacity = AlignUp(capacity, CACHE_LINE_SIZE)>
class StaticString final
{
public:
    static constexpr u64 npos = REV_U64_MAX;

    REV_INLINE StaticString()
        : m_Length(0)
    {
        memset_char(m_Data, '\0', aligned_capacity);
    }

    REV_INLINE StaticString(nullptr_t)
        : m_Length(0)
    {
        memset_char(m_Data, '\0', aligned_capacity);
    }

    StaticString(char symbol, u64 count = 1)
        : m_Length(count)
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        memset_char(m_Data, symbol, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    StaticString(const char *cstring)
        : m_Length(strlen(cstring))
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, cstring, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    StaticString(const char *cstring, u64 length)
        : m_Length(length)
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, cstring, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    StaticString(const ConstString& const_string)
        : m_Length(const_string.Length())
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, const_string.Data(), m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString(const StaticString<other_capacity, other_aligned_capacity>& other)
        : m_Length(other.m_Length)
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, other.m_Data, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString(StaticString<other_capacity, other_aligned_capacity>&& other) noexcept
        : m_Length(other.m_Length)
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, other.m_Data, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
    }

    REV_INLINE ~StaticString()
    {
    }

    REV_INLINE u64 Length() const { return m_Length; }

    REV_INLINE const char *Data() const { return m_Data; }
    REV_INLINE       char *Data()       { return m_Data; }

    REV_INLINE const StaticString& ToString() const { return *this; }
    REV_INLINE       StaticString& ToString()       { return *this; }

    REV_INLINE bool Empty() const { return !m_Length; }

    REV_INLINE const char *begin()   const { return m_Data;            }
    REV_INLINE const char *cbegin()  const { return m_Data;            }
    REV_INLINE const char *rbegin()  const { return m_Data + m_Length; }
    REV_INLINE const char *crbegin() const { return m_Data + m_Length; }

    REV_INLINE const char *end()   const { return m_Data + m_Length; }
    REV_INLINE const char *cend()  const { return m_Data + m_Length; }
    REV_INLINE const char *rend()  const { return m_Data;            }
    REV_INLINE const char *crend() const { return m_Data;            }

    REV_INLINE char *begin()  { return m_Data;            }
    REV_INLINE char *rbegin() { return m_Data + m_Length; }

    REV_INLINE char *end()  { return m_Data + m_Length; }
    REV_INLINE char *rend() { return m_Data;            }

    REV_INLINE char First() const { return *m_Data;              }
    REV_INLINE char Last()  const { return m_Data[m_Length - 1]; }

    REV_INLINE char& First() { return *m_Data;              }
    REV_INLINE char& Last()  { return m_Data[m_Length - 1]; }

    REV_INLINE void Clear()
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

    REV_INLINE bool Equals(const char *cstring) const
    {
        return m_Length == strlen(cstring) && !Compare(cstring);
    }

    REV_INLINE bool Equals(const char *cstring, u64 length) const
    {
        return m_Length == length && !Compare(cstring);
    }

    REV_INLINE bool Equals(const ConstString& const_string) const
    {
        return m_Length == const_string.Length() && !Compare(const_string);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE bool Equals(const StaticString<other_capacity, other_aligned_capacity>& static_string) const
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
        REV_CHECK_M(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

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
        REV_CHECK_M(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

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
        REV_CHECK_M(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

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
        REV_CHECK_M(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

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
        REV_CHECK_M(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);

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
        REV_CHECK_M(from < m_Length && from < to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);

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
        REV_CHECK_M(from < m_Length && from <= to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);
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

    REV_INLINE StaticString& Replace(u64 from, u64 to, char symbol, u64 count = 1)
    {
        Erase(from, to);
        Insert(from, symbol, count);
        return *this;
    }

    REV_INLINE StaticString& Replace(u64 from, u64 to, const char *cstring)
    {
        Erase(from, to);
        Insert(from, cstring);
        return *this;
    }

    REV_INLINE StaticString& Replace(u64 from, u64 to, const char *cstring, u64 length)
    {
        Erase(from, to);
        Insert(from, cstring, length);
        return *this;
    }

    REV_INLINE StaticString& Replace(u64 from, u64 to, const ConstString& const_string)
    {
        Erase(from, to);
        Insert(from, const_string);
        return *this;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString& Replace(u64 from, u64 to, const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        Erase(from, to);
        Insert(from, static_string);
        return *this;
    }

    REV_INLINE StaticString& PushFront(char symbol, u64 count = 1)
    {
        return Insert(0, symbol, count);
    }

    REV_INLINE StaticString& PushFront(const char *cstring)
    {
        return Insert(0, cstring);
    }

    REV_INLINE StaticString& PushFront(const char *cstring, u64 length)
    {
        return Insert(0, cstring, length);
    }

    REV_INLINE StaticString& PushFront(const ConstString& const_string)
    {
        return Insert(0, const_string);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString& PushFront(const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        return Insert(0, static_string);
    }

    REV_INLINE StaticString& PushBack(char symbol, u64 count = 1)
    {
        return Insert(m_Length, symbol, count);
    }

    REV_INLINE StaticString& PushBack(const char *cstring)
    {
        return Insert(m_Length, cstring);
    }

    REV_INLINE StaticString& PushBack(const char *cstring, u64 length)
    {
        return Insert(m_Length, cstring, length);
    }

    REV_INLINE StaticString& PushBack(const ConstString& const_string)
    {
        return Insert(m_Length, const_string);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString& PushBack(const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        return Insert(m_Length, static_string);
    }

    REV_INLINE StaticString SubString(u64 from) const
    {
        REV_CHECK_M(from <= m_Length, "Bad arguments: from = %I64u, length = %I64u", from, m_Length);
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

    REV_INLINE StaticString SubString(u64 from, u64 to) const
    {
        REV_CHECK_M(from <= to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);
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
        REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

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
        REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

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
        REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

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
        REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

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
        REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

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
        REV_CHECK_M(roffset < m_Length, "Offset out of bounds.");

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

    REV_INLINE StaticString& operator=(nullptr_t)
    {
        memset_char(m_Data, '\0', m_Length);
        m_Length = 0;
        return *this;
    }

    REV_INLINE StaticString& operator=(char symbol)
    {
        if (m_Length > 1)
        {
            memset_char(m_Data + 1, '\0', m_Length - 1);
        }

        m_Length = 1;
        *m_Data  = symbol;

        return *this;
    }

    StaticString& operator=(const char *cstring)
    {
        u64 length = strlen(cstring);
        REV_CHECK_M(length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

        if (m_Length > length)
        {
            memset_char(m_Data + length, '\0', m_Length - length);
        }

        m_Length = length;
        CopyMemory(m_Data, cstring, m_Length);

        return *this;
    }

    StaticString& operator=(const ConstString& const_string)
    {
        REV_CHECK_M(const_string.Length() < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

        if (m_Length > const_string.Length())
        {
            memset_char(m_Data + const_string.Length(), '\0', m_Length - const_string.Length());
        }

        m_Length = const_string.Length();
        CopyMemory(m_Data, const_string.Data(), m_Length);

        return *this;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& operator=(const StaticString<other_capacity, other_aligned_capacity>& other)
    {
        if (this != &other)
        {
            REV_CHECK_M(other.m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

            if (m_Length > other.Length())
            {
                memset_char(m_Data + other.m_Length, '\0', m_Length - other.m_Length);
            }

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
            REV_CHECK_M(other.m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

            if (m_Length > other.m_Length)
            {
                memset_char(m_Data + other.m_Length, '\0', m_Length - other.m_Length);
            }

            m_Length = other.m_Length;
            CopyMemory(m_Data, other.m_Data, m_Length);
        }
        return *this;
    }

    REV_INLINE StaticString& operator+=(char right)
    {
        return PushBack(right);
    }

    REV_INLINE StaticString& operator+=(const char *right)
    {
        return PushBack(right);
    }

    REV_INLINE StaticString& operator+=(const ConstString& right)
    {
        return PushBack(right);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString& operator+=(const StaticString<other_capacity, other_aligned_capacity>& right)
    {
        return PushBack(right);
    }

    REV_INLINE char operator[](u64 index) const
    {
        REV_CHECK_M(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
        return m_Data[index];
    }

    REV_INLINE char& operator[](u64 index)
    {
        REV_CHECK_M(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
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

template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() == strlen(right)  && !left.Compare(right); }
template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() == right.Length() && !left.Compare(right); }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Equals(right);                                      }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator==(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  == right.Length() && !right.Compare(left); }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator==(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() == right.Length() && !right.Compare(left); }

template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator!=(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() != strlen(right)  || left.Compare(right); }
template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator!=(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() != right.Length() || left.Compare(right); }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator!=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return !left.Equals(right);                                    }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator!=(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  != right.Length() || right.Compare(left); }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator!=(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() != right.Length() || right.Compare(left); }

template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator<(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() <= strlen(right)  && left.Compare(right) < 0; }
template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator<(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator<(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() <= right.Length() && left.Compare(right) < 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator<(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  <= right.Length() && right.Compare(left) < 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator<(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() <= right.Length() && right.Compare(left) < 0; }

template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator>(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() >= strlen(right)  && left.Compare(right) > 0; }
template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator>(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator>(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() >= right.Length() && left.Compare(right) > 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator>(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  >= right.Length() && right.Compare(left) > 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator>(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() >= right.Length() && right.Compare(left) > 0; }

template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator<=(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() <= strlen(right)  && left.Compare(right) <= 0; }
template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator<=(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator<=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() <= right.Length() && left.Compare(right) <= 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator<=(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  <= right.Length() && right.Compare(left) <= 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator<=(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() <= right.Length() && right.Compare(left) <= 0; }

template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator>=(const StaticString<l_capacity, l_aligned_capacity>& left, const char                                         *right) { return left.Length() >= strlen(right)  && left.Compare(right) >= 0; }
template<u64 l_capacity, u64 l_aligned_capacity>                                         REV_INLINE bool operator>=(const StaticString<l_capacity, l_aligned_capacity>& left, const ConstString&                                  right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator>=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() >= right.Length() && left.Compare(right) >= 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator>=(const char                                         *left, const StaticString<r_capacity, r_aligned_capacity>& right) { return strlen(left)  >= right.Length() && right.Compare(left) >= 0; }
template<                                        u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator>=(const ConstString&                                  left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Length() >= right.Length() && right.Compare(left) >= 0; }

}
