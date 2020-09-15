//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

class ENGINE_IMPEXP Logger final
{
public:
    enum class TARGET
    {
        NONE    = 0,
        FILE    = BIT(0),
        CONSOLE = BIT(1),
        WINDBG  = BIT(2),
    };

    enum class MESSAGE_KIND
    {
        INFO,
        SUCCESS,
        WARNING,
        ERROR,
    };

public:
    Logger(
        in  const char *name,
        opt const char *filename, // @NOTE(Roman): Required if you are logging to the file
        in  TARGET      target
    );

    Logger(in const Logger& other, opt const char *name = null, opt TARGET target = TARGET::NONE);
    Logger(in Logger&& other) noexcept;

    ~Logger();

    void LogVA(in MESSAGE_KIND message_kind, in const char *format, opt va_list args) const;

    void __cdecl Log(in MESSAGE_KIND message_kind, in const char *format, opt ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(message_kind, format, args);
        va_end(args);
    }

    void __cdecl LogInfo(in const char *format, opt ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(MESSAGE_KIND::INFO, format, args);
        va_end(args);
    }

    void __cdecl LogSuccess(in const char *format, opt ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(MESSAGE_KIND::SUCCESS, format, args);
        va_end(args);
    }

    void __cdecl LogWarning(in const char *format, opt ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(MESSAGE_KIND::WARNING, format, args);
        va_end(args);
    }

    void __cdecl LogError(in const char *format, opt ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(MESSAGE_KIND::ERROR, format, args);
        va_end(args);
    }

    Logger& operator=(in const Logger& other);
    Logger& operator=(in Logger&& other) noexcept;

private:
    const char *m_Name;
    HANDLE      m_File;
    HANDLE      m_Console;
    TARGET      m_Target;
    union
    {
        u16 full;
        struct
        {
            u8 high;
            u8 low;
        };
    } m_Attribs;

    static SRWLOCK s_SRWLock;
};

ENUM_CLASS_OPERATORS(Logger::TARGET)
