//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/allocator.h"
#include "tools/function.hpp"

// @TODO(Roman): Iterators for list to be able to use for (auto el : list) {}

template<typename, bool, typename>
class List;

template<typename T, typename Allocator_t = Allocator>
class DLinkedListNode final
{
public:
    using Type = T;

    DLinkedListNode()
        : m_Prev(null),
          m_Next(null),
          m_Data()
    {
    }

    DLinkedListNode(const DLinkedListNode& other)
        : m_Prev(other.m_Prev),
          m_Next(other.m_Next),
          m_Data(other.m_Data)
    {
    }

    DLinkedListNode(DLinkedListNode&& other) noexcept
        : m_Prev(other.m_Prev),
          m_Next(other.m_Next),
          m_Data(RTTI::move(other.m_Data))
    {
        other.m_Prev = null;
        other.m_Next = null;
    }

    ~DLinkedListNode()
    {
        m_Prev = null;
        m_Next = null;
    }

    constexpr DLinkedListNode *Prev() { return m_Prev; }
    constexpr DLinkedListNode *Next() { return m_Next; }
    constexpr T&               Data() { return m_Data; }

    constexpr const DLinkedListNode *Prev() const { return m_Prev; }
    constexpr const DLinkedListNode *Next() const { return m_Next; }
    constexpr const T&               Data() const { return m_Data; }

    DLinkedListNode& operator=(const DLinkedListNode& other)
    {
        if (this != &other)
        {
            m_Prev = other.m_Prev;
            m_Next = other.m_Next;
            m_Data = other.m_Data;
        }
        return *this;
    }

    DLinkedListNode& operator=(DLinkedListNode&& other) noexcept
    {
        if (this != &other)
        {
            m_Prev = other.m_Prev;
            m_Next = other.m_Next;
            m_Data = RTTI::move(other.m_Data);

            other.m_Prev = null;
            other.m_Next = null;
        }
        return *this;
    }

private:
    DLinkedListNode *m_Prev;
    DLinkedListNode *m_Next;
    T                m_Data;

    friend class List<T, true, Allocator_t>;
};

template<typename T>
INLINE auto operator==(const DLinkedListNode<T>& left, const DLinkedListNode<T>& right)
    -> decltype(left.Data() == right.Data())
{
    return left.Prev() == right.Prev()
        && left.Next() == right.Next()
        && left.Data() == right.Data();
}

template<typename T>
INLINE auto operator!=(const DLinkedListNode<T>& left, const DLinkedListNode<T>& right)
    -> decltype(left.Data() != right.Data())
{
    return left.Prev() != right.Prev()
        || left.Next() != right.Next()
        || left.Data() != right.Data();
}

template<typename T, typename Allocator_t = Allocator>
class SLinkedListNode final
{
public:
    using Type = T;

    SLinkedListNode()
        : m_Next(null),
          m_Data()
    {
    }

    SLinkedListNode(const SLinkedListNode& other)
        : m_Next(other.m_Next),
          m_Data(other.m_Data)
    {
    }

    SLinkedListNode(SLinkedListNode&& other) noexcept
        : m_Next(other.m_Next),
          m_Data(RTTI::move(other.m_Data))
    {
        other.m_Next = null;
    }

    ~SLinkedListNode()
    {
        m_Next = null;
    }

    constexpr SLinkedListNode *Next() { return m_Next; }
    constexpr T&               Data() { return m_Data; }

    constexpr const SLinkedListNode *Next() const { return m_Next; }
    constexpr const T&               Data() const { return m_Data; }

    SLinkedListNode& operator=(const SLinkedListNode& other)
    {
        if (this != &other)
        {
            m_Next = other.m_Next;
            m_Data = other.m_Data;
        }
        return *this;
    }

    SLinkedListNode& operator=(SLinkedListNode&& other) noexcept
    {
        if (this != &other)
        {
            m_Next = other.m_Next;
            m_Data = RTTI::move(other.m_Data);

            other.m_Next = null;
        }
        return *this;
    }

private:
    SLinkedListNode *m_Next;
    T                m_Data;

    friend class List<T, false, Allocator_t>;
};

