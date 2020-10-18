//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

class ENGINE_IMPEXP Event final
{
public:
    enum class FLAGS
    {
        NONE        = 0,
        RESETTABLE  = CREATE_EVENT_MANUAL_RESET,
        INITIAL_SET = CREATE_EVENT_INITIAL_SET,
    };

    Event(const char *name = null, FLAGS flags = FLAGS::NONE);
    Event(const Event& other);
    Event(Event&& other) noexcept;

    constexpr operator HANDLE() const { return m_Handle; }

    ~Event();

    bool Set();
    bool Reset();

    void Wait(u32 milliseconds = INFINITE, bool alerable = false) const;

    Event& operator=(const Event& other);
    Event& operator=(Event&& other) noexcept;

private:
    HANDLE m_Handle;
    FLAGS  m_Flags;
};

ENUM_CLASS_OPERATORS(Event::FLAGS);
