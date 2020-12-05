//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"

#pragma pack(push, 1)
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

    template<typename = RTTI::enable_if_t<RTTI::is_default_constructible_v<T>>> constexpr TupleNode()                       : m_Val()                        {}
    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<T>>>    constexpr TupleNode(const T& val)           : m_Val(val)                     {}
    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<T>>>    constexpr TupleNode(T&& val)                : m_Val(RTTI::move(val))         {}
    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<T>>>    constexpr TupleNode(const TupleNode& other) : m_Val(other.m_Val)             {}
    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<T>>>    constexpr TupleNode(TupleNode&& other)      : m_Val(RTTI::move(other.m_Val)) {}

    constexpr const T&  Get() const &  { return            m_Val;  }
    constexpr       T&  Get()       &  { return            m_Val;  }
    constexpr const T&& Get() const && { return RTTI::move(m_Val); }
    constexpr       T&& Get()       && { return RTTI::move(m_Val); }

    constexpr u64 Index() const { return index; }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<T>>> constexpr TupleNode& operator=(const T& val)           { m_Val = val;                     return *this; }
    template<typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>> constexpr TupleNode& operator=(T&& val)                { m_Val = RTTI::move(val);         return *this; }
    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<T>>> constexpr TupleNode& operator=(const TupleNode& other) { m_Val = other.m_Val;             return *this; }
    template<typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<T>>> constexpr TupleNode& operator=(TupleNode&& other)      { m_Val = RTTI::move(other.m_Val); return *this; }

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
    constexpr TupleImpl()                 {}
    constexpr TupleImpl(const TupleImpl&) {}
    constexpr TupleImpl(TupleImpl&&)      {}

    constexpr u64 Count() const { return 0; }

    constexpr bool Equals(const TupleImpl&)  const { return true;  }
    constexpr bool Less(const TupleImpl&)    const { return false; }
    constexpr bool Greater(const TupleImpl&) const { return false; }

    template<typename> constexpr bool Has() const { return false; }

    template<typename Callback> constexpr void ForEach(Callback&& callback) {}

    constexpr TupleImpl& operator=(const TupleImpl&) { return *this; }
    constexpr TupleImpl& operator=(TupleImpl&&)      { return *this; }
};

