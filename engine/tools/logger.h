//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"

class ENGINE_API Logger final
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
        const char *name,
        const char *filename, // @NOTE(Roman): Required if you are logging to the file
        TARGET      target
    );

    Logger(const Logger& other, const char *name = null, TARGET target = TARGET::NONE);
    Logger(Logger&& other) noexcept;

    ~Logger();

    void LogVA(MESSAGE_KIND message_kind, const char *format, va_list args) const;

    void __cdecl Log(MESSAGE_KIND message_kind, const char *format, ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(message_kind, format, args);
        va_end(args);
    }

    void __cdecl LogInfo(const char *format, ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(MESSAGE_KIND::INFO, format, args);
        va_end(args);
    }

    void __cdecl LogSuccess(const char *format, ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(MESSAGE_KIND::SUCCESS, format, args);
        va_end(args);
    }

    void __cdecl LogWarning(const char *format, ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(MESSAGE_KIND::WARNING, format, args);
        va_end(args);
    }

    void __cdecl LogError(const char *format, ...) const
    {
        va_list args;
        va_start(args, format);
        LogVA(MESSAGE_KIND::ERROR, format, args);
        va_end(args);
    }

    Logger& operator=(const Logger& other);
    Logger& operator=(Logger&& other) noexcept;

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
