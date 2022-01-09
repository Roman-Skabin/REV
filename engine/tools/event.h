// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/common.h"

namespace REV
{
    // @TODO(Roman): Wait for multiple events
    class REV_API Event final
    {
    public:
        enum class FLAGS : u32
        {
            NONE        = 0,
            RESETTABLE  = CREATE_EVENT_MANUAL_RESET,
            INITIAL_SET = CREATE_EVENT_INITIAL_SET,
        };

        Event(const char *name = null, FLAGS flags = FLAGS::NONE);
        Event(const Event& other);
        Event(Event&& other) noexcept;

        REV_INLINE const HANDLE Handle() const { return m_Handle; }
        REV_INLINE       HANDLE Handle()       { return m_Handle; }

        ~Event();

        bool Set();
        bool Reset();

        void Wait(u32 milliseconds = INFINITE) const;

        Event& operator=(const Event& other);
        Event& operator=(Event&& other) noexcept;

    private:
        HANDLE m_Handle;
        FLAGS  m_Flags;
    };

    REV_ENUM_CLASS_OPERATORS(Event::FLAGS);
}
