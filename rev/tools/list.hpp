//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "core/allocator.h"
#include "tools/function.hpp"

namespace REV
{
    template<typename T, bool doubly_linked>
    class List;

    template<typename T>
    class DLinkedListNode final
    {
    public:
        using Type = T;
    
        REV_INLINE DLinkedListNode()
            : m_Prev(null),
              m_Next(null),
              m_Data()
        {
        }

        REV_INLINE DLinkedListNode(const DLinkedListNode& other)
            : m_Prev(other.m_Prev),
              m_Next(other.m_Next),
              m_Data(other.m_Data)
        {
        }

        REV_INLINE DLinkedListNode(DLinkedListNode&& other) noexcept
            : m_Prev(other.m_Prev),
              m_Next(other.m_Next),
              m_Data(RTTI::move(other.m_Data))
        {
            other.m_Prev = null;
            other.m_Next = null;
        }

        REV_INLINE ~DLinkedListNode()
        {
            m_Prev = null;
            m_Next = null;
        }

        REV_INLINE DLinkedListNode *Prev() { return m_Prev; }
        REV_INLINE DLinkedListNode *Next() { return m_Next; }
        REV_INLINE T&               Data() { return m_Data; }

        REV_INLINE const DLinkedListNode *Prev() const { return m_Prev; }
        REV_INLINE const DLinkedListNode *Next() const { return m_Next; }
        REV_INLINE const T&               Data() const { return m_Data; }

        REV_INLINE DLinkedListNode& operator=(const DLinkedListNode& other)
        {
            if (this != &other)
            {
                m_Prev = other.m_Prev;
                m_Next = other.m_Next;
                m_Data = other.m_Data;
            }
            return *this;
        }
    
        REV_INLINE DLinkedListNode& operator=(DLinkedListNode&& other) noexcept
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
    
        friend class List<T, true>;
    };

    template<typename T>
    REV_INLINE auto operator==(const DLinkedListNode<T>& left, const DLinkedListNode<T>& right)
        -> decltype(left.Data() == right.Data())
    {
        return left.Prev() == right.Prev()
            && left.Next() == right.Next()
            && left.Data() == right.Data();
    }
    
    template<typename T>
    REV_INLINE auto operator!=(const DLinkedListNode<T>& left, const DLinkedListNode<T>& right)
        -> decltype(left.Data() != right.Data())
    {
        return left.Prev() != right.Prev()
            || left.Next() != right.Next()
            || left.Data() != right.Data();
    }
    
    template<typename T>
    class SLinkedListNode final
    {
    public:
        using Type = T;
    
        REV_INLINE SLinkedListNode()
            : m_Next(null),
              m_Data()
        {
        }
    
        REV_INLINE SLinkedListNode(const SLinkedListNode& other)
            : m_Next(other.m_Next),
              m_Data(other.m_Data)
        {
        }
    
        REV_INLINE SLinkedListNode(SLinkedListNode&& other) noexcept
            : m_Next(other.m_Next),
              m_Data(RTTI::move(other.m_Data))
        {
            other.m_Next = null;
        }
    
        REV_INLINE ~SLinkedListNode()
        {
            m_Next = null;
        }
    
        REV_INLINE SLinkedListNode *Next() { return m_Next; }
        REV_INLINE T&               Data() { return m_Data; }
    
        REV_INLINE const SLinkedListNode *Next() const { return m_Next; }
        REV_INLINE const T&               Data() const { return m_Data; }
    
        REV_INLINE SLinkedListNode& operator=(const SLinkedListNode& other)
        {
            if (this != &other)
            {
                m_Next = other.m_Next;
                m_Data = other.m_Data;
            }
            return *this;
        }
    
        REV_INLINE SLinkedListNode& operator=(SLinkedListNode&& other) noexcept
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
    
        friend class List<T, false>;
    };
    
    template<typename T>
    REV_INLINE auto operator==(const SLinkedListNode<T>& left, const SLinkedListNode<T>& right)
        -> decltype(left.Data() == right.Data())
    {
        return left.Next() == right.Next()
            && left.Data() == right.Data();
    }
    
    template<typename T>
    REV_INLINE auto operator!=(const SLinkedListNode<T>& left, const SLinkedListNode<T>& right)
        -> decltype(left.Data() != right.Data())
    {
        return left.Next() != right.Next()
            || left.Data() != right.Data();
    }
    
    template<typename T>
    struct SLinkedListIterator
    {
        using Node = SLinkedListNode<T>;
    
        Node *node;
    
        REV_INLINE SLinkedListIterator(Node *node)                       : node(node)       {}
        REV_INLINE SLinkedListIterator(const SLinkedListIterator& other) : node(other.node) {}

        REV_INLINE ~SLinkedListIterator() { node = null; }
    
        REV_INLINE SLinkedListIterator& operator=(Node *_node)                      { node = _node;      return *this; }
        REV_INLINE SLinkedListIterator& operator=(const SLinkedListIterator& other) { node = other.node; return *this; }
    
