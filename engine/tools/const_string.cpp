//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/const_string.h"

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

ConstString::ConstString(const char *cstring, u64 len)
    : m_Length(len != npos ? len : strlen(cstring)),
      m_Data(cstring)
{
}

ConstString::ConstString(const ConstString& other)
    : m_Length(other.m_Length),
      m_Data(other.m_Data)
{
}

ConstString::~ConstString()
{
    m_Length = 0;
    m_Data   = null;
}

u64 ConstString::Find(const ConstString& what) const
{
    Check(what.m_Length <= m_Length);
    const char *found_str = strstr(m_Data, what.m_Data);
    return found_str ? found_str - m_Data : npos;
}

u64 ConstString::Find(const char *what) const
{
    Check(strlen(what) <= m_Length);
    const char *found_str = strstr(m_Data, what);
    return found_str ? found_str - m_Data : npos;
}

u64 ConstString::Find(char what) const
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

u64 ConstString::RFind(char what) const
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

s32 ConstString::Compare(const ConstString& other) const
{
    return strcmp(m_Data, other.m_Data);
}

s32 ConstString::Compare(const char *cstring) const
{
    return strcmp(m_Data, cstring);
}

ConstString& ConstString::operator=(const ConstString& other)
{
    m_Length = other.m_Length;
    m_Data   = other.m_Data;
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

ConstString& ConstString::operator=(nullptr_t)
{
    m_Length = 0;
    m_Data   = null;
    return *this;
}

char ConstString::operator[](u64 index) const
{
    CheckM(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
    return m_Data[index];
}

bool operator==(const ConstString& left, const ConstString& right)
{
    return left.m_Length == right.m_Length
        && !memcmp(left.m_Data, right.m_Data, left.m_Length);
}

bool operator!=(const ConstString& left, const ConstString& right)
{
    return left.m_Length != right.m_Length
        || memcmp(left.m_Data, right.m_Data, left.m_Length);
}
