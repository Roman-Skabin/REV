//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/allocator.h"

template<typename T, typename Allocator_t = Allocator>
class List final
{
public:
    using Type = T;

    class Node final
    {
        Node *Prev() { return m_Prev; }
        Node *Next() { return m_Next; }
        T&    Data() { return m_Data; }

        const Node *Prev() const { return m_Prev; }
        const Node *Next() const { return m_Next; }
        const T&    Data() const { return m_Data; }

    private:
        Node *m_Prev;
        Node *m_Next;
        T     m_Data;

        friend class List;
    };

public:
    List(in Allocator_t *allocator)
        : m_Allocator(allocator),
          m_First(null),
          m_Last(null)
    {
        Check(allocator);
    }

    List(in const List& other)
        : m_Allocator(other..m_Allocator),
          m_First(null),
          m_Last(null)
    {
        for (Node *it = other.m_First; it; it = it->m_Next)
        {
            PushBack(it->m_Data);
        }
    }

    List(in List&& other)
        : m_Allocator(other..m_Allocator),
          m_First(other.m_First),
          m_Last(other.m_Last)
    {
        other.m_First = null;
        other.m_Last  = null;
    }

    ~List()
    {
        Clear();
        m_Allocator = null;
    }

    u64 Count() const { return m_Count; }
    
    bool Empty() const { return m_Count == 0; }

    const Node *First() const { return m_First; }
          Node *First()       { return m_First; }

    const Node *Last() const { return m_Last; }
          Node *Last()       { return m_Last; }

    template<typename T>
    void ForEach(T&& callback) const
    {
        for (const Node *it = m_First; it; it = it->m_Next)
        {
            callback(it);
        }
    }

    template<typename T>
    void ForEach(T&& callback)
    {
        for (Node *it = m_First; it; it = it->m_Next)
        {
            callback(it);
        }
    }

    template<typename ...U, typename = RTTI::enable_if_t<RTTI::is_same_v<T, U>...>>
    void Insert(in Node *where, in const U&... elements)
    {
        (..., InsertOne(where)->m_Data = elements);
    }

    template<typename ...U, typename = RTTI::enable_if_t<RTTI::is_same_v<T, U>...>>
    void Insert(in Node *where, in U&&... elements)
    {
        (..., InsertOne(where)->m_Data = RTTI::move(elements));
    }

    template<typename ...U, typename = RTTI::enable_if_t<RTTI::is_same_v<T, U>...>> void PushFront(in const U&... elements) { Insert(m_First, elements...); }
    template<typename ...U, typename = RTTI::enable_if_t<RTTI::is_same_v<T, U>...>> void PushBack(in const U&... elements)  { Insert(null,    elements...); }

    template<typename ...U, typename = RTTI::enable_if_t<RTTI::is_same_v<T, U>...>> void PushFront(in U&&... elements) { Insert(m_First, RTTI::forward<U>(elements)...); }
    template<typename ...U, typename = RTTI::enable_if_t<RTTI::is_same_v<T, U>...>> void PushBack(in U&&... elements)  { Insert(null,    RTTI::forward<U>(elements)...); }

    template<typename ...ConstructorArgs>
    void Emplace(in Node *where, in const ConstructorArgs&... args)
    {
        InsertOne(where)->m_Data = T(args...);
    }

    template<typename ...ConstructorArgs>
    void Emplace(in Node *where, in ConstructorArgs&&... args)
    {
        InsertOne(where)->m_Data = T(RTTI::forward<ConstructorArgs>(args)...);
    }

    template<typename ...ConstructorArgs> void EmplaceFront(in const ConstructorArgs&... args) { Emplace(m_First, args...); }
    template<typename ...ConstructorArgs> void EmplaceBack(in const ConstructorArgs&... args)  { Emplace(null,    args...); }

    template<typename ...ConstructorArgs> void EmplaceFront(in ConstructorArgs&&... args) { Emplace(m_First, RTTI::forward<ConstructorArgs>(args)...); }
    template<typename ...ConstructorArgs> void EmplaceBack(in ConstructorArgs&&... args)  { Emplace(null,    RTTI::forward<ConstructorArgs>(args)...); }

    void Erase(in Node *what)
    {
        Node *prev = what->m_Prev;
        Node *next = what->m_Next;

        if (prev) prev->m_Next = m_Next;
        if (next) next->m_Prev = m_Prev;

        if (what == m_Firts) m_First = next;
        if (what == m_Last ) m_Last  = prev;

        m_Allocator.DeAlloc(what);
        --m_Count;
    }

    void EraseFront() { Erase(m_First); }
    void EraseBack()  { Erase(m_Back);  }

    void Clear()
    {
        for (Node *it = m_First; it; )
        {
            it->m_Data.~T();

            Node *next = it->m_Next;
            m_Allocator.DeAlloc(it);
            it = next;
        }

        m_Count = 0;
        m_First = null;
        m_Last  = null;
    }

    List& operator=(in const List& other)
    {
        if (this != &other)
        {
            Clear();

            for (Node *it = other.m_First; it; it = it->m_Next)
            {
                PushBack(it->m_Data);
            }
        }
        return *this;
    }

    List& operator=(in List&& other)
    {
        if (this != &other)
        {
            Clear();

            m_Allocator = other.m_Allocator;
            m_Count     = other.m_Count;
            m_First     = other.m_First;
            m_Last      = other.m_Last;

            other.m_First = null;
            other.m_Last  = null;
        }
        return *this;
    }

    Node *operator[](in u64 index)
    {
        CheckM(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
        Node *node = m_Fisrt;
        for (u64 i = 0; i < index; ++i)
        {
            node = node->m_Next;
        }
        return node;
    }

    const Node *operator[](in u64 index) const
    {
        CheckM(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
        const Node *node = m_Fisrt;
        for (u64 i = 0; i < index; ++i)
        {
            node = node->m_Next;
        }
        return node;
    }

private:
    Node *InsertOne(Node *where)
    {
        Node *node = m_Allocator.Alloc<Node>();
        ++m_Count;

        node->m_Prev = where ? where->m_Prev : m_Last;
        node->m_Next = where;

        if (node->m_Prev    ) node->m_Prev->m_Next = node;
        if (where           ) where->m_Prev        = node;
        if (where == m_First) m_First              = node;
        if (!where          ) m_Last               = node;

        return node;
    }

private:
    Allocator_t *m_Allocator;
    u64          m_Count;
    Node        *m_First;
    Node        *m_Last;
};
