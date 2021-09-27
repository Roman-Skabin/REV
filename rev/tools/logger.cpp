// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "tools/logger.h"
#include "tools/critical_section.hpp"
#include "tools/static_string_builder.hpp"

namespace REV
{

REV_GLOBAL CriticalSection<false> g_CriticalSection;

Logger::Logger(const ConstString& name, const ConstString& filename, TARGET target)
    : m_File(),
      m_Console(INVALID_HANDLE_VALUE),
      m_Target(target),
      m_Attribs(),
      m_Name(name)
{
    if (m_Target & TARGET_FILE)
    {
        REV_CHECK_M(filename.Data(), "filename is null. It is illegal if you want to log to a file.");

        if (!m_File.Open(filename, FILE_FLAG_WRITE | FILE_FLAG_TRUNCATE | FILE_FLAG_SEQ | FILE_FLAG_FLUSH))
        {
            m_File    = null;
            m_Target &= ~TARGET_FILE;
            LogError("Failed to open log file");
        }
    }

    if (m_Target & TARGET_CONSOLE)
    {
        m_Console = GetStdHandle(STD_OUTPUT_HANDLE);

        if (!m_Console || m_Console == INVALID_HANDLE_VALUE)
        {
            m_Target  &= ~TARGET_CONSOLE;
            LogError("Failed to open console handle");
        }
        else
        {
            CONSOLE_SCREEN_BUFFER_INFO info = {0};
            REV_DEBUG_RESULT(GetConsoleScreenBufferInfo(m_Console, &info));
            m_Attribs.full = info.wAttributes;
        }
    }

    LogSuccess(m_Name, " has been created");
}

Logger::Logger(const Logger& other, const ConstString& name, TARGET target)
    : m_File(null),
      m_Console(INVALID_HANDLE_VALUE),
      m_Target(target != TARGET_NONE ? target : other.m_Target),
      m_Attribs(),
      m_Name(name.Length() ? name : other.m_Name)
{
    if (m_Target & TARGET_FILE)
    {
        if (other.m_Target & TARGET_FILE)
        {
            m_File = other.m_File;
        }
        else
        {
            REV_ERROR_M("Can not create %.*s targetted to a file because %.*s is not targetted to a file",
                         m_Name.Length(), m_Name.Data(),
                         other.m_Name.Length(), other.m_Name.Data());
        }
    }

    if (m_Target & TARGET_CONSOLE)
    {
        m_Attribs = other.m_Attribs;
        m_Console = GetStdHandle(STD_OUTPUT_HANDLE);

        if (!m_Console || m_Console == INVALID_HANDLE_VALUE)
        {
            m_Target  &= ~TARGET_CONSOLE;
            LogError("Failed to open console handle");
        }
        else if (!m_Attribs.full)
        {
            CONSOLE_SCREEN_BUFFER_INFO info = {0};
            REV_DEBUG_RESULT(GetConsoleScreenBufferInfo(m_Console, &info));
            m_Attribs.full = info.wAttributes;
        }
    }

    LogSuccess(m_Name, " has been duplicated from ", other.m_Name);
}

Logger::Logger(Logger&& other) noexcept
    : m_File(RTTI::move(other.m_File)),
      m_Console(other.m_Console),
      m_Target(other.m_Target),
      m_Attribs(other.m_Attribs),
      m_Name(other.m_Name)
{
    other.m_Console      = INVALID_HANDLE_VALUE;
    other.m_Target       = TARGET_NONE;
    other.m_Attribs.full = 0;
    other.m_Name         = null;
}

Logger::~Logger()
{
    LogInfo(m_Name, " has been destroyed");
    m_Console      = INVALID_HANDLE_VALUE;
    m_Target       = TARGET_NONE;
    m_Attribs.full = 0;
}

void Logger::PrintMessage(MESSAGE_KIND message_kind, const StaticString<1024>& message) const
{
    g_CriticalSection.Enter();

    if (m_Target & TARGET_FILE)
    {
        m_File.Write(message.Data(), message.Length());
    }
    if (m_Target & TARGET_CONSOLE)
    {
        const char *buffer_data   = message.Data();
        u32         buffer_length = cast(u32, message.Length());

        switch (message_kind)
        {
            case MESSAGE_KIND::DEBUG:
            case MESSAGE_KIND::INFO:
            {
                REV_DEBUG_RESULT(WriteConsoleA(m_Console, buffer_data, buffer_length, 0, 0));
            } break;

            case MESSAGE_KIND::SUCCESS:
            {
                REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, (m_Attribs.high << 8) | 0x0A));
                REV_DEBUG_RESULT(WriteConsoleA(m_Console, buffer_data, buffer_length, 0, 0));
                REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, m_Attribs.full));
            } break;

            case MESSAGE_KIND::WARNING:
            {
                REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, (m_Attribs.high << 8) | 0x06));
                REV_DEBUG_RESULT(WriteConsoleA(m_Console, buffer_data, buffer_length, 0, 0));
                REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, m_Attribs.full));
            } break;

            case MESSAGE_KIND::ERROR:
            {
                REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, (m_Attribs.high << 8) | 0x04));
                REV_DEBUG_RESULT(WriteConsoleA(m_Console, buffer_data, buffer_length, 0, 0));
                REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, m_Attribs.full));
            } break;
        }
    }
    if (m_Target & TARGET_WINDBG)
    {
        OutputDebugStringA(message.Data());
    }

    g_CriticalSection.Leave();
}

Logger& Logger::operator=(const Logger& other)
{
    if (this != &other)
    {
        if (other.m_Target & TARGET_FILE)
        {
            m_File = other.m_File;
        }
        m_Console = other.m_Console;
        m_Target  = other.m_Target;
        m_Attribs = other.m_Attribs;
        m_Name    = other.m_Name;
    }
    return *this;
}

Logger& Logger::operator=(Logger&& other) noexcept
{
    if (this != &other)
    {
        m_File               = RTTI::move(other.m_File);
        m_Console            = other.m_Console;
        m_Target             = other.m_Target;
        m_Attribs            = other.m_Attribs;
        m_Name               = other.m_Name;
        other.m_Console      = INVALID_HANDLE_VALUE;
        other.m_Target       = TARGET_NONE;
        other.m_Attribs.full = 0;
        other.m_Name         = null;
    }
    return *this;
}

}
