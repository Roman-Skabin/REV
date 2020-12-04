//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"

class ENGINE_API ConstString final
{
public:
    static constexpr u64 npos = U64_MAX;

    ConstString();
    ConstString(nullptr_t);
    ConstString(const char *cstring);
    ConstString(const char *cstring, u64 length);
    ConstString(const ConstString& other);
    ConstString(ConstString&& other) noexcept;

    ~ConstString();

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

    constexpr const ConstString& ToString() const { return *this; }
    constexpr       ConstString& ToString()       { return *this; }

    u64 Find(char symbol, u64 offset = 0) const;
    u64 Find(const char *cstring, u64 offset = 0) const;
    u64 Find(const char *cstring, u64 cstring_length, u64 offset = 0) const;
    u64 Find(const ConstString& const_string, u64 offset = 0) const;

    u64 RFind(char symbol, u64 offset = 0) const;

    s8 Compare(const char *cstring) const;
    s8 Compare(const ConstString& other) const;

    ConstString& operator=(nullptr_t);
    ConstString& operator=(const char *cstring);
    ConstString& operator=(const ConstString& other);
    ConstString& operator=(ConstString&& other) noexcept;

    char operator[](u64 index) const;

    friend bool operator==(const ConstString& left, const ConstString& right);
    friend bool operator!=(const ConstString& left, const ConstString& right);
    friend bool operator<=(const ConstString& left, const ConstString& right);
    friend bool operator>=(const ConstString& left, const ConstString& right);
    friend bool operator< (const ConstString& left, const ConstString& right);
    friend bool operator> (const ConstString& left, const ConstString& right);

private:
    u64         m_Length;
    const char *m_Data;
};
