//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/logger.h"
#include "tools/critical_section.hpp"
#include "tools/static_string_builder.hpp"
#include <time.h>

namespace REV
{

REV_GLOBAL CriticalSection<false> g_CriticalSection;

Logger::Logger(const ConstString& name, const char *filename, TARGET target)
    : m_File(null),
      m_Console(null),
      m_Target(target),
      m_Attribs(),
      m_Name(name)
{
    if ((m_Target & TARGET::FILE) != TARGET::NONE)
    {
        REV_CHECK_M(filename, "filename is null. It is illegal if you want to log to a file.");

        // @Optimize(Roman): FILE_FLAG_NO_BUFFERING
        m_File = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_WRITE, null, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, null);

        if (m_File == INVALID_HANDLE_VALUE)
        {
            m_File    = null;
            m_Target &= ~TARGET::FILE;
            LogError("Failed to open log file");
        }
    }

    if ((m_Target & TARGET::CONSOLE) != TARGET::NONE)
    {
        m_Console = GetStdHandle(STD_OUTPUT_HANDLE);

        if (m_Console == INVALID_HANDLE_VALUE)
        {
            m_Console  = 0;
            m_Target  &= ~TARGET::CONSOLE;
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
      m_Console(null),
      m_Target(target != TARGET::NONE ? target : other.m_Target),
      m_Attribs(),
      m_Name(name.Length() ? name : other.m_Name)
{
    if ((m_Target & TARGET::FILE) != TARGET::NONE)
    {
        if ((other.m_Target & TARGET::FILE) != TARGET::NONE)
        {
            HANDLE process_handle = GetCurrentProcess();
            REV_DEBUG_RESULT(DuplicateHandle(process_handle,
                                             other.m_File,
                                             process_handle,
                                             &m_File,
                                             0,
                                             false,
                                             DUPLICATE_SAME_ACCESS));
        }
        else
        {
            REV_FAILED_M("Can not create targetted to a file %.*s because %.*s is not targetted to a file",
                         m_Name.Length(), m_Name.Data(),
                         other.m_Name.Length(), other.m_Name.Data());
        }
    }

    if ((m_Target & TARGET::CONSOLE) != TARGET::NONE)
    {
        m_Attribs = other.m_Attribs;
        m_Console = GetStdHandle(STD_OUTPUT_HANDLE);

        if (m_Console == INVALID_HANDLE_VALUE)
        {
            m_Console  = null;
            m_Target  &= ~TARGET::CONSOLE;
            LogError("Failed to open console handle");
        }
        else if (!m_Attribs.full)
        {
            CONSOLE_SCREEN_BUFFER_INFO info = {0};
            REV_DEBUG_RESULT(GetConsoleScreenBufferInfo(m_Console, &info));
            m_Attribs.full = info.wAttributes;
        }
    }

    LogSuccess(m_Name, " has been duplicated from %s");
}

Logger::Logger(Logger&& other) noexcept
    : m_File(other.m_File),
      m_Console(other.m_Console),
      m_Target(other.m_Target),
      m_Attribs(other.m_Attribs),
      m_Name(other.m_Name)
{
    other.m_File         = null;
    other.m_Console      = null;
    other.m_Target       = TARGET::NONE;
    other.m_Attribs.full = 0;
    other.m_Name         = null;
}

Logger::~Logger()
{
    if (m_File || m_Console || m_Target != TARGET::NONE)
    {
        LogInfo(m_Name, " has been destroyed");
    }

    if (m_File && (m_Target & TARGET::FILE) != TARGET::NONE)
    {
        REV_DEBUG_RESULT(CloseHandle(m_File));
    }

    m_File         = null;
    m_Console      = null;
    m_Target       = TARGET::NONE;
    m_Attribs.full = 0;
}

void Logger::PrintMessage(MESSAGE_KIND message_kind, const StaticString<1024>& message) const
{
    if (m_Target != TARGET::NONE)
    {
        const char *buffer_data   = message.Data();
        u32         buffer_length = cast<u32>(message.Length());

        g_CriticalSection.Enter();

        if (m_File && (m_Target & TARGET::FILE) != TARGET::NONE)
        {
            REV_DEBUG_RESULT(WriteFile(m_File, buffer_data, buffer_length, 0, 0));
            // @Optimize(Roman): Use unbuffered I/O instead of calling FlushFileBuffers
            REV_DEBUG_RESULT(FlushFileBuffers(m_File));
        }
        if (m_Console && (m_Target & TARGET::CONSOLE) != TARGET::NONE)
        {
            switch (message_kind)
            {
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
        if ((m_Target & TARGET::WINDBG) != TARGET::NONE)
        {
            OutputDebugStringA(buffer_data);
        }

        g_CriticalSection.Leave();
    }
}

Logger& Logger::operator=(const Logger& other)
{
    if (this != &other)
    {
        if ((other.m_Target & TARGET::FILE) != TARGET::NONE)
        {
            HANDLE process_handle = GetCurrentProcess();
            REV_DEBUG_RESULT(DuplicateHandle(process_handle,
                                             other.m_File,
                                             process_handle,
                                             &m_File,
                                             0,
                                             false,
                                             DUPLICATE_SAME_ACCESS));
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
        m_File               = other.m_File;
        m_Console            = other.m_Console;
        m_Target             = other.m_Target;
        m_Attribs            = other.m_Attribs;
        m_Name               = other.m_Name;
        other.m_File         = null;
        other.m_Console      = null;
        other.m_Target       = TARGET::NONE;
        other.m_Attribs.full = 0;
        other.m_Name         = null;
    }
    return *this;
}

}
