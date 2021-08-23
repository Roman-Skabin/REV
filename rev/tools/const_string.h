//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "memory/memlow.h"

namespace REV
{
    class REV_API ConstString final
    {
    public:
        static constexpr u64 npos = REV_U64_MAX;

        REV_INLINE ConstString()                                : m_Length(0),              m_Data(null)                  {}
        REV_INLINE ConstString(nullptr_t)                       : m_Length(0),              m_Data(null)                  {}
        REV_INLINE ConstString(const char *cstring, u64 length) : m_Length(length),         m_Data(cast<char *>(cstring)) {}
        REV_INLINE ConstString(const ConstString& other)        : m_Length(other.m_Length), m_Data(other.m_Data)          {}
        REV_INLINE ConstString(ConstString&& other) noexcept    : m_Length(other.m_Length), m_Data(other.m_Data)          { other.m_Length = 0; other.m_Data = null; }

        REV_INLINE ~ConstString() { m_Length = 0; m_Data = null; }

        ConstString SubString(u64 from) const;
        ConstString SubString(u64 from, u64 to) const;

                   u64 Find(char symbol, u64 offset = 0)                             const;
                   u64 Find(const char *cstring, u64 cstring_length, u64 offset = 0) const;
        REV_INLINE u64 Find(const ConstString& const_string, u64 offset = 0)         const { return Find(const_string.m_Data, const_string.m_Length, offset); }

        u64 RFind(char symbol, u64 offset = 0) const;

        REV_INLINE COMPARE_RESULT Compare(const char *cstring, u64 cstring_length) const { return CompareStrings(m_Data, m_Length, cstring,      cstring_length); }
        REV_INLINE COMPARE_RESULT Compare(const ConstString& other)                const { return CompareStrings(m_Data, m_Length, other.m_Data, other.m_Length); }

        REV_INLINE const char *Data()   const { return  m_Data;   }
        REV_INLINE u64         Length() const { return  m_Length; }
        REV_INLINE bool        Empty()  const { return !m_Length; }

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

        REV_INLINE char First() const { return *m_Data;                                   }
        REV_INLINE char Last()  const { return m_Length ? m_Data[m_Length - 1] : *m_Data; }

        REV_INLINE char& First() { return *m_Data;                                   }
        REV_INLINE char& Last()  { return m_Length ? m_Data[m_Length - 1] : *m_Data; }

        REV_INLINE const char *pFirst() const { return m_Data;                                    }
        REV_INLINE const char *pLast()  const { return m_Length ? m_Data + m_Length - 1 : m_Data; }

        REV_INLINE char *pFirst() { return m_Data;                                    }
        REV_INLINE char *pLast()  { return m_Length ? m_Data + m_Length - 1 : m_Data; }

        REV_INLINE const char *GetPointer(u64 index) const
        {
            REV_CHECK_M(m_Length, "ConstString is empty");
            REV_CHECK_M(index < m_Length, "Expected max index: %I64u, got: %I64u", m_Length - 1, index);
            return m_Data + index;
        }

        REV_INLINE char *GetPointer(u64 index)
        {
            REV_CHECK_M(m_Length, "ConstString is empty");
            REV_CHECK_M(index < m_Length, "Expected max index: %I64u, got: %I64u", m_Length - 1, index);
            return m_Data + index;
        }

        REV_INLINE const ConstString& ToString() const { return *this; }
        REV_INLINE       ConstString& ToString()       { return *this; }

        REV_INLINE ConstString& operator=(nullptr_t)
        {
            m_Length = 0;
            m_Data   = null;
            return *this;
        }

        REV_INLINE ConstString& operator=(const ConstString& other)
        {
            if (this != &other)
            {
                m_Length = other.m_Length;
                m_Data   = other.m_Data;
            }
            return *this;
        }

        REV_INLINE ConstString& operator=(ConstString&& other) noexcept
        {
            if (this != &other)
            {
                m_Length       = other.m_Length;
                m_Data         = other.m_Data;
                other.m_Length = 0;
                other.m_Data   = null;
            }
            return *this;
        }

        REV_INLINE char operator[](u64 index) const
        {
            REV_CHECK_M(m_Length, "ConstString is empty");
            REV_CHECK_M(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
            return m_Data[index];
        }

        REV_INLINE char& operator[](u64 index)
        {
            REV_CHECK_M(m_Length, "ConstString is empty");
            REV_CHECK_M(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
            return m_Data[index];
        }

    private:
        u64   m_Length;
        char *m_Data;
    };

    REV_INLINE bool operator==(const ConstString& left, const ConstString& right) { return !left.Compare(right);      }
    REV_INLINE bool operator!=(const ConstString& left, const ConstString& right) { return  left.Compare(right);      }
    REV_INLINE bool operator<=(const ConstString& left, const ConstString& right) { return  left.Compare(right) <= 0; }
    REV_INLINE bool operator>=(const ConstString& left, const ConstString& right) { return  left.Compare(right) >= 0; }
    REV_INLINE bool operator< (const ConstString& left, const ConstString& right) { return  left.Compare(right) <  0; }
    REV_INLINE bool operator> (const ConstString& left, const ConstString& right) { return  left.Compare(right) >  0; }
}
