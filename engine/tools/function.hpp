//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

template<typename Ret, typename ...Args>
class Function;

template<typename Ret, typename ...Args>
class Function<Ret(Args...)>
{
public:
    using FunctionType = Ret(Args...);

    Function()
        : m_FunctionPointer(nullptr)
    {
    }

    Function(nullptr_t)
        : m_FunctionPointer(nullptr)
    {
    }

    Function(FunctionType *function)
        : m_FunctionPointer(function)
    {
    }

    // @TODO(Roman): Type safety
    template<typename Lambda>
    Function(Lambda&& lambda)
        : m_FunctionPointer(cast<FunctionType *>(&lambda))
    {
    }

    Function(const Function& other)
        : m_FunctionPointer(other.m_FunctionPointer)
    {
    }

    Function(Function&& other) noexcept
        : m_FunctionPointer(other.m_FunctionPointer)
    {
        other.m_FunctionPointer = nullptr;
    }

    ~Function()
    {
        m_FunctionPointer = nullptr;
    }

    Function& operator=(const Function& other)
    {
        m_FunctionPointer = other.m_FunctionPointer;
    }

    Function& operator=(Function&& other) noexcept
    {
        m_FunctionPointer       = other.m_FunctionPointer;
        other.m_FunctionPointer = nullptr;
    }

    Ret operator()(const Args&... args) const
    {
        if (m_FunctionPointer)
        {
            return m_FunctionPointer(args...);
        }
    }

    Ret operator()(Args&&... args) const
    {
        if (m_FunctionPointer)
        {
            return m_FunctionPointer(RTTI::forward<Args>(args)...);
        }
    }

private:
    FunctionType *m_FunctionPointer;
};
