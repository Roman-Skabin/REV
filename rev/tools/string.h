//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "core/allocator.h"
#include "tools/static_string.hpp"

namespace REV
{

class String final
{
public:
    static constexpr u64 npos = REV_U64_MAX;

    explicit String(Allocator *allocator, u64 initial_capacity = 16, u64 alignment_in_bytes = REV::DEFAULT_ALIGNMENT);
    String(Allocator *allocator, char symbol, u64 count, u64 alignment_in_bytes = REV::DEFAULT_ALIGNMENT);
    String(Allocator *allocator, const char *cstring, u64 cstring_length, u64 alignment_in_bytes = REV::DEFAULT_ALIGNMENT);
    String(Allocator *allocator, const ConstString& const_string, u64 alignment_in_bytes = REV::DEFAULT_ALIGNMENT);
    String(const String& other);
    String(String&& other);

    template<u64 ssc, u64 ssac>
    String(Allocator *allocator, const StaticString<ssc, ssac>& static_string, u64 alignment_in_bytes = REV::DEFAULT_ALIGNMENT)
        : m_Header(null)
    {
        REV_CHECK(allocator);
        if (alignment_in_bytes < REV::DEFAULT_ALIGNMENT) alignment_in_bytes = REV::DEFAULT_ALIGNMENT;

        m_Header                     = cast(Header *, allocator->AllocateAligned(sizeof(Header) + ssc, alignment_in_bytes));
        m_Header->allocator          = allocator;
        m_Header->length             = static_string.Length();
        m_Header->capacity           = ssc;
        m_Header->alignment_in_bytes = alignment_in_bytes;

        CopyMemory(m_Header->data, static_string.Data(), static_string.Length());
    }

    ~String();

               String& Insert(u64 where, char symbol, u64 count = 1);
               String& Insert(u64 where, const char *cstring, u64 cstring_length);
    REV_INLINE String& Insert(u64 where, const ConstString& const_string) { return Insert(where, const_string.Data(), const_string.Length()); }
    REV_INLINE String& Insert(u64 where, const String& string)            { return Insert(where, string.Data(),       string.Length());       }

    template<u64 ssc, u64 ssac>
    REV_INLINE String& Insert(u64 where, const StaticString<ssc, ssac>& static_string)
    {
        return Insert(where, static_string.Data(), static_string.Length());
    }

    REV_INLINE String& PushFront(char symbol, u64 count = 1)              { return Insert(0, symbol, count);           }
    REV_INLINE String& PushFront(const char *cstring, u64 cstring_length) { return Insert(0, cstring, cstring_length); }
    REV_INLINE String& PushFront(const ConstString& const_string)         { return Insert(0, const_string);            }
    REV_INLINE String& PushFront(const String& string)                    { return Insert(0, string);                  }

    template<u64 ssc, u64 ssac>
    REV_INLINE String& PushFront(const StaticString<ssc, ssac>& static_string)
    {
        return Insert(0, static_string);
    }
    
    REV_INLINE String& PushBack(char symbol, u64 count = 1)              { return Insert(m_Header->length, symbol, count);           }
    REV_INLINE String& PushBack(const char *cstring, u64 cstring_length) { return Insert(m_Header->length, cstring, cstring_length); }
    REV_INLINE String& PushBack(const ConstString& const_string)         { return Insert(m_Header->length, const_string);            }
    REV_INLINE String& PushBack(const String& string)                    { return Insert(m_Header->length, string);                  }

    template<u64 ssc, u64 ssac>
    REV_INLINE String& PushBack(const StaticString<ssc, ssac>& static_string)
    {
        return Insert(m_Header->length, static_string);
    }

    String& Erase(u64 from, u64 to = npos);

    REV_INLINE String& EraseFront() { return Erase(0);                    }
    REV_INLINE String& EraseBack()  { return Erase(m_Header->length - 1); }

    String& Clear();

    void Reserve(u64 capacity);

