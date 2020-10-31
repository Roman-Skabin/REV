//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#pragma pack(push, 8)
class ENGINE_API CriticalSection final
{
public:
    CriticalSection();
    CriticalSection(CriticalSection&& other);

    ~CriticalSection();

    void Enter();
    void Leave();

    CriticalSection& operator=(CriticalSection&& other);

private:
    CriticalSection(const CriticalSection&)            = delete;
    CriticalSection& operator=(const CriticalSection&) = delete;

private:
    RTL_CRITICAL_SECTION m_Handle;
};
#pragma pack(pop)
