// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/const_string.h"
#include "tools/static_string_builder.hpp"
#include "tools/file.h"

namespace REV
{
    // @TODO(Roman): Singleton?
    class REV_API Logger final
    {
    public:
        enum TARGET : u32
        {
            TARGET_NONE    = 0,
            TARGET_FILE    = 1 << 0,
            TARGET_CONSOLE = 1 << 1,
            TARGET_WINDBG  = 1 << 2,
        };

        enum class MESSAGE_KIND : u32
        {
            DEBUG,
            INFO,
            SUCCESS,
            WARNING,
            ERROR,
        };

    public:
        Logger(
            const ConstString& name,
            const ConstString& filename, // @NOTE(Roman): Required if you are logging to the file
            TARGET             target
        );
        Logger(const Logger& other, const ConstString& name = null, TARGET target = TARGET_NONE);
        Logger(Logger&& other) noexcept;

        ~Logger();

        template<typename ...T>
        REV_INLINE void REV_CDECL Log(MESSAGE_KIND message_kind, const T& ...args) const
        {
            #if !REV_DEBUG
                if (message_kind == MESSAGE_KIND::DEBUG) return;
            #endif
            PrintMessage(message_kind, ConstructMessage(message_kind, args...));
        }

        template<typename ...T> REV_INLINE void REV_CDECL LogDebug(const T& ...args)   const { Log(MESSAGE_KIND::DEBUG,   args...); }
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
            // [dd.mm.yyyy hh:mm:ss]<LoggerName>(MessageKind): Message.

            SYSTEMTIME time{0};
            GetLocalTime(&time);

            StaticStringBuilder<1024> builder;

            builder.m_IntFormat.Fill  = '0';
            builder.m_IntFormat.Width = 2;
            builder.Build('[', time.wDay, '.', time.wMonth, '.');

            builder.m_IntFormat.Width = 4;
            builder.Build(time.wYear, ' ');

            builder.m_IntFormat.Width = 2;
            builder.Build(time.wHour, ':', time.wMinute, ':', time.wSecond, ']');

            builder.m_IntFormat.Fill  = ' ';
            builder.m_IntFormat.Width = 0;

            if (m_Name.Length()) builder.Build('<', m_Name, '>');

            switch (message_kind)
            {
                case MESSAGE_KIND::DEBUG:   builder.Build("(Debug): ");                              break;
                case MESSAGE_KIND::INFO:    builder.Build("(Info): ");                               break;
                case MESSAGE_KIND::SUCCESS: builder.Build("(Success): ");                            break;
                case MESSAGE_KIND::WARNING: builder.Build("(Warning): ");                            break;
                case MESSAGE_KIND::ERROR:   builder.Build("(Error): ");                              break;
                default:                    REV_ERROR_M("Wrong MESSAGE_KIND: %I32u", message_kind); break;
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
        mutable File m_File;
        HANDLE       m_Console;
        TARGET       m_Target;
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

    REV_ENUM_OPERATORS(Logger::TARGET)
}
