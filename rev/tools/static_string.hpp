//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/simd.h"
#include "tools/const_string.h"

namespace REV
{

template<u64 capacity, u64 aligned_capacity = AlignUp(capacity, CACHE_LINE_SIZE)>
class StaticString final
{
public:
    static_assert(aligned_capacity && aligned_capacity % 16 == 0, "StaticString capacity must be multiple of 16");

    static constexpr u64 npos = REV_U64_MAX;

    REV_INLINE StaticString()
        : m_Length(0)
    {
        ZeroMemory(m_Data, aligned_capacity);
    }

    REV_INLINE StaticString(nullptr_t)
        : m_Length(0)
    {
        ZeroMemory(m_Data, aligned_capacity);
    }

    REV_INLINE StaticString(char symbol, u64 count = 1)
        : m_Length(count)
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        FillMemoryChar(m_Data, symbol, m_Length);
        ZeroMemory(m_Data + m_Length, aligned_capacity - m_Length);
    }

    REV_INLINE StaticString(const char *cstring, u64 length)
        : m_Length(length)
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, cstring, m_Length);
        ZeroMemory(m_Data + m_Length, aligned_capacity - m_Length);
    }

    REV_INLINE StaticString(const ConstString& const_string)
        : m_Length(const_string.Length())
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, const_string.Data(), m_Length);
        ZeroMemory(m_Data + m_Length, aligned_capacity - m_Length);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString(const StaticString<other_capacity, other_aligned_capacity>& other)
        : m_Length(other.m_Length)
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, other.m_Data, m_Length);
        ZeroMemory(m_Data + m_Length, aligned_capacity - m_Length);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString(StaticString<other_capacity, other_aligned_capacity>&& other) noexcept
        : m_Length(other.m_Length)
    {
        REV_CHECK_M(m_Length < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);
        CopyMemory(m_Data, other.m_Data, m_Length);
        ZeroMemory(m_Data + m_Length, aligned_capacity - m_Length);
    }

    REV_INLINE ~StaticString()
    {
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

        FillMemoryChar(m_Data + where, symbol, count);
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
        return Insert(where, const_string.Data(), const_string.Length());
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    StaticString& Insert(u64 where, const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        return Insert(where, static_string.Data(), static_string.Length());
    }

    REV_INLINE StaticString& PushFront(char symbol, u64 count = 1)      { return Insert(0, symbol, count);   }
    REV_INLINE StaticString& PushFront(const char *cstring, u64 length) { return Insert(0, cstring, length); }
    REV_INLINE StaticString& PushFront(const ConstString& const_string) { return Insert(0, const_string);    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString& PushFront(const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        return Insert(0, static_string);
    }

    REV_INLINE StaticString& PushBack(char symbol, u64 count = 1)      { return Insert(m_Length, symbol, count);   }
    REV_INLINE StaticString& PushBack(const char *cstring, u64 length) { return Insert(m_Length, cstring, length); }
    REV_INLINE StaticString& PushBack(const ConstString& const_string) { return Insert(m_Length, const_string);    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString& PushBack(const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        return Insert(m_Length, static_string);
    }

    StaticString& Erase(u64 from, u64 to = npos)
    {
        if (to == npos) to = from + 1;
        REV_CHECK_M(from < m_Length && from < to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);

        if (from < m_Length - 1 || to < m_Length)
        {
            MoveMemory(m_Data + from, m_Data + to, m_Length - to);
        }

        u64 delta = to - from;
        ZeroMemory(m_Data + m_Length - delta, delta);

        m_Length -= delta;
        return *this;
    }

    REV_INLINE StaticString& EraseFront() { return Erase(0);            }
    REV_INLINE StaticString& EraseBack()  { return Erase(m_Length - 1); }

    REV_INLINE StaticString& Clear()
    {
        ZeroMemory(m_Data, m_Length);
        m_Length = 0;
        return *this;
    }

    StaticString& Replace(u64 from, u64 to, char symbol, u64 count = 1)
    {
        if (to == npos) to = from + 1;
        REV_CHECK_M(from < m_Length && from < to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);

        u64 delta = to - from;
        FillMemoryChar(m_Data + from, symbol, delta);

        if (count > delta)
        {
            Insert(to, symbol, count - delta);
        }

        return *this;
    }

    StaticString& Replace(u64 from, u64 to, const char *cstring, u64 cstring_length)
    {
        if (to == npos) to = from + 1;
        REV_CHECK_M(from < m_Length && from < to && to <= m_Length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Length);

        u64 delta = to - from;
        CopyMemory(m_Data + from, cstring, delta);

        if (cstring_length > delta)
        {
            Insert(to, cstring + delta, cstring_length - delta);
        }

        return *this;
    }

    REV_INLINE StaticString& Replace(u64 from, u64 to, const ConstString& const_string)
    {
        return Replace(from, to, const_string.Data(), const_string.Length());
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString& Replace(u64 from, u64 to, const StaticString<other_capacity, other_aligned_capacity>& static_string)
    {
        return Replace(from, to, static_string.Data(), static_string.Length());
    }

    void Reverse()
    {
        char *first = m_Data;
        char *last  = pLast();

        while (first < last)
        {
            char temp = *first;
            *first++  = *last;
            *last--   = temp;
        }
    }

    StaticString SubString(u64 from) const
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

    StaticString SubString(u64 from, u64 to) const
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

        char *_end = m_Data + m_Length;

        for (const char *it = m_Data + offset; it < _end; ++it)
        {
            if (*it == what)
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

    REV_INLINE u64 Find(const ConstString& const_string, u64 offset = 0) const
    {
        return Find(const_string.Data(), const_string.Length(), offset);
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE u64 Find(const StaticString<other_capacity, other_aligned_capacity>& static_string, u64 offset = 0) const
    {
        return Find(static_string.Data(), static_string.Length(), offset);
    }

    u64 RFind(char what, u64 offset = 0) const
    {
        REV_CHECK_M(offset < m_Length, "Offset out of bounds.");

        for (const char *it = pLast() - offset; it >= m_Data; --it)
        {
            if (*it == what)
            {
                return it - m_Data;
            }
        }

        return npos;
    }

    REV_INLINE COMPARE_RESULT Compare(const char *cstring, u64 cstring_length) const
    {
        return CompareStrings(m_Data, m_Length, cstring, cstring_length);
    }

    REV_INLINE COMPARE_RESULT Compare(const ConstString& const_string) const
    {
        return CompareStrings(m_Data, m_Length, const_string.Data(), const_string.Length());
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE COMPARE_RESULT Compare(const StaticString<other_capacity, other_aligned_capacity>& static_string) const
    {
        return CompareStringsAligned(m_Data, AlignUp(m_Length, 16), static_string.m_Data, AlignUp(static_string.m_Length, 16));
    }

    REV_INLINE bool Equals(const char *cstring, u64 cstring_length) const
    {
        return CompareStrings(m_Data, m_Length, cstring, cstring_length) == COMPARE_RESULT_EQ;
    }

    REV_INLINE bool Equals(const ConstString& const_string) const
    {
        return CompareStrings(m_Data, m_Length, const_string.Data(), const_string.Length()) == COMPARE_RESULT_EQ;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE bool Equals(const StaticString<other_capacity, other_aligned_capacity>& static_string) const
    {
        if constexpr (aligned_capacity == 16 && other_aligned_capacity == 16)
        {
            return Math::mm_equals(m_Data, static_string.m_Data);
        }
        else if constexpr (aligned_capacity == 32 && other_aligned_capacity == 32)
        {
            return Math::mm256_equals(m_Data, static_string.m_Data);
        }
        else if constexpr (aligned_capacity == 64 && other_aligned_capacity == 64)
        {
            return Math::mm512_equals(m_Data, static_string.m_Data);
        }
        else
        {
            return CompareStringsAligned(m_Data, AlignUp(m_Length, 16), static_string.m_Data, AlignUp(static_string.m_Length, 16)) == COMPARE_RESULT_EQ;
        }
    }

    REV_INLINE bool NotEquals(const char *cstring, u64 cstring_length) const
    {
        return CompareStrings(m_Data, m_Length, cstring, cstring_length) != COMPARE_RESULT_EQ;
    }

    REV_INLINE bool NotEquals(const ConstString& const_string) const
    {
        return CompareStrings(m_Data, m_Length, const_string.Data(), const_string.Length()) != COMPARE_RESULT_EQ;
    }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE bool NotEquals(const StaticString<other_capacity, other_aligned_capacity>& static_string) const
    {
        if constexpr (aligned_capacity == 16 && other_aligned_capacity == 16)
        {
            return Math::mm_not_equals(m_Data, static_string.m_Data);
        }
        else if constexpr (aligned_capacity == 32 && other_aligned_capacity == 32)
        {
            return Math::mm256_not_equals(m_Data, static_string.m_Data);
        }
        else if constexpr (aligned_capacity == 64 && other_aligned_capacity == 64)
        {
            return Math::mm512_not_equals(m_Data, static_string.m_Data);
        }
        else
        {
            return CompareStringsAligned(m_Data, AlignUp(m_Length, 16), static_string.m_Data, AlignUp(static_string.m_Length, 16)) != COMPARE_RESULT_EQ;
        }
    }

    REV_INLINE const char *Data() const { return m_Data; }
    REV_INLINE       char *Data()       { return m_Data; }

    REV_INLINE u64 Length()   const { return m_Length; }
    REV_INLINE u64 Capacity() const { return aligned_capacity; }

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

    REV_INLINE const StaticString& ToString() const { return *this; }
    REV_INLINE       StaticString& ToString()       { return *this; }

    REV_INLINE StaticString& operator=(nullptr_t)
    {
        ZeroMemory(m_Data, m_Length);
        m_Length = 0;
        return *this;
    }

    REV_INLINE StaticString& operator=(char symbol)
    {
        if (m_Length > 1)
        {
            ZeroMemory(m_Data + 1, m_Length - 1);
        }

        m_Length = 1;
        *m_Data  = symbol;

        return *this;
    }

    StaticString& operator=(const ConstString& const_string)
    {
        REV_CHECK_M(const_string.Length() < aligned_capacity, "Length is to high, max allowed length is %I64u", aligned_capacity);

        if (m_Length > const_string.Length())
        {
            ZeroMemory(m_Data + const_string.Length(), m_Length - const_string.Length());
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

            if (m_Length > other.m_Length)
            {
                ZeroMemory(m_Data + other.m_Length, m_Length - other.m_Length);
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
                ZeroMemory(m_Data + other.m_Length, m_Length - other.m_Length);
            }

            m_Length = other.m_Length;
            CopyMemory(m_Data, other.m_Data, m_Length);
        }
        return *this;
    }

    REV_INLINE StaticString& operator+=(char right)               { return PushBack(right); }
    REV_INLINE StaticString& operator+=(const ConstString& right) { return PushBack(right); }

    template<u64 other_capacity, u64 other_aligned_capacity>
    REV_INLINE StaticString& operator+=(const StaticString<other_capacity, other_aligned_capacity>& right)
    {
        return PushBack(right);
    }

    REV_INLINE char operator[](u64 index) const
    {
        REV_CHECK_M(m_Length, "StaticString is empty");
        REV_CHECK_M(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
        return m_Data[index];
    }

    REV_INLINE char& operator[](u64 index)
    {
        REV_CHECK_M(m_Length, "StaticString is empty");
        REV_CHECK_M(index < m_Length, "Index of bound, max allowed is %I64u", m_Length - 1);
        return m_Data[index];
    }

private:
    u64  m_Length;
    char m_Data[aligned_capacity];

    template<u64, u64> friend class StaticString;
    template<u64, u64> friend class StaticStringBuilder;
};

template<u64 capacity, u64 aligned_capacity> StaticString(const StaticString<capacity, aligned_capacity>&) -> StaticString<capacity, aligned_capacity>;
template<u64 capacity, u64 aligned_capacity> StaticString(StaticString<capacity, aligned_capacity>&&)      -> StaticString<capacity, aligned_capacity>;

template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator==(const StaticString<capacity, aligned_capacity>& left, const ConstString& right) { return !left.Compare(right);      }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator!=(const StaticString<capacity, aligned_capacity>& left, const ConstString& right) { return  left.Compare(right);      }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator< (const StaticString<capacity, aligned_capacity>& left, const ConstString& right) { return  left.Compare(right) <  0; }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator> (const StaticString<capacity, aligned_capacity>& left, const ConstString& right) { return  left.Compare(right) >  0; }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator<=(const StaticString<capacity, aligned_capacity>& left, const ConstString& right) { return  left.Compare(right) <= 0; }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator>=(const StaticString<capacity, aligned_capacity>& left, const ConstString& right) { return  left.Compare(right) >= 0; }

template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator==(const ConstString& left, const StaticString<capacity, aligned_capacity>& right) { return !right.Compare(left);      }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator!=(const ConstString& left, const StaticString<capacity, aligned_capacity>& right) { return  right.Compare(left);      }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator< (const ConstString& left, const StaticString<capacity, aligned_capacity>& right) { return  right.Compare(left) >  0; }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator> (const ConstString& left, const StaticString<capacity, aligned_capacity>& right) { return  right.Compare(left) <  0; }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator<=(const ConstString& left, const StaticString<capacity, aligned_capacity>& right) { return  right.Compare(left) >= 0; }
template<u64 capacity, u64 aligned_capacity> REV_INLINE bool operator>=(const ConstString& left, const StaticString<capacity, aligned_capacity>& right) { return  right.Compare(left) <= 0; }

template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator==(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Equals(right);       }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator!=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.NotEquals(right);    }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator< (const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Compare(right) <  0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator> (const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Compare(right) >  0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator<=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Compare(right) <= 0; }
template<u64 l_capacity, u64 l_aligned_capacity, u64 r_capacity, u64 r_aligned_capacity> REV_INLINE bool operator>=(const StaticString<l_capacity, l_aligned_capacity>& left, const StaticString<r_capacity, r_aligned_capacity>& right) { return left.Compare(right) >= 0; }

}