               String& Replace(u64 from, u64 to, char symbol, u64 count = 1);
               String& Replace(u64 from, u64 to, const char *cstring, u64 cstring_length);
    REV_INLINE String& Replace(u64 from, u64 to, const ConstString& const_string) { return Replace(from, to, const_string.Data(), const_string.Length());  }
    REV_INLINE String& Replace(u64 from, u64 to, const String& string)            { return Replace(from, to, string.Data(),       string.Length());        }

    template<u64 ssc, u64 ssac>
    REV_INLINE String& Replace(u64 from, u64 to, const StaticString<ssc, ssac>& static_string)
    {
        return Replace(from, to, static_string.Data(), static_string.Length());
    }

    void Reverse();

    REV_INLINE static String Concat(const String&      left,                        char         right, u64 count = 1)    { return RTTI::move(String(left).PushBack(right, count));        }
    REV_INLINE static String Concat(const String&      left,                  const char        *right, u64 right_length) { return RTTI::move(String(left).PushBack(right, right_length)); }
    REV_INLINE static String Concat(const String&      left,                  const ConstString& right)                   { return RTTI::move(String(left).PushBack(right));               }
    REV_INLINE static String Concat(const String&      left,                  const String&      right)                   { return RTTI::move(String(left).PushBack(right));               }
    REV_INLINE static String Concat(const String&      left,                        String&&     right)                   { return RTTI::move(       right.PushFront(left));               }
    REV_INLINE static String Concat(      String&&     left,                        char         right, u64 count = 1)    { return RTTI::move(       left.PushBack(right, count));         }
    REV_INLINE static String Concat(      String&&     left,                  const char        *right, u64 right_length) { return RTTI::move(       left.PushBack(right, right_length));  }
    REV_INLINE static String Concat(      String&&     left,                  const ConstString& right)                   { return RTTI::move(       left.PushBack(right));                }
    REV_INLINE static String Concat(      String&&     left,                  const String&      right)                   { return RTTI::move(       left.PushBack(right));                }
    REV_INLINE static String Concat(      String&&     left,                        String&&     right)                   { return RTTI::move(       left.PushBack(right));                }
    REV_INLINE static String Concat(      char         left,                  const String&      right)                   { return RTTI::move(String(right).PushFront(left));              }
    REV_INLINE static String Concat(const char        *left, u64 left_length, const String&      right)                   { return RTTI::move(String(right).PushFront(left, left_length)); }
    REV_INLINE static String Concat(const ConstString& left,                  const String&      right)                   { return RTTI::move(String(right).PushFront(left));              }
    REV_INLINE static String Concat(      char         left,                        String&&     right)                   { return RTTI::move(       right.PushFront(left));               }
    REV_INLINE static String Concat(const char        *left, u64 left_length,       String&&     right)                   { return RTTI::move(       right.PushFront(left, left_length));  }
    REV_INLINE static String Concat(const ConstString& left,                        String&&     right)                   { return RTTI::move(       right.PushFront(left));               }

    template<u64 ssc, u64 ssac> REV_INLINE static String Concat(const String&                  left, const StaticString<ssc, ssac>& right) { return RTTI::move(String(left).PushBack(right));  }
    template<u64 ssc, u64 ssac> REV_INLINE static String Concat(      String&&                 left, const StaticString<ssc, ssac>& right) { return RTTI::move(       left.PushBack(right));   }
    template<u64 ssc, u64 ssac> REV_INLINE static String Concat(const StaticString<ssc, ssac>& left, const String&                  right) { return RTTI::move(String(right).PushFront(left)); }
    template<u64 ssc, u64 ssac> REV_INLINE static String Concat(const StaticString<ssc, ssac>& left,       String&&                 right) { return RTTI::move(       right.PushFront(left));  }

    String SubString(u64 from) const &;
    String SubString(u64 from, u64 to) const &;
    String SubString(u64 from) &&;
    String SubString(u64 from, u64 to) &&;

               u64 Find(char symbol, u64 offset = 0)                             const;
               u64 Find(const char *cstring, u64 cstring_length, u64 offset = 0) const;
    REV_INLINE u64 Find(const ConstString& const_string, u64 offset = 0)         const { return Find(const_string.Data(), const_string.Length(), offset); }
    REV_INLINE u64 Find(const String& string, u64 offset = 0)                    const { return Find(string.Data(),       string.Length(),       offset); }