        REV_INLINE SLinkedListIterator& operator++() { if (node) node = node->next; return *this; }
    
        REV_INLINE SLinkedListIterator& operator++(int) { if (node) { SLinkedListIterator ret(node); node = node->next; return ret; } else { return *this; } }
    
        REV_INLINE const T& operator*() const { return node->Data(); }
        REV_INLINE       T& operator*()       { return node->Data(); }
    
        REV_INLINE const T *operator->() const { return &node->Data(); }
        REV_INLINE       T *operator->()       { return &node->Data(); }
    
        REV_INLINE bool operator==(SLinkedListIterator& other) const { return node == other.other; }
        REV_INLINE bool operator!=(SLinkedListIterator& other) const { return node != other.other; }
    };

    template<typename T>
    struct SLinkedListConstIterator
    {
        using Node = SLinkedListNode<T>;

        const Node *node;

        REV_INLINE SLinkedListConstIterator(const Node *node)                      : node(node)       {}
        REV_INLINE SLinkedListConstIterator(const SLinkedListConstIterator& other) : node(other.node) {}

        REV_INLINE ~SLinkedListConstIterator() { node = null; }

        REV_INLINE SLinkedListConstIterator& operator=(const Node *_node)                     { node = _node;      return *this; }
        REV_INLINE SLinkedListConstIterator& operator=(const SLinkedListConstIterator& other) { node = other.node; return *this; }

        REV_INLINE SLinkedListConstIterator& operator++() { if (node) node = node->next; return *this; }

        REV_INLINE SLinkedListConstIterator& operator++(int) { if (node) { SLinkedListConstIterator ret(node); node = node->next; return ret; } else { return *this; } }

        REV_INLINE const T& operator*() const { return node->Data(); }

        REV_INLINE const T *operator->() const { return &node->Data(); }

        REV_INLINE bool operator==(SLinkedListConstIterator& other) const { return node == other.other; }
        REV_INLINE bool operator!=(SLinkedListConstIterator& other) const { return node != other.other; }
    };

    template<typename T>
    struct DLinkedListIterator
    {
        using Node = DLinkedListNode<T>;

        Node *node;

        REV_INLINE DLinkedListIterator(Node *node)                       : node(node)       {}
        REV_INLINE DLinkedListIterator(const DLinkedListIterator& other) : node(other.node) {}

        REV_INLINE ~DLinkedListIterator() { node = null; }

        REV_INLINE DLinkedListIterator& operator=(Node *_node)                      { node = _node;      return *this; }
        REV_INLINE DLinkedListIterator& operator=(const DLinkedListIterator& other) { node = other.node; return *this; }

        REV_INLINE DLinkedListIterator& operator++() { if (node) node = node->next; return *this; }
        REV_INLINE DLinkedListIterator& operator--() { if (node) node = node->prev; return *this; }

        REV_INLINE DLinkedListIterator& operator++(int) { if (node) { DLinkedListIterator ret(node); node = node->next; return ret; } else { return *this; } }
        REV_INLINE DLinkedListIterator& operator--(int) { if (node) { DLinkedListIterator ret(node); node = node->prev; return ret; } else { return *this; } }

        REV_INLINE const T& operator*() const { return node->Data(); }
        REV_INLINE       T& operator*()       { return node->Data(); }

        REV_INLINE const T *operator->() const { return &node->Data(); }
        REV_INLINE       T *operator->()       { return &node->Data(); }

        REV_INLINE bool operator==(DLinkedListIterator& other) const { return node == other.other; }
        REV_INLINE bool operator!=(DLinkedListIterator& other) const { return node != other.other; }
    };

    template<typename T>
    struct DLinkedListConstIterator
    {
        using Node = DLinkedListNode<T>;

        const Node *node;

        REV_INLINE DLinkedListConstIterator(const Node *node)                      : node(node)       {}
        REV_INLINE DLinkedListConstIterator(const DLinkedListConstIterator& other) : node(other.node) {}

        REV_INLINE ~DLinkedListConstIterator() { node = null; }

        REV_INLINE DLinkedListConstIterator& operator=(const Node *_node)                     { node = _node;      return *this; }
        REV_INLINE DLinkedListConstIterator& operator=(const DLinkedListConstIterator& other) { node = other.node; return *this; }

        REV_INLINE DLinkedListConstIterator& operator++() { if (node) node = node->next; return *this; }
        REV_INLINE DLinkedListConstIterator& operator--() { if (node) node = node->prev; return *this; }

        REV_INLINE DLinkedListConstIterator& operator++(int) { if (node) { DLinkedListConstIterator ret(node); node = node->next; return ret; } else { return *this; } }
        REV_INLINE DLinkedListConstIterator& operator--(int) { if (node) { DLinkedListConstIterator ret(node); node = node->prev; return ret; } else { return *this; } }

        REV_INLINE const T& operator*() const { return node->Data(); }

        REV_INLINE const T *operator->() const { return &node->Data(); }

        REV_INLINE bool operator==(DLinkedListConstIterator& other) const { return node == other.other; }
        REV_INLINE bool operator!=(DLinkedListConstIterator& other) const { return node != other.other; }
    };

