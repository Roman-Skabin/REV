//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

class ENGINE_API ConstString final
{
public:
    static constexpr u64 npos = U64_MAX;

    ConstString();
    ConstString(nullptr_t);
    ConstString(const char *cstring, u64 len = npos);
    ConstString(const ConstString& other);

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

    constexpr const ConstString& ToConstString() const { return *this; }
    constexpr       ConstString& ToConstString()       { return *this; }

    u64 Find(const ConstString& what) const;
    u64 Find(const char *what) const;
    u64 Find(char what) const;

    u64 RFind(char what) const;

    s32 Compare(const ConstString& other) const;
    s32 Compare(const char *cstring) const;

    ConstString& operator=(nullptr_t);
    ConstString& operator=(const char *cstring);
    ConstString& operator=(const ConstString& other);

    char operator[](u64 index) const;

    friend bool operator==(const ConstString& left, const ConstString& right);
    friend bool operator!=(const ConstString& left, const ConstString& right);

private:
    ConstString(ConstString&&)            = delete;
    ConstString& operator=(ConstString&&) = delete;

private:
    u64         m_Length;
    const char *m_Data;
};