    template<u64 ssc, u64 ssac>
    REV_INLINE u64 Find(const StaticString<ssc, ssac>& static_string, u64 offset = 0) const
    {
        return Find(static_string.Data(), static_string.Length(), offset);
    }

    u64 RFind(char symbol, u64 offset = 0) const;

    REV_INLINE COMPARE_RESULT Compare(const char *cstring, u64 cstring_length) const { return CompareStrings(m_Header->data, m_Header->length, cstring,             cstring_length);        }
    REV_INLINE COMPARE_RESULT Compare(const ConstString& const_string)         const { return CompareStrings(m_Header->data, m_Header->length, const_string.Data(), const_string.Length()); }
    REV_INLINE COMPARE_RESULT Compare(const String& string) const
    {
        return CompareStringsAligned(m_Header->data, AlignUp(m_Header->length, 16), string.m_Header->data, AlignUp(string.m_Header->length, 16));
    }

    template<u64 ssc, u64 ssac>
    REV_INLINE COMPARE_RESULT Compare(const StaticString<ssc, ssac>& static_string) const
    {
        return CompareStringsAligned(m_Header->data, AlignUp(m_Header->length, 16), static_string.m_Data, AlignUp(static_string.m_Length, 16));
    }

    REV_INLINE u64 Length()    const { return m_Header->length;             }
    REV_INLINE u64 Capacity()  const { return m_Header->capacity;           }
    REV_INLINE u64 Alignment() const { return m_Header->alignment_in_bytes; }

    REV_INLINE const char *Data() const { return m_Header->data; }
    REV_INLINE       char *Data()       { return m_Header->data; }

    REV_INLINE const String& ToString() const { return *this; }
    REV_INLINE       String& ToString()       { return *this; }

    REV_INLINE ConstString ToConstString() const { return ConstString(m_Header->data, m_Header->length); }

    REV_INLINE bool Empty() const { return !m_Header->length; }

    REV_INLINE const Allocator *GetAllocator() const { return m_Header->allocator; }

    REV_INLINE const char *begin()   const { return m_Header->data;                    }
    REV_INLINE const char *cbegin()  const { return m_Header->data;                    }
    REV_INLINE const char *rbegin()  const { return m_Header->data + m_Header->length; }
    REV_INLINE const char *crbegin() const { return m_Header->data + m_Header->length; }

    REV_INLINE const char *end()   const { return m_Header->data + m_Header->length; }
    REV_INLINE const char *cend()  const { return m_Header->data + m_Header->length; }
    REV_INLINE const char *rend()  const { return m_Header->data;                    }
    REV_INLINE const char *crend() const { return m_Header->data;                    }

    REV_INLINE char *begin()  { return m_Header->data;                    }
    REV_INLINE char *rbegin() { return m_Header->data + m_Header->length; }

    REV_INLINE char *end()  { return m_Header->data + m_Header->length; }
    REV_INLINE char *rend() { return m_Header->data;                    }

    REV_INLINE char First() const { return *m_Header->data;                                                           }
    REV_INLINE char Last()  const { return m_Header->length ? m_Header->data[m_Header->length - 1] : *m_Header->data; }

    REV_INLINE char& First() { return *m_Header->data;                                                           }
    REV_INLINE char& Last()  { return m_Header->length ? m_Header->data[m_Header->length - 1] : *m_Header->data; }

    REV_INLINE const char *pFirst() const { return m_Header->data;                                                            }
    REV_INLINE const char *pLast()  const { return m_Header->length ? m_Header->data + m_Header->length - 1 : m_Header->data; }

    REV_INLINE char *pFirst() { return m_Header->data;                                                            }
    REV_INLINE char *pLast()  { return m_Header->length ? m_Header->data + m_Header->length - 1 : m_Header->data; }

    REV_INLINE const char *GetPointer(u64 index) const
    {
        REV_CHECK_M(m_Header->length, "String is empty");
        REV_CHECK_M(index < m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length - 1, index);
        return m_Header->data + index;
    }

