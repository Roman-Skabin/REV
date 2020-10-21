//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

class ConstString final
{
public:
    static inline constexpr u64 npos = U64_MAX;

    ConstString()
        : m_Length(0),
          m_Data(null)
    {
    }

    ConstString(nullptr_t)
        : m_Length(0),
          m_Data(null)
    {
    }

    template<u64 count>
    ConstString(const char (&arr)[count])
        : m_Length(count - 1),
          m_Data(arr)
    {
    }

    ConstString(const char *cstring, u64 len = 0)
        : m_Length(len ? len : strlen(cstring)),
          m_Data(cstring)
    {
    }

    ConstString(const ConstString& other)
        : m_Length(other.m_Length),
          m_Data(other.m_Data)
    {
    }

    ~ConstString()
    {
        m_Length = 0;
        m_Data   = null;
    }

    constexpr const char *begin()   const { return m_Data;            }
    constexpr const char *cbegin()  const { return m_Data;            }
    constexpr const char *rbegin()  const { return m_Data + m_Length; }
    constexpr const char *crbegin() const { return m_Data + m_Length; }

    constexpr const char *end()   const { return m_Data + m_Length; }
    constexpr const char *cend()  const { return m_Data + m_Length; }
    constexpr const char *rend()  const { return m_Data;            }
    constexpr const char *crend() const { return m_Data;            }

    constexpr const char *Data()   const { return  m_Data;   }
    constexpr u64         Length() const { return  m_Length; }
    constexpr bool        Empty()  const { return !m_Length; }

    constexpr operator const char *() const { return m_Data; }

    u64 Find(const ConstString& what) const
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

    s32 Compare(const ConstString& other) const
    {
        return strcmp(m_Data, other.m_Data);
    }

    s32 Compare(const char *cstring) const
    {
        return strcmp(m_Data, cstring);
    }

    ConstString& operator=(const ConstString& other)
    {
        m_Length = other.m_Length;
        m_Data   = other.m_Data;
        return *this;
    }

    template<u64 count>
    ConstString& operator=(const char (&arr)[count])
    {
        m_Length = count - 1;
        m_Data   = arr;
        return *this;
    }

    ConstString& operator=(const char *cstring)
    {
        if (m_Data != cstring)
        {
            m_Length = strlen(cstring);
            m_Data   = cstring;
        }
        return *this;
    }

    ConstString& operator=(nullptr_t)
    {
        m_Length = 0;
        m_Data   = null;
        return *this;
    }

    char operator[](u64 index) const
    {
        CheckM(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
        return m_Data[index];
    }

    friend bool operator==(const ConstString& left, const ConstString& right)
    {
        return left.m_Length == right.m_Length
            && !memcmp(left.m_Data, right.m_Data, left.m_Length);
    }

    friend bool operator!=(const ConstString& left, const ConstString& right)
    {
        return left.m_Length != right.m_Length
            || memcmp(left.m_Data, right.m_Data, left.m_Length);
    }

private:
    ConstString(ConstString&&)            = delete;
    ConstString& operator=(ConstString&&) = delete;

private:
    u64         m_Length;
    const char *m_Data;
};
