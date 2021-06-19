//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/logger.h"
#include "tools/critical_section.hpp"
#include <time.h>

namespace REV
{

REV_GLOBAL CriticalSection<false> g_CriticalSection;

Logger::Logger(const char *name, const char *filename, TARGET target)
    : m_Name(name),
      m_File(null),
      m_Console(null),
      m_Target(target),
      m_Attribs()
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

    LogSuccess("%s has been created", m_Name);
}

Logger::Logger(const Logger& other, const char *name, TARGET target)
    : m_Name(name ? name : other.m_Name),
      m_File(null),
      m_Console(null),
      m_Target(target != TARGET::NONE ? target : other.m_Target),
      m_Attribs()
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
            REV_FAILED_M("Can not create targetted to a file %s because %s is not targetted to a file", m_Name, other.m_Name);
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

    LogSuccess("%s has been duplicated from %s", m_Name, other.m_Name);
}

Logger::Logger(Logger&& other) noexcept
    : m_Name(other.m_Name),
      m_File(other.m_File),
      m_Console(other.m_Console),
      m_Target(other.m_Target),
      m_Attribs(other.m_Attribs)
{
    other.m_Name         = null;
    other.m_File         = null;
    other.m_Console      = null;
    other.m_Target       = TARGET::NONE;
    other.m_Attribs.full = 0;
}

Logger::~Logger()
{
    if (m_File || m_Console || m_Target != TARGET::NONE)
    {
        LogInfo("%s has been destroyed", m_Name);
    }

    if (m_File && (m_Target & TARGET::FILE) != TARGET::NONE)
    {
        REV_DEBUG_RESULT(CloseHandle(m_File));
    }

    m_Name         = null;
    m_File         = null;
    m_Console      = null;
    m_Target       = TARGET::NONE;
    m_Attribs.full = 0;
}

void Logger::LogVA(MESSAGE_KIND message_kind, const char *format, va_list args) const
{
    if (m_Target != TARGET::NONE)
    {
        time_t raw_time;
        time(&raw_time);
        tm *timeinfo = localtime(&raw_time);
        REV_CHECK(timeinfo);

        char buffer[1024] = {'\0'};
        int  length       = 0;

        if (m_Name)
        {
            length = sprintf(buffer, "[%02I32d.%02I32d.%04I32d %02I32d:%02I32d:%02I32d]<%s>: ",
                             timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                             m_Name);
        }
        else
        {
            length = sprintf(buffer, "[%02I32d.%02I32d.%04I32d %02I32d:%02I32d:%02I32d]: ",
                             timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        }

        length += vsprintf(buffer + length, format, args);

        buffer[length++] = '\r';
        buffer[length++] = '\n';

        if (m_File && (m_Target & TARGET::FILE) != TARGET::NONE)
        {
            g_CriticalSection.Enter();
            REV_DEBUG_RESULT(WriteFile(m_File, buffer, length, 0, 0));
            // @Optimize(Roman): Use unbuffered I/O instead of calling FlushFileBuffers
            REV_DEBUG_RESULT(FlushFileBuffers(m_File));
            g_CriticalSection.Leave();
        }
        if (m_Console && (m_Target & TARGET::CONSOLE) != TARGET::NONE)
        {
            switch (message_kind)
            {
                case MESSAGE_KIND::INFO:
                {
                    g_CriticalSection.Enter();
                    REV_DEBUG_RESULT(WriteConsoleA(m_Console, buffer, length, 0, 0));
                    g_CriticalSection.Leave();
                } break;

                case MESSAGE_KIND::SUCCESS:
                {
                    g_CriticalSection.Enter();
                    REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, (m_Attribs.high << 8) | 0x0A));
                    REV_DEBUG_RESULT(WriteConsoleA(m_Console, buffer, length, 0, 0));
                    REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, m_Attribs.full));
                    g_CriticalSection.Leave();
                } break;

                case MESSAGE_KIND::WARNING:
                {
                    g_CriticalSection.Enter();
                    REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, (m_Attribs.high << 8) | 0x06));
                    REV_DEBUG_RESULT(WriteConsoleA(m_Console, buffer, length, 0, 0));
                    REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, m_Attribs.full));
                    g_CriticalSection.Leave();
                } break;

                case MESSAGE_KIND::ERROR:
                {
                    g_CriticalSection.Enter();
                    REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, (m_Attribs.high << 8) | 0x04));
                    REV_DEBUG_RESULT(WriteConsoleA(m_Console, buffer, length, 0, 0));
                    REV_DEBUG_RESULT(SetConsoleTextAttribute(m_Console, m_Attribs.full));
                    g_CriticalSection.Leave();
                } break;
            }
        }
        if ((m_Target & TARGET::WINDBG) != TARGET::NONE)
        {
            g_CriticalSection.Enter();
            OutputDebugStringA(buffer);
            g_CriticalSection.Leave();
        }
    }
}

Logger& Logger::operator=(const Logger& other)
{
    if (this != &other)
    {
        m_Name = other.m_Name;
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
    }
    return *this;
}

Logger& Logger::operator=(Logger&& other) noexcept
{
    if (this != &other)
    {
        m_Name               = other.m_Name;
        m_File               = other.m_File;
        m_Console            = other.m_Console;
        m_Target             = other.m_Target;
        m_Attribs            = other.m_Attribs;
        other.m_Name         = null;
        other.m_File         = null;
        other.m_Console      = null;
        other.m_Target       = TARGET::NONE;
        other.m_Attribs.full = 0;
    }
    return *this;
}

}