    REV_INLINE char *GetPointer(u64 index)
    {
        REV_CHECK_M(m_Header->length, "String is empty");
        REV_CHECK_M(index < m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length - 1, index);
        return m_Header->data + index;
    }

    String& operator=(nullptr_t);
    String& operator=(char symbol);
    String& operator=(const ConstString& const_string);
    String& operator=(const String& other);
    String& operator=(String&& other);

    template<u64 ssc, u64 ssac>
    String& operator=(const StaticString<ssc, ssac>& static_string)
    {
        if (static_string.Length() < m_Header->length)
        {
            ZeroMemory(m_Header->data + static_string.Length(), m_Header->length - static_string.Length());
        }

        CopyMemory(m_Header->data, static_string.Data(), static_string.Length());

        m_Header->length = static_string.Length();
        Fit();

        return *this;
    }

    REV_INLINE String& operator+=(char right)               { return PushBack(right); }
    REV_INLINE String& operator+=(const ConstString& right) { return PushBack(right); }
    REV_INLINE String& operator+=(const String& right)      { return PushBack(right); }

    template<u64 ssc, u64 ssac>
    REV_INLINE String& operator+=(const StaticString<ssc, ssac>& right)
    {
        return PushBack(right);
    }

    REV_INLINE char operator[](u64 index) const
    {
        REV_CHECK_M(m_Header->length, "String is empty");
        REV_CHECK_M(index < m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length - 1, index);
        return m_Header->data[index];
    }

    REV_INLINE char& operator[](u64 index)
    {
        REV_CHECK_M(m_Header->length, "String is empty");
        REV_CHECK_M(index < m_Header->length, "Expected max index: %I64u, got: %I64u", m_Header->length - 1, index);
        return m_Header->data[index];
    }

private:
    void Expand();
    void Fit();

private:
    struct Header
    {
        Allocator *allocator;
        u64        length;
        u64        capacity;
        u64        alignment_in_bytes;
        char       data[0];
    };

