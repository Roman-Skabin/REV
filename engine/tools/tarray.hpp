//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

template<typename...>
class TArray;

template<>
class TArray<>
{
public:
    constexpr TArray()                  {}
    constexpr TArray(const TArray&)     {}
    constexpr TArray(TArray&&) noexcept {}

    constexpr TArray& operator=(const TArray&)     { return *this; }
    constexpr TArray& operator=(TArray&&) noexcept { return *this; }

    friend constexpr bool operator==(const TArray<>&, const TArray<>&) { return true;  }
    friend constexpr bool operator!=(const TArray<>&, const TArray<>&) { return false; }

    friend constexpr bool operator<(const TArray<>&, const TArray<>&) { return false; }
    friend constexpr bool operator>(const TArray<>&, const TArray<>&) { return false; }

    friend constexpr bool operator<=(const TArray<>&, const TArray<>&) { return true; }
    friend constexpr bool operator>=(const TArray<>&, const TArray<>&) { return true; }
};

template<typename First, typename ...Rest>
class TArray<First, Rest...> : private TArray<Rest...>
{
public:
    using Base = TArray<Rest...>;

    template<typename = RTTI::enable_if_t<RTTI::is_default_constructible_v<First> && RTTI::are_default_constructible_v<Rest...>>>
    constexpr TArray()
        : Base(),
          m_First()
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<First> && RTTI::are_copy_constructible_v<Rest...>>>
    constexpr TArray(const First& first, const Rest&... rest)
        : Base(rest...),
          m_First(first)
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<First> && RTTI::are_move_constructible_v<Rest...>>>
    constexpr TArray(First&& first, Rest&&... rest)
        : Base(RTTI::forward<Rest>(rest)...),
          m_First(RTTI::forward<First>(first))
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_constructible_v<First> && RTTI::are_copy_constructible_v<Rest...>>>
    constexpr TArray(const TArray& other)
        : Base(other.GetRest()...),
          m_First(other.m_First)
    {
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_constructible_v<First> && RTTI::are_move_constructible_v<Rest...>>>
    constexpr TArray(TArray&& other) noexcept
        : Base(RTTI::forware<Rest>(other.GetRest())...),
          m_First(RTTI::forware<First>(other.m_First))
    {
    }

    constexpr const Base& GetRest() const { return *this; }
    constexpr       Base& GetRest()       { return *this; }

    template<typename = RTTI::enable_if_t<RTTI::is_copy_assignable_v<First> && RTTI::are_copy_assignable_v<Rest...>>>
    constexpr TArray& operator=(const TArray& other)
    {
        GetRest() = other.GetRest();
        m_First   = other.m_First;
        return *this;
    }

    template<typename = RTTI::enable_if_t<RTTI::is_move_assignable_v<First> && RTTI::are_move_assignable_v<Rest...>>>
    constexpr TArray& operator=(TArray&& other) noexcept
    {
        GetRest() = RTTI::forward<TArray<Rest...>>(other.GetRest());
        m_First   = RTTI::forward<First>(other.m_First);
        return *this;
    }

    template<typename _First, typename ..._Rest>
    friend constexpr auto operator==(const TArray<_First, _Rest...>& left, const TArray<_First, _Rest...>& right)
        -> decltype(left.m_First == right.m_First)
    {
        return left.m_First   == right.m_First
            && left.GetRest() == right.GetRest();
    }

    template<typename _First, typename ..._Rest>
    friend constexpr auto operator!=(const TArray<_First, _Rest...>& left, const TArray<_First, _Rest...>& right)
        -> decltype(left.m_First != right.m_First)
    {
        return left.m_First   != right.m_First
            || left.GetRest() != right.GetRest();
    }

    template<typename _First, typename ..._Rest>
    friend constexpr auto operator<(const TArray<_First, _Rest...>& left, const TArray<_First, _Rest...>& right)
        -> decltype(left.m_First < right.m_First)
    {
        return left.m_First   < right.m_First
            && left.GetRest() < right.GetRest();
    }

    template<typename _First, typename ..._Rest>
    friend constexpr auto operator>(const TArray<_First, _Rest...>& left, const TArray<_First, _Rest...>& right)
        -> decltype(left.m_First > right.m_First)
    {
        return left.m_First   > right.m_First
            && left.GetRest() > right.GetRest();
    }

    template<typename _First, typename ..._Rest>
    friend constexpr auto operator<=(const TArray<_First, _Rest...>& left, const TArray<_First, _Rest...>& right)
        -> decltype(left.m_First <= right.m_First)
    {
        return left.m_First   <= right.m_First
            && left.GetRest() <= right.GetRest();
    }

    template<typename _First, typename ..._Rest>
    friend constexpr auto operator>=(const TArray<_First, _Rest...>& left, const TArray<_First, _Rest...>& right)
        -> decltype(left.m_First >= right.m_First)
    {
        return left.m_First   >= right.m_First
            && left.GetRest() >= right.GetRest();
    }

private:
    First m_First;
};