template<u64 index, typename First, typename ...Rest>
class TupleImpl<index, First, Rest...> : public  TupleNode<index, First>,
                                         private TupleImpl<index + 1, Rest...>
{
public:
    using BaseNode  = TupleNode<index, First>;
    using BaseTuple = TupleImpl<index + 1, Rest...>;

    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<First> && RTTI::are_copy_constructible_v<Rest...>>>
    constexpr TupleImpl(const First& first, const Rest&... rest)
        : BaseNode(first),
          BaseTuple(rest...)
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<First> && RTTI::are_move_constructible_v<Rest>>>
    constexpr TupleImpl(First&& first, Rest&&... rest)
        : BaseNode(RTTI::move(first)),
          BaseTuple(RTTI::forward<Rest>(rest)...)
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<First> && RTTI::are_copy_constructible_v<Rest...>>>
    constexpr TupleImpl(const TupleImpl& other)
        : BaseNode(other.GetFirst()),
          BaseTuple(other.GetRest())
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<First> && RTTI::are_move_constructible_v<Rest>>>
    constexpr TupleImpl(TupleImpl&& other)
        : BaseNode(other.GetFirst()),
          BaseTuple(other.GetRest())
    {
    }

    constexpr const BaseTuple&  GetRest() const &  { return            *this;  }
    constexpr       BaseTuple&  GetRest()       &  { return            *this;  }
    constexpr const BaseTuple&& GetRest() const && { return RTTI::move(*this); }
    constexpr       BaseTuple&& GetRest()       && { return RTTI::move(*this); }

    constexpr const First&  GetFirst() const &  { return BaseNode::Get(); }
    constexpr       First&  GetFirst()       &  { return BaseNode::Get(); }
    constexpr const First&& GetFirst() const && { return BaseNode::Get(); }
    constexpr       First&& GetFirst()       && { return BaseNode::Get(); }

    constexpr u64 Count() const { return RTTI::sequence_count_v<First, Rest...>; }

    constexpr bool Equals(const TupleImpl& other) const
    {
        return BaseNode::Get() == other.GetFirst()
            && BaseTuple::Equals(other.GetRest());
    }

    constexpr bool Less(const TupleImpl& other) const
    {
        return (BaseNode::Get() <  other.GetFirst())
            || (BaseNode::Get() >= other.GetFirst() && BaseTuple::Less(other.GetRest()));
    }

    constexpr bool Greater(const TupleImpl& other) const
    {
        return (BaseNode::Get() >  other.GetFirst())
            || (BaseNode::Get() <= other.GetFirst() && BaseTuple::Greater(other.GetRest()));
    }

    template<typename T>
    constexpr bool Has() const
    {
        return RTTI::is_any_of_v<T, First, Rest...>;
    }

    template<u64 get_index, u64 count = RTTI::sequence_count_v<First, Rest...>>
    constexpr const auto& Get() const &
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
    constexpr auto& Get() &
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
    constexpr const auto&& Get() const &&
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
    constexpr auto&& Get() &&
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
    constexpr const auto& Get() const &
    {
        if constexpr (RTTI::is_any_of_v<T, First, Rest...>)
        {
            if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return BaseNode::Get();
            else                                              return BaseTuple::Get<T>();
        }
        else
        {
            static_assert(false, "There is no such type in tuple");
            return cast<const T&>(T());
        }
    }

    template<typename T>
    constexpr auto& Get() &
    {
        if constexpr (RTTI::is_any_of_v<T, First, Rest...>)
        {
            if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return BaseNode::Get();
            else                                              return BaseTuple::Get<T>();
        }
        else
        {
            static_assert(false, "There is no such type in tuple");
            return cast<T&>(T());
        }
    }

    template<typename T>
    constexpr const auto&& Get() const &&
    {
        if constexpr (RTTI::is_any_of_v<T, First, Rest...>)
        {
            if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return BaseNode::Get();
            else                                              return BaseTuple::Get<T>();
        }
        else
        {
            static_assert(false, "There is no such type in tuple");
            return cast<const T&&>(T());
        }
    }

    template<typename T>
    constexpr auto&& Get() &&
    {
        if constexpr (RTTI::is_any_of_v<T, First, Rest...>)
        {
            if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return BaseNode::Get();
            else                                              return BaseTuple::Get<T>();
        }
        else
        {
            static_assert(false, "There is no such type in tuple");
            return cast<T&&>(T());
        }
    }

    template<typename Callback>
    constexpr void ForEach(Callback&& callback) const
    {
        callback(index, BaseNode::Get());
        BaseTuple::ForEach(callback);
    }

    template<typename Callback>
    constexpr void ForEach(Callback&& callback)
    {
        callback(index, BaseNode::Get());
        BaseTuple::ForEach(callback);
    }

    template<u64 ...indices>
    constexpr TupleImpl<0, RTTI::get_sequence_type_t<indices, First, Rest...>...> SubTuple() const
    {
        return RTTI::move(TupleImpl<0, RTTI::get_sequence_type_t<indices, First, Rest...>...>(Get<indices>()...));
    }

    template<typename ...T>
    constexpr TupleImpl<0, T...> SubTuple() const
    {
        return RTTI::move(TupleImpl<0, T...>(Get<T>()...));
    }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<First> && RTTI::are_copy_assignable_v<Rest...>>>
    constexpr TupleImpl& operator=(const TupleImpl& other)
    {
        BaseNode::Get() = other.GetFirst();
        BaseTuple::operator=(other.GetRest());
        return *this;
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<First> && RTTI::are_move_assignable_v<Rest...>>>
    constexpr TupleImpl& operator=(TupleImpl&& other)
    {
        BaseNode::Get() = other.GetFirst();
        BaseTuple::operator=(other.GetRest());
        return *this;
    }
};

template<u64 index, typename ...T> constexpr bool operator==(const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return left.Equals(right);   }
template<u64 index, typename ...T> constexpr bool operator!=(const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return !left.Equals(right);  }
template<u64 index, typename ...T> constexpr bool operator< (const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return left.Less(right);     }
template<u64 index, typename ...T> constexpr bool operator> (const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return left.Greater(right);  }
template<u64 index, typename ...T> constexpr bool operator<=(const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return !left.Greater(right); }
template<u64 index, typename ...T> constexpr bool operator>=(const TupleImpl<index, T...>& left, const TupleImpl<index, T...>& right) { return !left.Less(right);    }

//
// Tuple
//

template<typename ...T>
using Tuple = TupleImpl<0, T...>;

}
#pragma pack(pop)
