// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/common.h"

namespace REV
{

//
// TupleNode
//

template<u64 index, typename T>
class TupleNode
{
public:
    using Type = T;

    template<typename = RTTI::enable_if_t<RTTI::is_default_constructible_v<T>>> constexpr REV_INLINE TupleNode()                       : m_Val()                        {}
    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<T>>>    constexpr REV_INLINE TupleNode(const T& val)           : m_Val(val)                     {}
    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<T>>>    constexpr REV_INLINE TupleNode(T&& val)                : m_Val(RTTI::move(val))         {}
    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<T>>>    constexpr REV_INLINE TupleNode(const TupleNode& other) : m_Val(other.m_Val)             {}
    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<T>>>    constexpr REV_INLINE TupleNode(TupleNode&& other)      : m_Val(RTTI::move(other.m_Val)) {}

    constexpr REV_INLINE const T&  Get() const &  { return            m_Val;  }
    constexpr REV_INLINE       T&  Get()       &  { return            m_Val;  }
    constexpr REV_INLINE const T&& Get() const && { return RTTI::move(m_Val); }
    constexpr REV_INLINE       T&& Get()       && { return RTTI::move(m_Val); }

    constexpr REV_INLINE u64 Index() const { return index; }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<T>>> constexpr REV_INLINE TupleNode& operator=(const T& val)           { m_Val = val;                     return *this; }
    template<typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>> constexpr REV_INLINE TupleNode& operator=(T&& val)                { m_Val = RTTI::move(val);         return *this; }
    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<T>>> constexpr REV_INLINE TupleNode& operator=(const TupleNode& other) { m_Val = other.m_Val;             return *this; }
    template<typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>> constexpr REV_INLINE TupleNode& operator=(TupleNode&& other)      { m_Val = RTTI::move(other.m_Val); return *this; }

    T m_Val;
};

//
// TupleImpl
//

template<u64 index, typename ...T>
class TupleImpl;

template<u64 index>
class TupleImpl<index>
{
public:
    constexpr REV_INLINE TupleImpl()                 {}
    constexpr REV_INLINE TupleImpl(const TupleImpl&) {}
    constexpr REV_INLINE TupleImpl(TupleImpl&&)      {}

    constexpr REV_INLINE u64 Count() const { return 0; }

    constexpr REV_INLINE bool Equals(const TupleImpl&)  const { return true;  }
    constexpr REV_INLINE bool Less(const TupleImpl&)    const { return false; }
    constexpr REV_INLINE bool Greater(const TupleImpl&) const { return false; }

    template<typename> constexpr REV_INLINE bool Has() const { return false; }

    template<typename Callback> constexpr REV_INLINE void ForEach(Callback&& callback) {}

