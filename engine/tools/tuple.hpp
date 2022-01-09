// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/function.hpp"

namespace REV
{

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

    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool Equals(const TupleImpl<other_index, OtherTypes...>&) const { return false; }
    template<u64 other_index>                         constexpr REV_INLINE bool Equals(const TupleImpl<other_index>&)                const { return true;  }

    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool Less(const TupleImpl&) const { return true; }
    template<u64 other_index>                         constexpr REV_INLINE bool Less(const TupleImpl&) const { return false; }

    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool Greater(const TupleImpl&) const { return false; }
    template<u64 other_index>                         constexpr REV_INLINE bool Greater(const TupleImpl&) const { return false; }

    template<typename> constexpr REV_INLINE bool Has() const { return false; }

    template<typename Callback> constexpr REV_INLINE void ForEach(Callback&& callback) {}

    constexpr REV_INLINE TupleImpl& operator=(const TupleImpl&) { return *this; }
    constexpr REV_INLINE TupleImpl& operator=(TupleImpl&&)      { return *this; }

    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator==(const TupleImpl<other_index, OtherTypes...>& right) const { return  Equals(right);  }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator!=(const TupleImpl<other_index, OtherTypes...>& right) const { return !Equals(right);  }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator< (const TupleImpl<other_index, OtherTypes...>& right) const { return  Less(right);    }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator> (const TupleImpl<other_index, OtherTypes...>& right) const { return  Greater(right); }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator<=(const TupleImpl<other_index, OtherTypes...>& right) const { return !Greater(right); }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator>=(const TupleImpl<other_index, OtherTypes...>& right) const { return !Less(right);    }
};

template<u64 index, typename First, typename ...Rest>
class TupleImpl<index, First, Rest...> : private TupleImpl<index + 1, Rest...>
{
public:
    using FirstType = First;
    using BaseTuple = TupleImpl<index + 1, Rest...>;

    template<u64 ...indices>
    using SubTupleType = TupleImpl<0, RTTI::get_sequence_type_t<indices, First, Rest...>...>

    constexpr REV_INLINE TupleImpl()
        : BaseTuple(),
          m_First()
    {
        static_assert(RTTI::is_default_constructible_v<First>);
    }

    constexpr REV_INLINE TupleImpl(const First& first, const Rest&... rest)
        : BaseTuple(rest...),
          m_First(first)
    {
        static_assert(RTTI::is_copy_constructible_v<First>);
    }

    constexpr REV_INLINE TupleImpl(First&& first, Rest&&... rest)
        : BaseTuple(RTTI::move(rest)...),
          m_First(RTTI::move(first))
    {
        static_assert(RTTI::is_move_constructible_v<First>);
    }

    template<u64 other_index>
    constexpr REV_INLINE TupleImpl(const TupleImpl<other_index, First, Rest...>& other)
        : BaseTuple(other.GetRest()),
          m_First(other.m_First)
    {
        static_assert(RTTI::is_copy_constructible_v<First>);
    }

    template<u64 other_index>
    constexpr REV_INLINE TupleImpl(TupleImpl<other_index, First, Rest...>&& other)
        : BaseTuple(RTTI::move(other.GetRest())),
          m_First(RTTI::move(other.m_First))
    {
        static_assert(RTTI::is_move_constructible_v<First>);
    }

    constexpr REV_INLINE const BaseTuple& GetRest() const { return *this; }
    constexpr REV_INLINE       BaseTuple& GetRest()       { return *this; }
    constexpr REV_INLINE       u64        Count()   const { return RTTI::sequence_count_v<First, Rest...>; }

    template<u64 other_index, typename ...OtherTypes>
    constexpr REV_INLINE bool Equals(const TupleImpl<other_index, OtherTypes...>& other) const
    {
        if constexpr (RTTI::sequence_count_v<First, Rest...> != RTTI::sequence_count_v<OtherTypes...>)
        {
            return false;
        }
        else
        {
            using OtherTuple = TupleImpl<other_index, OtherTypes...>;
            static_assert(RTTI::are_comparable_eq_v<First, OtherTuple::FirstType>);

            return m_First == other.m_First
                && BaseTuple::Equals(other.GetRest());
        }
    }