    Header *m_Header;
};

REV_INLINE bool operator==(const String&      left, const ConstString& right) { return !left.Compare(right);      }
REV_INLINE bool operator==(const String&      left, const String&      right) { return !left.Compare(right);      }
REV_INLINE bool operator==(const ConstString& left, const String&      right) { return !right.Compare(left);      }
REV_INLINE bool operator!=(const String&      left, const ConstString& right) { return  left.Compare(right);      }
REV_INLINE bool operator!=(const String&      left, const String&      right) { return  left.Compare(right);      }
REV_INLINE bool operator!=(const ConstString& left, const String&      right) { return  right.Compare(left);      }
REV_INLINE bool operator< (const String&      left, const ConstString& right) { return  left.Compare(right) <  0; }
REV_INLINE bool operator< (const String&      left, const String&      right) { return  left.Compare(right) <  0; }
REV_INLINE bool operator< (const ConstString& left, const String&      right) { return  right.Compare(left) >  0; }
REV_INLINE bool operator> (const String&      left, const ConstString& right) { return  left.Compare(right) >  0; }
REV_INLINE bool operator> (const String&      left, const String&      right) { return  left.Compare(right) >  0; }
REV_INLINE bool operator> (const ConstString& left, const String&      right) { return  right.Compare(left) <  0; }
REV_INLINE bool operator<=(const String&      left, const ConstString& right) { return  left.Compare(right) <= 0; }
REV_INLINE bool operator<=(const String&      left, const String&      right) { return  left.Compare(right) <= 0; }
REV_INLINE bool operator<=(const ConstString& left, const String&      right) { return  right.Compare(left) >= 0; }
REV_INLINE bool operator>=(const String&      left, const ConstString& right) { return  left.Compare(right) >= 0; }
REV_INLINE bool operator>=(const String&      left, const String&      right) { return  left.Compare(right) >= 0; }
REV_INLINE bool operator>=(const ConstString& left, const String&      right) { return  right.Compare(left) <= 0; }

template<u64 ssc, u64 ssac> REV_INLINE bool operator==(const String&                  left, const StaticString<ssc, ssac>& right) { return !left.Compare(right);      }
template<u64 ssc, u64 ssac> REV_INLINE bool operator==(const StaticString<ssc, ssac>& left, const String&                  right) { return !right.Compare(left);      }
template<u64 ssc, u64 ssac> REV_INLINE bool operator!=(const String&                  left, const StaticString<ssc, ssac>& right) { return  left.Compare(right);      }
template<u64 ssc, u64 ssac> REV_INLINE bool operator!=(const StaticString<ssc, ssac>& left, const String&                  right) { return  right.Compare(left);      }
template<u64 ssc, u64 ssac> REV_INLINE bool operator< (const String&                  left, const StaticString<ssc, ssac>& right) { return  left.Compare(right) <  0; }
template<u64 ssc, u64 ssac> REV_INLINE bool operator< (const StaticString<ssc, ssac>& left, const String&                  right) { return  right.Compare(left) >  0; }
template<u64 ssc, u64 ssac> REV_INLINE bool operator> (const String&                  left, const StaticString<ssc, ssac>& right) { return  left.Compare(right) >  0; }
template<u64 ssc, u64 ssac> REV_INLINE bool operator> (const StaticString<ssc, ssac>& left, const String&                  right) { return  right.Compare(left) <  0; }
template<u64 ssc, u64 ssac> REV_INLINE bool operator<=(const String&                  left, const StaticString<ssc, ssac>& right) { return  left.Compare(right) <= 0; }
template<u64 ssc, u64 ssac> REV_INLINE bool operator<=(const StaticString<ssc, ssac>& left, const String&                  right) { return  right.Compare(left) >= 0; }
template<u64 ssc, u64 ssac> REV_INLINE bool operator>=(const String&                  left, const StaticString<ssc, ssac>& right) { return  left.Compare(right) >= 0; }
template<u64 ssc, u64 ssac> REV_INLINE bool operator>=(const StaticString<ssc, ssac>& left, const String&                  right) { return  right.Compare(left) <= 0; }

REV_INLINE String operator+(const String&      left,       char         right) { return String::Concat(           left ,           right ); }
REV_INLINE String operator+(const String&      left, const ConstString& right) { return String::Concat(           left ,           right ); }
REV_INLINE String operator+(const String&      left, const String&      right) { return String::Concat(           left ,           right ); }
REV_INLINE String operator+(      char         left, const String&      right) { return String::Concat(           left ,           right ); }
REV_INLINE String operator+(const ConstString& left, const String&      right) { return String::Concat(           left ,           right ); }
REV_INLINE String operator+(String&&           left,       char         right) { return String::Concat(RTTI::move(left),           right ); }
REV_INLINE String operator+(String&&           left, const ConstString& right) { return String::Concat(RTTI::move(left),           right ); }
REV_INLINE String operator+(String&&           left, const String&      right) { return String::Concat(RTTI::move(left),           right ); }
REV_INLINE String operator+(      char         left, String&&           right) { return String::Concat(           left, RTTI::move(right)); }
REV_INLINE String operator+(const ConstString& left, String&&           right) { return String::Concat(           left, RTTI::move(right)); }
REV_INLINE String operator+(const String&      left, String&&           right) { return String::Concat(           left, RTTI::move(right)); }

template<u64 ssc, u64 ssac> REV_INLINE String operator+(const String&                  left, const StaticString<ssc, ssac>& right) { return String::Concat(           left ,            right ); }
template<u64 ssc, u64 ssac> REV_INLINE String operator+(const StaticString<ssc, ssac>& left, const String&                  right) { return String::Concat(           left ,            right ); }
template<u64 ssc, u64 ssac> REV_INLINE String operator+(      String&&                 left, const StaticString<ssc, ssac>& right) { return String::Concat(RTTI::move(left),            right ); }
template<u64 ssc, u64 ssac> REV_INLINE String operator+(const StaticString<ssc, ssac>& left,       String&&                 right) { return String::Concat(           left , RTTI::move(right)); }

}
