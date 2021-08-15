//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "core/common.h"

namespace REV
{
    template<typename T>
    class ConstArray final
    {
    public:
        using Type = T;
        static constexpr const u64 npos = REV_U64_MAX;

        REV_INLINE ConstArray()                        : m_Data(null),         m_Count(0)             {}
        REV_INLINE ConstArray(nullptr_t)               : m_Data(null),         m_Count(0)             {}
        REV_INLINE ConstArray(T *data, u64 count)      : m_Data(data),         m_Count(count)         {}
        REV_INLINE ConstArray(const ConstArray& other) : m_Data(other.m_Data), m_Count(other.m_Count) {}
        REV_INLINE ConstArray(ConstArray&& other)      : m_Data(other.m_Data), m_Count(other.m_Count) { other.m_Data = null; other.m_Count = 0; }

        REV_INLINE ~ConstArray() { m_Data = null; m_Count = 0; }

        u64 Find(const T& what) const
        {
            T *_begin = m_Data;
            T *_end   = m_Data + m_Count;

            for (T *it = _begin; it < _end; ++it)
            {
                if (*it == what)
                {
                    return it - _begin;
                }
            }

            return npos;
        }

        u64 RFind(const T& what) const
        {
            T *first = m_Data;
            T *last  = pLast();

            for (T *it = last; it >= first; --it)
            {
                if (*it == what)
                {
                    return it - first;
                }
            }

            return npos;
        }

        REV_INLINE u64 Count() const { return m_Count; }

        REV_INLINE const T *Data() const { return m_Data; }
        REV_INLINE       T *Data()       { return m_Data; }

        REV_INLINE bool Empty() const { return !m_Count; }

        REV_INLINE const T *begin()   const { return m_Data;           }
        REV_INLINE const T *cbegin()  const { return m_Data;           }
        REV_INLINE const T *rbegin()  const { return m_Data + m_Count; }
        REV_INLINE const T *crbegin() const { return m_Data + m_Count; }

        REV_INLINE const T *end()   const { return m_Data + m_Count; }
        REV_INLINE const T *cend()  const { return m_Data + m_Count; }
        REV_INLINE const T *rend()  const { return m_Data;           }
        REV_INLINE const T *crend() const { return m_Data;           }

        REV_INLINE T *begin()  { return m_Data;           }
        REV_INLINE T *rbegin() { return m_Data + m_Count; }

        REV_INLINE T *end()  { return m_Data + m_Count; }
        REV_INLINE T *rend() { return m_Data;           }

        REV_INLINE const T& First() const { return *m_Data; }
        REV_INLINE const T& Last()  const { return m_Count ? m_Data[m_Count - 1] : *m_Data; }

        REV_INLINE T& First() { return *m_Data; }
        REV_INLINE T& Last()  { return m_Count ? m_Data[m_Count - 1] : *m_Data; }

        REV_INLINE const T *pFirst() const { return m_Data; }
        REV_INLINE const T *pLast()  const { return m_Count ? m_Data + m_Count - 1 : m_Data; }

        REV_INLINE T *pFirst() { return m_Data; }
        REV_INLINE T *pLast()  { return m_Count ? m_Data + m_Count - 1 : m_Data; }

        REV_INLINE const T *GetPointer(u64 index) const
        {
            REV_CHECK_M(m_Count, "Array is empty");
            REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
            return m_Data + index;
        }

        REV_INLINE T *GetPointer(u64 index)
        {
            REV_CHECK_M(m_Count, "Array is empty");
            REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
            return m_Data + index;
        }

        REV_INLINE ConstArray& operator=(nullptr_t)
        {
            m_Data  = null;
            m_Count = 0;
            return *this;
        }

        REV_INLINE ConstArray& operator=(const ConstArray& other)
        {
            if (this != &other)
            {
                m_Data  = other.m_Data;
                m_Count = other.m_Count;
            }
            return *this;
        }

        REV_INLINE ConstArray& operator=(ConstArray&& other) noexcept
        {
            if (this != &other)
            {
                m_Data        = other.m_Data;
                m_Count       = other.m_Count;
                other.m_Data  = null;
                other.m_Count = 0;
            }
            return *this;
        }

        REV_INLINE const T& operator[](u64 index) const
        {
            REV_CHECK_M(m_Count, "Array is empty");
            REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
            return m_Data[index];
        }

        REV_INLINE T& operator[](u64 index)
        {
            REV_CHECK_M(m_Count, "Array is empty");
            REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
            return m_Data[index];
        }

        REV_INLINE const T *operator+(u64 index) const
        {
            REV_CHECK_M(m_Count, "Array is empty");
            REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
            return m_Data + index;
        }

        REV_INLINE T *operator+(u64 index)
        {
            REV_CHECK_M(m_Count, "Array is empty");
            REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
            return m_Data + index;
        }

    private:
        T   *m_Data;
        u64  m_Count;
    };

    template<typename T> ConstArray(T *, u64)             -> ConstArray<T>;
    template<typename T> ConstArray(const ConstArray<T>&) -> ConstArray<T>;
    template<typename T> ConstArray(ConstArray<T>&&)      -> ConstArray<T>;
}