template<typename T>
INLINE auto operator==(const SLinkedListNode<T>& left, const SLinkedListNode<T>& right)
    -> decltype(left.Data() == right.Data())
{
    return left.Next() == right.Next()
        && left.Data() == right.Data();
}

template<typename T>
INLINE auto operator!=(const SLinkedListNode<T>& left, const SLinkedListNode<T>& right)
    -> decltype(left.Data() != right.Data())
{
    return left.Next() != right.Next()
        || left.Data() != right.Data();
}

template<typename T, bool doubly_linked = false, typename Allocator_t = Allocator>
class List final
{
public:
    using Type = T;
    using Node = RTTI::conditional_t<doubly_linked, DLinkedListNode<T, Allocator_t>, SLinkedListNode<T, Allocator_t>>;

public:
    List(Allocator_t *allocator)
        : m_Allocator(allocator),
          m_First(null),
          m_Last(null)
    {
        Check(allocator);
    }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<T>>>
    List(const List& other)
        : m_Allocator(other.m_Allocator),
          m_First(null),
          m_Last(null)
    {
        for (Node *it = other.m_First; it; it = it->m_Next)
        {
            PushBack(it->m_Data);
        }
    }

    List(List&& other) noexcept
        : m_Allocator(other.m_Allocator),
          m_First(other.m_First),
          m_Last(other.m_Last)
    {
        other.m_Allocator = null;
        other.m_First     = null;
        other.m_Last      = null;
    }

    ~List()
    {
        Clear();
        m_Allocator = null;
    }

    constexpr u64 Count() const { return m_Count; }
    
    constexpr bool Empty() const { return !m_Count; }

    constexpr const Node *First() const { return m_First; }
    constexpr const Node *Last()  const { return m_Last;  }

    constexpr Node *First() { return m_First; }
    constexpr Node *Last()  { return m_Last;  }

    void ForEach(Function<void(const Node *)>&& callback) const
    {
        for (const Node *it = m_First; it; it = it->m_Next)
        {
            callback(it);
        }
    }

    void ForEach(Function<void(Node *)>&& callback)
    {
        for (Node *it = m_First; it; it = it->m_Next)
        {
            callback(it);
        }
    }

    void ForEachR(Function<void(const Node *)>&& callback) const
    {
        if constexpr (doubly_linked)
        {
            for (const Node *it = m_Last; it; it = it->m_Prev)
            {
                callback(it);
            }
        }
        else
        {
            static_assert(false, "Use DLinkedListNode as ListNode to be able to use ForEachR");
        }
    }

    void ForEachR(Function<void(Node *)>&& callback)
    {
        if constexpr (doubly_linked)
        {
            for (Node *it = m_Last; it; it = it->m_Prev)
            {
                callback(it);
            }
        }
        else
        {
            static_assert(false, "Use DLinkedListNode as ListNode to be able to use ForEachR");
        }
    }

    template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_move_assignable_v<T>>>
    void Insert(Node *where, U&&... elements)
    {
        (..., (InsertOne(where)->m_Data = RTTI::move(elements)));
    }

    template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_copy_assignable_v<T>>>
    void Insert(Node *where, const U&... elements)
    {
        (..., (InsertOne(where)->m_Data = elements));
    }

    template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_move_assignable_v<T>>> constexpr void PushFront(U&&... elements) { Insert(m_First, RTTI::forward<U>(elements)...); }
    template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_move_assignable_v<T>>> constexpr void PushBack(U&&... elements)  { Insert(null,    RTTI::forward<U>(elements)...); }

    template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_copy_assignable_v<T>>> constexpr void PushFront(const U&... elements) { Insert(m_First, elements...); }
    template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_copy_assignable_v<T>>> constexpr void PushBack(const U&... elements)  { Insert(null,    elements...); }

    template<typename ...ConstructorArgs, typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>>
    void Emplace(Node *where, ConstructorArgs&&... args)
    {
        InsertOne(where)->m_Data = T(RTTI::forward<ConstructorArgs>(args)...);
    }

    template<typename ...ConstructorArgs, typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>> constexpr void EmplaceFront(ConstructorArgs&&... args) { Emplace(m_First, RTTI::forward<ConstructorArgs>(args)...); }
    template<typename ...ConstructorArgs, typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>> constexpr void EmplaceBack(ConstructorArgs&&... args)  { Emplace(null,    RTTI::forward<ConstructorArgs>(args)...); }

