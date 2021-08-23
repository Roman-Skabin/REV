//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "tools/string.h"

namespace REV
{

String::String(Allocator *allocator, u64 initial_capacity, u64 alignment_in_bytes)
    : m_Header(null)
{
    REV_CHECK(allocator);
    if (alignment_in_bytes < REV::DEFAULT_ALIGNMENT) alignment_in_bytes = REV::DEFAULT_ALIGNMENT;

    m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + initial_capacity, alignment_in_bytes));
    m_Header->allocator          = allocator;
    m_Header->length             = 0;
    m_Header->capacity           = initial_capacity;
    m_Header->alignment_in_bytes = alignment_in_bytes;
}

String::String(Allocator *allocator, char symbol, u64 count, u64 alignment_in_bytes)
    : m_Header(null)
{
    REV_CHECK(allocator);
    if (alignment_in_bytes < REV::DEFAULT_ALIGNMENT) alignment_in_bytes = REV::DEFAULT_ALIGNMENT;

    m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * count, alignment_in_bytes));
    m_Header->allocator          = allocator;
    m_Header->length             = count;
    m_Header->capacity           = 2 * count;
    m_Header->alignment_in_bytes = alignment_in_bytes;

    FillMemoryChar(m_Header->data, symbol, count);
}

String::String(Allocator *allocator, const char *cstring, u64 length, u64 alignment_in_bytes)
    : m_Header(null)
{
    REV_CHECK(allocator);
    if (alignment_in_bytes < REV::DEFAULT_ALIGNMENT) alignment_in_bytes = REV::DEFAULT_ALIGNMENT;

    m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * length, alignment_in_bytes));
    m_Header->allocator          = allocator;
    m_Header->length             = length;
    m_Header->capacity           = 2 * length;
    m_Header->alignment_in_bytes = alignment_in_bytes;

    CopyMemory(m_Header->data, cstring, length);
}

String::String(Allocator *allocator, const ConstString& const_string, u64 alignment_in_bytes)
    : m_Header(null)
{
    REV_CHECK(allocator);
    if (alignment_in_bytes < REV::DEFAULT_ALIGNMENT) alignment_in_bytes = REV::DEFAULT_ALIGNMENT;

    m_Header                     = cast<Header *>(allocator->AllocateAligned(sizeof(Header) + 2 * const_string.Length(), alignment_in_bytes));
    m_Header->allocator          = allocator;
    m_Header->length             = const_string.Length();
    m_Header->capacity           = 2 * const_string.Length();
    m_Header->alignment_in_bytes = alignment_in_bytes;

    CopyMemory(m_Header->data, const_string.Data(), const_string.Length());
}

String::String(const String& other)
{
    m_Header = cast<Header *>(other.m_Header->allocator->AllocateAligned(sizeof(Header) + other.m_Header->capacity,
                                                                         other.m_Header->alignment_in_bytes));
    CopyMemory(m_Header, other.m_Header, sizeof(Header) + other.m_Header->length);
}

String::String(String&& other)
    : m_Header(other.m_Header)
{
    other.m_Header = null;
}

String::~String()
{
    if (m_Header)
    {
        m_Header->allocator->DeAllocA(m_Header);
    }
}

String& String::Insert(u64 where, char symbol, u64 count)
{
    REV_CHECK_M(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

    u64 old_length = m_Header->length;

    m_Header->length += count;
    Expand();

    if (where < old_length)
    {
        MoveMemory(m_Header->data + where + count,
                   m_Header->data + where,
                   old_length - where);
    }

    FillMemoryChar(m_Header->data + where, symbol, count);
    return *this;
}

String& String::Insert(u64 where, const char *cstring, u64 length)
{
    REV_CHECK_M(where <= m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length, where);

    u64 old_length = m_Header->length;

    m_Header->length += length;
    Expand();

    if (where < old_length)
    {
        MoveMemory(m_Header->data + where + length,
                   m_Header->data + where,
                   old_length - where);
    }

    CopyMemory(m_Header->data + where, cstring, length);
    return *this;
}

String& String::Erase(u64 from, u64 to)
{
    if (to == npos) to = from + 1;
    REV_CHECK_M(from < m_Header->length && from < to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);

    if (from < m_Header->length - 1 || to < m_Header->length)
    {
        MoveMemory(m_Header->data + from,
                   m_Header->data + to,
                   m_Header->length - to);
    }

    u64 delta = to - from;
    ZeroMemory(m_Header->data + m_Header->length - delta, delta);

    m_Header->length -= delta;
    Fit();

    return *this;
}

String& String::Clear()
{
    ZeroMemory(m_Header->data, m_Header->length);
    m_Header->length = 0;
    return *this;
}

void String::Reserve(u64 capacity)
{
    if (m_Header->length <= capacity && m_Header->capacity != capacity)
    {
        m_Header = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                         sizeof(Header) + capacity,
                                                                         m_Header->alignment_in_bytes));
        m_Header->capacity = capacity;
    }
}

String& String::Replace(u64 from, u64 to, char symbol, u64 count)
{
    if (to == npos) to = from + 1;
    REV_CHECK_M(from < m_Header->length && from < to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);

    u64 delta = to - from;
    FillMemoryChar(m_Header->data + from, symbol, delta);

    if (count > delta)
    {
        Insert(to, symbol, count - delta);
    }

    return *this;
}

String& String::Replace(u64 from, u64 to, const char *cstring, u64 length)
{
    if (to == npos) to = from + 1;
    REV_CHECK_M(from < m_Header->length && from < to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);

    u64 delta = to - from;
    CopyMemory(m_Header->data + from, cstring, delta);

    if (length > delta)
    {
        Insert(to, cstring + delta, length - delta);
    }

    return *this;
}

