//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/const_string.h"
#include "tools/string.h"

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
        if (it + cstring_length > _end)
        {
            return npos;
        }

        if (CompareStrings(it, cstring_length, cstring, cstring_length) == COMPARE_RESULT_EQ)
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

}
