//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"
#include "math/simd.h"

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

    template<u64 count>
    StaticString(const char (&array)[count])
    {
        if constexpr (count)
        {
            m_Length = count - 1;
            CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
            CopyMemory(m_Data, array, m_Length);
            memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
        }
        else
        {
            m_Length = 0;
            memset_char(m_Data, '\0', aligned_capacity);
        }
    }

    StaticString(const char *string, u64 len = npos)
        : m_Length(len != npos ? len : strlen(string))
    {
        CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, string, m_Length);
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

    ~StaticString()
    {
        memset_char(m_Data, '\0', m_Length);
        m_Length = 0;
    }

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

    constexpr const char *Data() const { return m_Data; }
    constexpr       char *Data()       { return m_Data; }

    constexpr u64 Length() const { return m_Length; }

    constexpr bool Empty() const { return !m_Length; }

    void Clear()
    {
        memset_char(m_Data, '\0', m_Length);
        m_Length = 0;
    }

    u64 Find(char what) const
    {
        const char *_begin = m_Data;
        const char *_end   = m_Data + m_Length;

        for (const char *it = _begin; it < _end; ++it)
        {
            if (*it == what)
            {
                return it - _begin;
            }
        }

        return npos;
    }

    u64 RFind(char what) const
    {
        const char *first = m_Data;
        const char *last  = m_Data + m_Length - 1;

        for (const char *it = last; it > first; --it)
        {
            if (*it == what)
            {
                return it - first;
            }
        }

        return *first == what ? 0 : npos;
    }

    template<u64 cap, u64 acap>
    u64 Find(const StaticString<cap, acap>& what) const
    {
        Check(what.m_Length <= m_Length);
        const char *found_str = strstr(m_Data, what.m_Data);
        return found_str ? found_str - m_Data : npos;
    }

    u64 Find(const char *what) const
    {
        Check(strlen(what) <= m_Length);
        const char *found_str = strstr(m_Data, what);
        return found_str ? found_str - m_Data : npos;
    }

    template<u64 count>
    StaticString& operator=(const char (&array)[count])
    {
        if (this != &other)
        {
            if constexpr (count)
            {
                m_Length = count - 1;
                CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
                CopyMemory(m_Data, array, m_Length);
                memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
            }
            else
            {
                m_Length = 0;
                memset_char(m_Data, '\0', aligned_capacity);
            }
        }
        return *this;
    }

    StaticString& operator=(const char *string)
    {
        m_Length = strlen(string);
        CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, string, m_Length);
        memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
        return *this;
    }

    StaticString& operator=(nullptr_t)
    {
        memset_char(m_Data, '\0', m_Length);
        m_Length = 0;
        return *this;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& operator=(const StaticString<other_capacity, other_aligned_capacity>& other)
    {
        if (this != &other)
        {
            CheckM(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
            CopyMemory(m_Data, other.m_Data, m_Length);
            memset_char(m_Data + m_Length, '\0', aligned_capacity - m_Length);
        }
        return *this;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& operator+=(const StaticString<other_capacity, other_aligned_capacity>& other)
    {
        u64 entire_length = m_Length + other.m_Length;
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);
        CopyMemory(m_Data + m_Length, other.m_Data, other.m_Length);
        m_Length = entire_length;
        return *this;
    }

    template<u64 count>
    StaticString& operator+=(const char (&arr)[count])
    {
        u64 entire_length = m_Length + count - 1;
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);
        CopyMemory(m_Data + m_Length, arr, count - 1);
        m_Length = entire_length;
        return *this;
    }

    StaticString& operator+=(const char *cstring)
    {
        u64 cstring_len = strlen(cstring);
        u64 entire_length = m_Length + cstring_len;
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);
        CopyMemory(m_Data + m_Length, cstring, cstring_len);
        m_Length = entire_length;
        return *this;
    }

    StaticString& operator+=(char symbol)
    {
        u64 entire_length = m_Length + 1;
        CheckM(entire_length < aligned_capacity, "Entire length (%I64u) is too big for current static string capacity (%I64u)", entire_length, aligned_capacity);
        m_Data[m_Length] = symbol;
        m_Length = entire_length;
        return *this;
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

    template<u64 l_capacity, u64 r_capacity, u64 l_aligned_capacity, u64 r_aligned_capacity>
    friend bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left,
                           const StaticString<r_capacity, r_aligned_capacity>& right);

    template<u64 l_capacity, u64 r_capacity, u64 l_aligned_capacity, u64 r_aligned_capacity>
    friend bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left,
                           const StaticString<r_capacity, r_aligned_capacity>& right);

private:
    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString(StaticString<other_capacity, other_aligned_capacity>&&) = delete;

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& operator=(StaticString<other_capacity, other_aligned_capacity>&&) = delete;

private:
    u64  m_Length;
    char m_Data[aligned_capacity];

    template<u64 cap, u64 acap>
    friend class StaticString;
};

template<u64 count>
StaticString(const char (&)[count]) -> StaticString<count>;

template<u64 l_capacity, u64 r_capacity, u64 l_aligned_capacity, u64 r_aligned_capacity>
bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left,
                const StaticString<r_capacity, r_aligned_capacity>& right)
{
    if constexpr (l_aligned_capacity == 16 && r_aligned_capacity == 16)
    {
        return left.m_Length == right.m_Length && mm_equals(left.m_Data, right.m_Data);
    }
    else if constexpr (l_aligned_capacity == 32 && r_aligned_capacity == 32)
    {
        return left.m_Length == right.m_Length && mm256_equals(left.m_Data, right.m_Data);
    }
    else if constexpr (l_aligned_capacity == 64 && r_aligned_capacity == 64)
    {
        return left.m_Length == right.m_Length && mm512_equals(left.m_Data, right.m_Data);
    }
    else
    {
        return left.m_Length == right.m_Length && !memcmp(left.m_Data, right.m_Data, left.m_Length);
    }
}

template<u64 l_capacity, u64 r_capacity, u64 l_aligned_capacity, u64 r_aligned_capacity>
bool operator!=(const StaticString<l_capacity, l_aligned_capacity>& left,
                const StaticString<r_capacity, r_aligned_capacity>& right)
{
    if constexpr (l_aligned_capacity == 16 && r_aligned_capacity == 16)
    {
        return left.m_Length != right.m_Length || !mm_equals(left.m_Data, right.m_Data);
    }
    else if constexpr (l_aligned_capacity == 32 && r_aligned_capacity == 32)
    {
        return left.m_Length != right.m_Length || mm256_equals(left.m_Data, right.m_Data);
    }
    else if constexpr (l_aligned_capacity == 64 && r_aligned_capacity == 64)
    {
        return left.m_Length != right.m_Length || mm512_equals(left.m_Data, right.m_Data);
    }
    else
    {
        return left.m_Length != right.m_Length || memcmp(left.m_Data, right.m_Data, left.m_Length);
    }
}