void String::Reverse()
{
    char *first = m_Header->data;
    char *last  = pLast();

    while (first < last)
    {
        char temp = *first;
        *first++  = *last;
        *last--   = temp;
    }
}

String String::SubString(u64 from) const &
{
    REV_CHECK_M(from <= m_Header->length, "Bad arguments: from = %I64u, length = %I64u", from, m_Header->length);
    if (from == 0)
    {
        return *this;
    }
    else if (from == m_Header->length)
    {
        return RTTI::move(String(m_Header->allocator, 16ui64, m_Header->alignment_in_bytes));
    }
    else
    {
        return RTTI::move(String(m_Header->allocator, m_Header->data + from, m_Header->length - from, m_Header->alignment_in_bytes));
    }
}

String String::SubString(u64 from, u64 to) const &
{
    REV_CHECK_M(from <= to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);
    if (from == 0 && to == m_Header->length)
    {
        return *this;
    }
    else if (from == to)
    {
        return RTTI::move(String(m_Header->allocator, 16ui64, m_Header->alignment_in_bytes));
    }
    else
    {
        return RTTI::move(String(m_Header->allocator, m_Header->data + from, to - from, m_Header->alignment_in_bytes));
    }
}

String String::SubString(u64 from) &&
{
    REV_CHECK_M(from <= m_Header->length, "Bad arguments: from = %I64u, length = %I64u", from, m_Header->length);
    if (from == 0)
    {
        return RTTI::move(*this);
    }
    else if (from == m_Header->length)
    {
        return RTTI::move(Clear());
    }
    else
    {
        return RTTI::move(Erase(0, from));
    }
}

String String::SubString(u64 from, u64 to) &&
{
    REV_CHECK_M(from <= to && to <= m_Header->length, "Bad arguments: from = %I64u, to = %I64u, length = %I64u", from, to, m_Header->length);
    if (from == 0 && to == m_Header->length)
    {
        return RTTI::move(*this);
    }
    else if (from == to)
    {
        return RTTI::move(Clear());
    }
    else
    {
        MoveMemory(m_Header->data, m_Header->data + from, to - from);

        u64 new_length = to - from;
        ZeroMemory(m_Header->data + new_length, m_Header->length - new_length);

        m_Header->length = new_length;
        Fit();

        return RTTI::move(*this);
    }
}

u64 String::Find(char symbol, u64 offset) const
{
    REV_CHECK_M(offset < m_Header->length, "Offset out of bounds.");

    char *_end = m_Header->data + m_Header->length;

    for (const char *it = m_Header->data + offset; it < _end; ++it)
    {
        if (*it == symbol)
        {
            return it - m_Header->data;
        }
    }

    return npos;
}

u64 String::Find(const char *cstring, u64 cstring_length, u64 offset) const
{
    REV_CHECK_M(offset < m_Header->length, "Offset out of bounds.");

    const char *_end = m_Header->data + m_Header->length;

    for (const char *it = m_Header->data + offset; it < _end; ++it)
    {
        if (it + cstring_length > _end)
        {
            return npos;
        }

        if (CompareStrings(it, cstring_length, cstring, cstring_length) == COMPARE_RESULT_EQ)
        {
            return it - m_Header->data;
        }
    }

    return npos;
}

u64 String::RFind(char symbol, u64 offset) const
{
    REV_CHECK_M(offset < m_Header->length, "Offset out of bounds.");

    for (const char *it = pLast() - offset; it >= m_Header->data; --it)
    {
        if (*it == symbol)
        {
            return it - m_Header->data;
        }
    }

    return npos;
}

String& String::operator=(nullptr_t)
{
    ZeroMemory(m_Header->data, m_Header->length);

    m_Header->length = 0;
    Fit();

    return *this;
}

String& String::operator=(char symbol)
{
    if (m_Header->length > 1)
    {
        ZeroMemory(m_Header->data + 1, m_Header->length - 1);
    }

    *m_Header->data = symbol;

    m_Header->length = 1;
    Fit();

    return *this;
}

String& String::operator=(const ConstString& const_string)
{
    if (const_string.Length() < m_Header->length)
    {
        ZeroMemory(m_Header->data + const_string.Length(), m_Header->length - const_string.Length());
    }

    CopyMemory(m_Header->data, const_string.Data(), const_string.Length());

    m_Header->length = const_string.Length();
    Fit();

    return *this;
}

String& String::operator=(const String& other)
{
    if (this != &other)
    {
        if (other.m_Header->length < m_Header->length)
        {
            ZeroMemory(m_Header->data + other.m_Header->length, m_Header->length - other.m_Header->length);
        }

        CopyMemory(m_Header->data, other.m_Header->data, other.m_Header->length);

        m_Header->length = other.m_Header->length;
        Fit();
    }
    return *this;
}

String& String::operator=(String&& other)
{
    if (this != &other)
    {
        m_Header->allocator->DeAllocA(m_Header);

        m_Header       = other.m_Header;
        other.m_Header = null;
    }
    return *this;
}

void String::Expand()
{
    if (m_Header->length > m_Header->capacity)
    {
        m_Header->capacity *= 2;
        m_Header            = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                                    sizeof(Header) + m_Header->capacity,
                                                                                    m_Header->alignment_in_bytes));
    }
}

void String::Fit()
{
    if (2 * m_Header->length < m_Header->capacity)
    {
        m_Header->capacity = 2 * m_Header->length;
        m_Header           = cast<Header *>(m_Header->allocator->ReAllocateAligned(cast<void *&>(m_Header),
                                                                                   sizeof(Header) + m_Header->capacity,
                                                                                   m_Header->alignment_in_bytes));
    }
}

}