    template<typename T, bool doubly_linked = false>
    class List final
    {
    public:
        using Type          = T;
        using Node          = RTTI::conditional_t<doubly_linked, DLinkedListNode<T>, SLinkedListNode<T>>;
        using Iterator      = RTTI::conditional_t<doubly_linked, DLinkedListIterator<T>, SLinkedListIterator<T>>;
        using ConstIterator = RTTI::conditional_t<doubly_linked, DLinkedListConstIterator<T>, SLinkedListConstIterator<T>>;
    
    public:
        REV_INLINE explicit List(Allocator *allocator)
            : m_Allocator(allocator),
              m_Count(0),
              m_First(null),
              m_Last(null)
        {
            REV_CHECK(allocator);
        }
    
        template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<T>>>
        REV_INLINE List(const List& other)
            : m_Allocator(other.m_Allocator),
              m_Count(0),
              m_First(null),
              m_Last(null)
        {
            for (Node *it = other.m_First; it; it = it->m_Next)
            {
                PushBack(it->m_Data);
            }
        }
    
        REV_INLINE List(List&& other) noexcept
            : m_Allocator(other.m_Allocator),
              m_Count(other.m_Count),
              m_First(other.m_First),
              m_Last(other.m_Last)
        {
            other.m_Allocator = null;
            other.m_Count     = null;
            other.m_First     = null;
            other.m_Last      = null;
        }
    
        REV_INLINE ~List()
        {
            Clear();
            m_Allocator = null;
        }
    
        REV_INLINE u64 Count() const { return m_Count; }
        
        REV_INLINE bool Empty() const { return !m_Count; }
    
        REV_INLINE ConstIterator& begin()   const { return m_First; }
        REV_INLINE ConstIterator& cbegin()  const { return m_First; }
        REV_INLINE ConstIterator& rbegin()  const { return m_Last;  }
        REV_INLINE ConstIterator& crbegin() const { return m_Last;  }
    
        REV_INLINE ConstIterator& end()   const { return null; }
        REV_INLINE ConstIterator& cend()  const { return null; }
        REV_INLINE ConstIterator& rend()  const { return null; }
        REV_INLINE ConstIterator& crend() const { return null; }
    
        REV_INLINE Iterator& begin()  { return m_First; }
        REV_INLINE Iterator& rbegin() { return m_Last;  }
    
        REV_INLINE Iterator& end()  { return null; }
        REV_INLINE Iterator& rend() { return null; }
    
        REV_INLINE const Node *First() const { return m_First; }
        REV_INLINE const Node *Last()  const { return m_Last;  }
    
        REV_INLINE Node *First() { return m_First; }
        REV_INLINE Node *Last()  { return m_Last;  }
    
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
    
        template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_move_assignable_v<T>>> REV_INLINE void PushFront(U&&... elements) { Insert(m_First, RTTI::forward<U>(elements)...); }
        template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_move_assignable_v<T>>> REV_INLINE void PushBack(U&&... elements)  { Insert(null,    RTTI::forward<U>(elements)...); }
    
        template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_copy_assignable_v<T>>> REV_INLINE void PushFront(const U&... elements) { Insert(m_First, elements...); }
        template<typename ...U, typename = RTTI::enable_if_t<RTTI::are_same_v<T, U...> && RTTI::is_copy_assignable_v<T>>> REV_INLINE void PushBack(const U&... elements)  { Insert(null,    elements...); }
    
        template<typename ...ConstructorArgs, typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>>
        void Emplace(Node *where, ConstructorArgs&&... args)
        {
            InsertOne(where)->m_Data = T(RTTI::forward<ConstructorArgs>(args)...);
        }
    
        template<typename ...ConstructorArgs, typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>> REV_INLINE void EmplaceFront(ConstructorArgs&&... args) { Emplace(m_First, RTTI::forward<ConstructorArgs>(args)...); }
        template<typename ...ConstructorArgs, typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>> REV_INLINE void EmplaceBack(ConstructorArgs&&... args)  { Emplace(null,    RTTI::forward<ConstructorArgs>(args)...); }
    
        template<typename ...Nodes, typename = RTTI::enable_if_t<RTTI::are_same_v<Node, Nodes...>>>
        void Erase(Nodes *... what)
        {
            (..., EraseOne(what));
        }
    
        REV_INLINE void EraseFront() { EraseOne(m_First); }
        REV_INLINE void EraseBack()  { EraseOne(m_Back);  }
    
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
            REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
            Node *node = m_First;
            for (u64 i = 0; i < index; ++i)
            {
                node = node->m_Next;
            }
            return node;
        }
    
        const Node *operator[](u64 index) const
        {
            REV_CHECK_M(index < m_Count, "Expected max index: %I64u, got: %I64u", m_Count - 1, index);
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
                    Node *prev = null;
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
                Node *prev = null;
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
        Allocator *m_Allocator;
        u64        m_Count;
        Node      *m_First;
        Node      *m_Last;
    };
}