    template<u64 other_index, typename ...OtherTypes>
    constexpr REV_INLINE bool Less(const TupleImpl<other_index, OtherTypes...>& other) const
    {
        if constexpr (RTTI::cmplt(RTTI::sequence_count_v<First, Rest...>, RTTI::sequence_count_v<OtherTypes...>))
        {
            return true;
        }
        else if constexpr (RTTI::cmpgt(RTTI::sequence_count_v<First, Rest...>, RTTI::sequence_count_v<OtherTypes...>))
        {
            return false;
        }
        else
        {
            using OtherTuple = TupleImpl<other_index, OtherTypes...>;
            static_assert(RTTI::are_comparable_lt_v<First, OtherTuple::FirstType>);
            static_assert(RTTI::are_comparable_le_v<First, OtherTuple::FirstType>);

            return (m_First <  other.m_First)
                || (m_First <= other.m_First && BaseTuple::Less(other.GetRest()));
        }
    }

    template<u64 other_index, typename ...OtherTypes>
    constexpr REV_INLINE bool Greater(const TupleImpl<other_index, OtherTypes...>& other) const
    {
        if constexpr (RTTI::cmplt(RTTI::sequence_count_v<First, Rest...>, RTTI::sequence_count_v<OtherTypes...>))
        {
            return false;
        }
        else if constexpr (RTTI::cmpgt(RTTI::sequence_count_v<First, Rest...>, RTTI::sequence_count_v<OtherTypes...>))
        {
            return true;
        }
        else
        {
            using OtherTuple = TupleImpl<other_index, OtherTypes...>;
            static_assert(RTTI::are_comparable_gt_v<First, OtherTuple::FirstType>);
            static_assert(RTTI::are_comparable_ge_v<First, OtherTuple::FirstType>);

            return (m_First >  other.m_First)
                || (m_First >= other.m_First && BaseTuple::Greater(other.GetRest()));
        }
    }

    template<typename T>
    constexpr REV_INLINE bool Has() const
    {
        return RTTI::is_any_of_v<T, First, Rest...>;
    }

    template<u64 get_index, u64 count = RTTI::sequence_count_v<First, Rest...>>
    constexpr REV_INLINE const auto& Get() const
    {
        static_assert(get_index < count, "Index out of bound");

        if constexpr (index == get_index) return m_First;
        else                              return BaseTuple::Get<get_index, count>();
    }

    template<u64 get_index, u64 count = RTTI::sequence_count_v<First, Rest...>>
    constexpr REV_INLINE auto& Get()
    {
        static_assert(get_index < count, "Index out of bound");

        if constexpr (index == get_index) return m_First;
        else                              return BaseTuple::Get<get_index, count>();
    }

    template<typename T>
    constexpr REV_INLINE const auto& Get() const
    {
        static_assert(RTTI::is_any_of_v<T, First, Rest...>, "There is no such type in tuple");

        if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return m_First;
        else                                              return BaseTuple::Get<T>();
    }

    template<typename T>
    constexpr REV_INLINE auto& Get()
    {
        static_assert(RTTI::is_any_of_v<T, First, Rest...>, "There is no such type in tuple");

        if constexpr (RTTI::is_same_v<T, BaseNode::Type>) return m_First;
        else                                              return BaseTuple::Get<T>();
    }

    template<u64 ...indices>
    constexpr REV_INLINE SubTupleType<indices...> SubTuple() const
    {
        return RTTI::move(SubTupleType<indices...>(Get<indices>()...));
    }

    template<typename ...T>
    constexpr REV_INLINE TupleImpl<0, T...> SubTuple() const
    {
        return RTTI::move(TupleImpl<0, T...>(Get<T>()...));
    }

    constexpr REV_INLINE TupleImpl& operator=(const TupleImpl& other)
    {
        static_assert(RTTI::is_copy_assignable_v<First>);

        m_First = other.m_First;
        BaseTuple::operator=(other.GetRest());
        return *this;
    }

    constexpr REV_INLINE TupleImpl& operator=(TupleImpl&& other)
    {
        static_assert(RTTI::is_move_assignable_v<First>);

        m_First = other.m_First;
        BaseTuple::operator=(other.GetRest());
        return *this;
    }

    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator==(const TupleImpl<other_index, OtherTypes...>& right) const { return  Equals(right);  }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator!=(const TupleImpl<other_index, OtherTypes...>& right) const { return !Equals(right);  }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator< (const TupleImpl<other_index, OtherTypes...>& right) const { return  Less(right);    }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator> (const TupleImpl<other_index, OtherTypes...>& right) const { return  Greater(right); }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator<=(const TupleImpl<other_index, OtherTypes...>& right) const { return !Greater(right); }
    template<u64 other_index, typename ...OtherTypes> constexpr REV_INLINE bool operator>=(const TupleImpl<other_index, OtherTypes...>& right) const { return !Less(right);    }

    First m_First;
};
template<u64 index, typename ...T> TupleImpl(T...) -> TupleImpl<index, T...>;

//
// Tuple
//

template<typename ...T>
using Tuple = TupleImpl<0, T...>;

}
