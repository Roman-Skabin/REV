//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/const_string.h"

namespace REV
{

ConstString::ConstString()
    : m_Length(0),
      m_Data(null)
{
}

ConstString::ConstString(nullptr_t)
    : m_Length(0),
      m_Data(null)
{
}

ConstString::ConstString(const char *cstring)
    : m_Length(strlen(cstring)),
      m_Data(cstring)
{
}

ConstString::ConstString(const char *cstring, u64 length)
    : m_Length(length),
      m_Data(cstring)
{
}

ConstString::ConstString(const ConstString& other)
    : m_Length(other.m_Length),
      m_Data(other.m_Data)
{
}

ConstString::ConstString(ConstString&& other) noexcept
    : m_Length(other.m_Length),
      m_Data(other.m_Data)
{
    other.m_Length = 0;
    other.m_Data   = null;
}

ConstString::~ConstString()
{
    m_Length = 0;
    m_Data   = null;
}

u64 ConstString::Find(char symbol, u64 offset) const
{
    REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

    const char *_end = m_Data + m_Length;

    for (const char *it = m_Data + offset; it < _end; ++it)
    {
        if (*it == symbol)
        {
            return it - m_Data;
        }
    }

    return npos;
}

u64 ConstString::Find(const char *cstring, u64 offset) const
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

u64 ConstString::Find(const char *cstring, u64 cstring_length, u64 offset) const
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

u64 ConstString::Find(const ConstString& const_string, u64 offset) const
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

u64 ConstString::RFind(char symbol, u64 offset) const
{
    const char *last = m_Data + m_Length - 1;

    for (const char *it = last - offset; it > m_Data; --it)
    {
        if (*it == symbol)
        {
            return it - m_Data;
        }
    }

    return *m_Data == symbol ? 0 : npos;
}

s8 ConstString::Compare(const ConstString& other) const
{
    const char *left  = m_Data;
    const char *right = other.m_Data;

    while (*left++ == *right++)
    {
    }

    if (*left < *right) return -1;
    if (*left > *right) return  1;
    return 0;
}

s8 ConstString::Compare(const char *cstring) const
{
    const char *left  = m_Data;
    const char *right = cstring;

    while (*left++ == *right++)
    {
    }

    if (*left < *right) return -1;
    if (*left > *right) return  1;
    return 0;
}

ConstString& ConstString::operator=(nullptr_t)
{
    m_Length = 0;
    m_Data   = null;
    return *this;
}

ConstString& ConstString::operator=(const char *cstring)
{
    if (m_Data != cstring)
    {
        m_Length = strlen(cstring);
        m_Data   = cstring;
    }
    return *this;
}

ConstString& ConstString::operator=(const ConstString& other)
{
    m_Length = other.m_Length;
    m_Data   = other.m_Data;
    return *this;
}

ConstString& ConstString::operator=(ConstString&& other) noexcept
{
    m_Length       = other.m_Length;
    m_Data         = other.m_Data;
    other.m_Length = 0;
    other.m_Data   = null;
    return *this;
}

char ConstString::operator[](u64 index) const
{
    REV_CHECK_M(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
    return m_Data[index];
}

bool operator==(const ConstString& left, const ConstString& right)
{
    return left.m_Length == right.m_Length
        && !left.Compare(right);
}

bool operator!=(const ConstString& left, const ConstString& right)
{
    return left.m_Length != right.m_Length
        || left.Compare(right);
}

bool operator<=(const ConstString& left, const ConstString& right)
{
    return left.m_Length <= right.m_Length
        && left.Compare(right) <= 0;
}

bool operator>=(const ConstString& left, const ConstString& right)
{
    return left.m_Length >= right.m_Length
        && left.Compare(right) >= 0;
}

bool operator<(const ConstString& left, const ConstString& right)
{
    return left.m_Length <= right.m_Length
        && left.Compare(right) < 0;
}

bool operator>(const ConstString& left, const ConstString& right)
{
    return left.m_Length >= right.m_Length
        && left.Compare(right) > 0;
}

}
