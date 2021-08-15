//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "tools/const_string.h"
#include "tools/static_string_builder.hpp"

namespace REV
{
    // @TODO(Roman): Singleton?
    class REV_API Logger final
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
            const ConstString& name,
            const char        *filename, // @NOTE(Roman): Required if you are logging to the file
            TARGET             target
        );
        Logger(const Logger& other, const ConstString& name = null, TARGET target = TARGET::NONE);
        Logger(Logger&& other) noexcept;

        ~Logger();

        template<typename ...T>
        REV_INLINE void REV_CDECL Log(MESSAGE_KIND message_kind, const T& ...args) const
        {
            PrintMessage(message_kind, ConstructMessage(message_kind, args...));
        }

        template<typename ...T> REV_INLINE void REV_CDECL LogInfo(const T& ...args)    const { Log(MESSAGE_KIND::INFO,    args...); }
        template<typename ...T> REV_INLINE void REV_CDECL LogSuccess(const T& ...args) const { Log(MESSAGE_KIND::SUCCESS, args...); }
        template<typename ...T> REV_INLINE void REV_CDECL LogWarning(const T& ...args) const { Log(MESSAGE_KIND::WARNING, args...); }
        template<typename ...T> REV_INLINE void REV_CDECL LogError(const T& ...args)   const { Log(MESSAGE_KIND::ERROR,   args...); }

        Logger& operator=(const Logger& other);
        Logger& operator=(Logger&& other) noexcept;

    private:
        template<typename ...T>
        StaticString<1024> REV_CDECL ConstructMessage(MESSAGE_KIND message_kind, const T& ...args) const
        {
            // [dd.mm.yyyy hh:mm:ss]<LoggerName>(MessageKind): Mesesage.

            time_t raw_time;
            time(&raw_time);
            tm *timeinfo = localtime(&raw_time);
            REV_CHECK(timeinfo);

            StaticStringBuilder<1024> builder;

            builder.m_IntFormat.Fill  = '0';
            builder.m_IntFormat.Width = 2;
            builder.Build('[', timeinfo->tm_mday, '.', timeinfo->tm_mon + 1, '.');

            builder.m_IntFormat.Width = 4;
            builder.Build(timeinfo->tm_year + 1900, ' ');

            builder.m_IntFormat.Width = 2;
            builder.Build(timeinfo->tm_hour, ':', timeinfo->tm_min, ':', timeinfo->tm_sec, ']');

            builder.m_IntFormat.Fill  = ' ';
            builder.m_IntFormat.Width = 0;

            if (m_Name.Length()) builder.Build('<', m_Name, '>');

            switch (message_kind)
            {
                case MESSAGE_KIND::INFO:    builder.Build(ConstString(REV_CSTR_ARGS("(Info): ")));    break;
                case MESSAGE_KIND::SUCCESS: builder.Build(ConstString(REV_CSTR_ARGS("(Success): "))); break;
                case MESSAGE_KIND::WARNING: builder.Build(ConstString(REV_CSTR_ARGS("(Warning): "))); break;
                case MESSAGE_KIND::ERROR:   builder.Build(ConstString(REV_CSTR_ARGS("(Error): ")));   break;
                default:                    REV_FAILED_M("Wrong MESSAGE_KIND: %I32u", message_kind);  break;
            }

            builder.Build(args...);

            switch (message_kind)
            {
                case MESSAGE_KIND::ERROR: builder.BuildLn('!'); break;
                default:                  builder.BuildLn('.'); break;
            }

            return builder.ToString();
        }

        void PrintMessage(MESSAGE_KIND message_kind, const StaticString<1024>& message) const;

    private:
        HANDLE m_File;
        HANDLE m_Console;
        TARGET m_Target;
        union
        {
            u16 full;
            struct
            {
                u8 high;
                u8 low;
            };
        } m_Attribs;
        StaticString<CACHE_LINE_SIZE> m_Name;
    };

    REV_ENUM_CLASS_OPERATORS(Logger::TARGET)
}