    constexpr REV_INLINE TupleImpl& operator=(const TupleImpl&) { return *this; }
    constexpr REV_INLINE TupleImpl& operator=(TupleImpl&&)      { return *this; }
};

template<u64 index, typename First, typename ...Rest>
class TupleImpl<index, First, Rest...> : public  TupleNode<index, First>,
                                         private TupleImpl<index + 1, Rest...>
{
public:
    using BaseNode  = TupleNode<index, First>;
    using BaseTuple = TupleImpl<index + 1, Rest...>;

    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<First> && RTTI::are_copy_constructible_v<Rest...>>>
    constexpr REV_INLINE TupleImpl(const First& first, const Rest&... rest)
        : BaseNode(first),
          BaseTuple(rest...)
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<First> && RTTI::are_move_constructible_v<Rest>>>
    constexpr REV_INLINE TupleImpl(First&& first, Rest&&... rest)
        : BaseNode(RTTI::move(first)),
          BaseTuple(RTTI::forward<Rest>(rest)...)
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<First> && RTTI::are_copy_constructible_v<Rest...>>>
    constexpr REV_INLINE TupleImpl(const TupleImpl& other)
        : BaseNode(other.GetFirst()),
          BaseTuple(other.GetRest())
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<First> && RTTI::are_move_constructible_v<Rest>>>
    constexpr REV_INLINE TupleImpl(TupleImpl&& other)
        : BaseNode(other.GetFirst()),
          BaseTuple(other.GetRest())
    {
    }

    constexpr REV_INLINE const BaseTuple&  GetRest() const &  { return            *this;  }
    constexpr REV_INLINE       BaseTuple&  GetRest()       &  { return            *this;  }
    constexpr REV_INLINE const BaseTuple&& GetRest() const && { return RTTI::move(*this); }
    constexpr REV_INLINE       BaseTuple&& GetRest()       && { return RTTI::move(*this); }

    constexpr REV_INLINE const First&  GetFirst() const &  { return BaseNode::Get(); }
    constexpr REV_INLINE       First&  GetFirst()       &  { return BaseNode::Get(); }
    constexpr REV_INLINE const First&& GetFirst() const && { return BaseNode::Get(); }
    constexpr REV_INLINE       First&& GetFirst()       && { return BaseNode::Get(); }

    constexpr REV_INLINE u64 Count() const { return RTTI::sequence_count_v<First, Rest...>; }

    constexpr REV_INLINE bool Equals(const TupleImpl& other) const
    {
        return BaseNode::Get() == other.GetFirst()
            && BaseTuple::Equals(other.GetRest());
    }

    constexpr REV_INLINE bool Less(const TupleImpl& other) const
    {
        return (BaseNode::Get() <  other.GetFirst())
            || (BaseNode::Get() >= other.GetFirst() && BaseTuple::Less(other.GetRest()));
    }

    constexpr REV_INLINE bool Greater(const TupleImpl& other) const
    {
        return (BaseNode::Get() >  other.GetFirst())
            || (BaseNode::Get() <= other.GetFirst() && BaseTuple::Greater(other.GetRest()));
    }

    template<typename T>
    constexpr REV_INLINE bool Has() const
    {
        return RTTI::is_any_of_v<T, First, Rest...>;
    }

    template<u64 get_index, u64 count = RTTI::sequence_count_v<First, Rest...>>
    constexpr REV_INLINE const auto& Get() const &
    {
        if constexpr (get_index < count)
        {
            if constexpr (index == get_index) return BaseNode::Get();
            else                              return BaseTuple::Get<get_index, count>();
        }
        else
        {
            static_assert(false, "Index out of bound");
        }
    }

    template<u64 get_index, u64 count = RTTI::sequence_count_v<First, Rest...>>
    constexpr REV_INLINE auto& Get() &
    {
        if constexpr (get_index < count)
        {
            if constexpr (index == get_index) return BaseNode::Get();
            else                              return BaseTuple::Get<get_index, count>();
        }
        else
        {
            static_assert(false, "Index out of bound");
        }
    }

    template<u64 get_index, u64 count = RTTI::sequence_count_v<First, Rest...>>
    constexpr REV_INLINE const auto&& Get() const &&
    {
        if constexpr (get_index < count)
        {
            if constexpr (index == get_index) return BaseNode::Get();
            else                              return BaseTuple::Get<get_index, count>();
        }
        else
        {
            static_assert(false, "Index out of bound");
        }
    }

    template<u64 get_index, u64 count = RTTI::sequence_count_v<First, Rest...>>
    constexpr REV_INLINE auto&& Get() &&
    {
        if constexpr (get_index < count)
        {
            if constexpr (index == get_index) return BaseNode::Get();
            else                              return BaseTuple::Get<get_index, count>();
        }
        else
        {
            static_assert(false, "Index out of bound");
        }
    }

    template<typename T>
    constexpr REV_INLINE const auto& Get() const &
    {
        if constexpr (RTTI::is_any_of_v<T, First, Rest...>)
        {
            if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return BaseNode::Get();
            else                                              return BaseTuple::Get<T>();
        }
        else
        {
            static_assert(false, "There is no such type in tuple");
            return static_cast(const T&, T());
        }
    }

    template<typename T>
    constexpr REV_INLINE auto& Get() &
    {
        if constexpr (RTTI::is_any_of_v<T, First, Rest...>)
        {
            if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return BaseNode::Get();
            else                                              return BaseTuple::Get<T>();
        }
        else
        {
            static_assert(false, "There is no such type in tuple");
            return static_cast(T&, T());
        }
    }

    template<typename T>
    constexpr REV_INLINE const auto&& Get() const &&
    {
        if constexpr (RTTI::is_any_of_v<T, First, Rest...>)
        {
            if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return BaseNode::Get();
            else                                              return BaseTuple::Get<T>();
        }
        else
        {
            static_assert(false, "There is no such type in tuple");
            return static_cast(const T&&, T());
        }
    }

    template<typename T>
    constexpr REV_INLINE auto&& Get() &&
    {
        if constexpr (RTTI::is_any_of_v<T, First, Rest...>)
        {
            if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return BaseNode::Get();
            else                                              return BaseTuple::Get<T>();
        }
        else
        {
            static_assert(false, "There is no such type in tuple");
            return static_cast(T&&, T());
        }
    }

    template<typename Callback>
    constexpr REV_INLINE void ForEach(Callback&& callback) const
    {
        callback(index, BaseNode::Get());
        BaseTuple::ForEach(callback);
    }

    template<typename Callback>
    constexpr REV_INLINE void ForEach(Callback&& callback)
    {
        callback(index, BaseNode::Get());
        BaseTuple::ForEach(callback);
    }

    template<u64 ...indices>
    constexpr REV_INLINE TupleImpl<0, RTTI::get_sequence_type_t<indices, First, Rest...>...> SubTuple() const
    {
        return RTTI::move(TupleImpl<0, RTTI::get_sequence_type_t<indices, First, Rest...>...>(Get<indices>()...));
    }

    template<typename ...T>
    constexpr REV_INLINE TupleImpl<0, T...> SubTuple() const
    {
        return RTTI::move(TupleImpl<0, T...>(Get<T>()...));
    }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<First> && RTTI::are_copy_assignable_v<Rest...>>>
    constexpr REV_INLINE TupleImpl& operator=(const TupleImpl& other)
    {
        BaseNode::Get() = other.GetFirst();
        BaseTuple::operator=(other.GetRest());
        return *this;
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<First> && RTTI::are_move_assignable_v<Rest...>>>
    constexpr REV_INLINE TupleImpl& operator=(TupleImpl&& other)
    {
        BaseNode::Get() = other.GetFirst();
        BaseTuple::operator=(other.GetRest());
        return *this;
    }
};

template<u64 index, typename ...T> constexpr REV_INLINE bool operator==(const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return left.Equals(right);   }
template<u64 index, typename ...T> constexpr REV_INLINE bool operator!=(const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return !left.Equals(right);  }
template<u64 index, typename ...T> constexpr REV_INLINE bool operator< (const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return left.Less(right);     }
template<u64 index, typename ...T> constexpr REV_INLINE bool operator> (const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return left.Greater(right);  }
template<u64 index, typename ...T> constexpr REV_INLINE bool operator<=(const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return !left.Greater(right); }
template<u64 index, typename ...T> constexpr REV_INLINE bool operator>=(const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return !left.Less(right);    }

//
// Tuple
//

template<typename ...T>
using Tuple = TupleImpl<0, T...>;

}
