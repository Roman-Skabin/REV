//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/const_string.h"

namespace REV
{

ConstString ConstString::SubString(u64 from) const
{
    REV_CHECK_M(from <= m_Length, "Bad arguments: from = %I64u, length = %I64u", from, m_Length);
    if (from == 0)
    {
        return *this;
    }
    else if (from == m_Length)
    {
        return ConstString();
    }
    else
    {
        return ConstString(m_Data + from, m_Length - from);
    }
}

ConstString ConstString::SubString(u64 from, u64 to) const
{
    REV_CHECK_M(from <= to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);
    if (from == 0 && to == m_Length)
    {
        return *this;
    }
    else if (from == to)
    {
        return ConstString();
    }
    else
    {
        return ConstString(m_Data + from, to - from);
    }
}

u64 ConstString::Find(char symbol, u64 offset) const
{
    REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

    char *_end = m_Data + m_Length;

    for (char *it = m_Data + offset; it < _end; ++it)
    {
        if (*it == symbol)
        {
            return it - m_Data;
        }
    }

    return npos;
}

u64 ConstString::Find(const char *cstring, u64 cstring_length, u64 offset) const
{
    REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

    const char *_end = m_Data + m_Length;

    for (const char *it = m_Data + offset; it < _end; ++it)
    {
        const char *sub_end = it + cstring_length;

        if (sub_end > _end)
        {
            return npos;
        }

        const char *sub         = it;
        const char *cstr        = cstring;
        u64         cstr_length = cstring_length;

        while (cstr_length-- && *sub++ == *cstr++)
        {
        }

        if (sub == sub_end)
        {
            return it - m_Data;
        }
    }

    return npos;
}

u64 ConstString::Find(const ConstString& const_string, u64 offset) const
{
    REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

    const char *_end = m_Data + m_Length;

    for (const char *it = m_Data + offset; it < _end; ++it)
    {
        const char *sub_end = it + const_string.m_Length;

        if (sub_end > _end)
        {
            return npos;
        }

        const char *sub         = it;
        const char *cstr        = const_string.m_Data;
        u64         cstr_length = const_string.m_Length;

        while (cstr_length-- && *sub++ == *cstr++)
        {
        }

        if (sub == sub_end)
        {
            return it - m_Data;
        }
    }

    return npos;
}

u64 ConstString::RFind(char symbol, u64 offset) const
{
    REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

    const char *last = m_Data + m_Length - 1;

    for (const char *it = last - offset; it >= m_Data; --it)
    {
        if (*it == symbol)
        {
            return it - m_Data;
        }
    }

    return npos;
}

s8 ConstString::Compare(const char *cstring, u64 cstring_length) const
{
    if (m_Length < cstring_length) return -1;
    if (m_Length > cstring_length) return  1;

    const char *left  = m_Data;
    const char *right = cstring;

    while (cstring_length-- && *left++ == *right++)
    {
    }

    if (*left < *right) return -1;
    if (*left > *right) return  1;
    return 0;
}

s8 ConstString::Compare(const ConstString& other) const
{
    if (m_Length < other.m_Length) return -1;
    if (m_Length > other.m_Length) return  1;

    const char *left   = m_Data;
    const char *right  = other.m_Data;
    u64         length = m_Length;

    while (length-- && *left++ == *right++)
    {
    }

    if (*left < *right) return -1;
    if (*left > *right) return  1;

    return 0;
}

}