    template<typename ...Nodes, typename = RTTI::enable_if_t<RTTI::are_same_v<Node, Nodes...>>>
    void Erase(Nodes *... what)
    {
        (..., EraseOne(what));
    }

    constexpr void EraseFront() { EraseOne(m_First); }
    constexpr void EraseBack()  { EraseOne(m_Back);  }

    void Clear()
    {
        for (Node *it = m_First; it; )
        {
            if constexpr (RTTI::is_destructible_v<T>)
            {
                it->m_Data.~T();
            }

            Node *next = it->m_Next;
            m_Allocator->DeAlloc(it);
            it = next;
        }

        m_Count = 0;
        m_First = null;
        m_Last  = null;
    }

    const Node *Find(const T& what) const
    {
        for (Node *it = m_First; it; it = it->m_Next)
        {
            if (it->m_Data == what)
            {
                return it;
            }
        }
        return null;
    }

    template<typename = RTTI::enable_if_t<doubly_linked>>
    const Node *RFind(const T& what) const
    {
        for (Node *it = m_Last; it; it = it->m_Prev)
        {
            if (it->m_Data == what)
            {
                return it;
            }
        }
        return null;
    }

    Node *Find(const T& what)
    {
        for (Node *it = m_First; it; it = it->m_Next)
        {
            if (it->m_Data == what)
            {
                return it;
            }
        }
        return null;
    }

    template<typename = RTTI::enable_if_t<doubly_linked>>
    Node *RFind(const T& what)
    {
        for (Node *it = m_Last; it; it = it->m_Prev)
        {
            if (it->m_Data == what)
            {
                return it;
            }
        }
        return null;
    }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<T>>>
    List& operator=(const List& other)
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

    List& operator=(List&& other) noexcept
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

    Node *operator[](u64 index)
    {
        CheckM(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
        Node *node = m_First;
        for (u64 i = 0; i < index; ++i)
        {
            node = node->m_Next;
        }
        return node;
    }

    const Node *operator[](u64 index) const
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
        if constexpr (doubly_linked)
        {
            Node *node = m_Allocator->Alloc<Node>();
            ++m_Count;

            node->m_Prev = where ? where->m_Prev : m_Last;
            node->m_Next = where;

            if (node->m_Prev    ) node->m_Prev->m_Next = node;
            if (where           ) where->m_Prev        = node;
            else                  m_Last               = node;
            if (where == m_First) m_First              = node;

            return node;
        }
        else
        {
            Node *node = m_Allocator->Alloc<Node>();
            ++m_Count;

            node->m_Next = where;

            if (!where)
            {
                if (m_Last) m_Last->m_Next = node;
                m_Last = node;
            }
            else
            {
                Node *prev = nullptr;
                for (Node *it = m_First; it && it != where; prev = it, it = it->m_Next)
                {
                }
                prev->m_Next = node;
            }

            if (where == m_First)
            {
                m_First = node;
            }

            return node;
        }
    }

    void EraseOne(Node *what)
    {
        if constexpr (doubly_linked)
        {
            Node *prev = what->m_Prev;
            Node *next = what->m_Next;

            if (prev) prev->m_Next = next;
            if (next) next->m_Prev = prev;

            if (what == m_First) m_First = next;
            if (what == m_Last ) m_Last  = prev;

            if constexpr (RTTI::is_destructible_v<T>)
            {
                what->m_Data.~T();
            }

            m_Allocator->DeAlloc(what);
            --m_Count;
        }
        else
        {
            Node *prev = nullptr;
            for (Node *it = m_First; it && it != what; prev = it, it = it->m_Next)
            {
            }

            Node *next = what->m_Next;

            if (prev) prev->m_Next = next;

            if (what == m_First) m_First = next;
            if (what == m_Last ) m_Last  = prev;

            if constexpr (RTTI::is_destructible_v<T>)
            {
                what->m_Data.~T();
            }

            m_Allocator->DeAlloc(what);
            --m_Count;
        }
    }

private:
    Allocator_t *m_Allocator;
    u64          m_Count;
    Node        *m_First;
    Node        *m_Last;
};
