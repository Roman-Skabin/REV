//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/logger.h"
#include <time.h>

global SRWLOCK gSRWLock = SRWLOCK_INIT;

void CreateLogger(
    IN       const char *name,
    OPTIONAL const char *filename,
    IN       LOG_TO      log_to,
    OUT      Logger     *logger)
{
    Check(name);
    Check(logger);

    logger->name    = name;
    logger->file    = 0;
    logger->console = 0;
    logger->log_to  = log_to;

    if ((logger->log_to & LOG_TO_FILE) && filename)
    {
        // @Optimize(Roman): FILE_FLAG_NO_BUFFERING
        logger->file = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_WRITE, null, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, null);

        if (logger->file == INVALID_HANDLE_VALUE)
        {
            logger->file    = 0;
            logger->log_to &= ~LOG_TO_FILE;
            LogError(logger, "Failed to open log file");
        }
    }

    if (logger->log_to & LOG_TO_CONSOLE)
    {
        logger->console = GetStdHandle(STD_OUTPUT_HANDLE);

        if (logger->console == INVALID_HANDLE_VALUE)
        {
            logger->console  = 0;
            logger->log_to  &= ~LOG_TO_CONSOLE;
            LogError(logger, "Failed to open console handle");
        }
        else
        {
            CONSOLE_SCREEN_BUFFER_INFO info = {0};
            DebugResult(GetConsoleScreenBufferInfo(logger->console, &info));
            logger->attribs.full = info.wAttributes;
        }
    }

    LogSuccess(logger, "%s was created", logger->name);
}

void DuplicateLogger(
    IN  const char *name,
    IN  LOG_TO      log_to,
    IN  Logger     *src,
    OUT Logger     *dest)
{
    Check(name);
    CheckM(log_to & LOG_TO_FILE, "<log_to> param required LOG_TO_FILE flag, got: 0x%08I32X", log_to);
    Check(src);
    CheckM(src->log_to & LOG_TO_FILE, "%s (src) required LOG_TO_FILE flag, got: 0x%I32X", src->name, src->log_to);
    CheckM(src->file, "%s's (src) file is <null>", src->name);
    Check(dest);

    dest->name         = name;
    dest->file         = 0;
    dest->console      = 0;
    dest->log_to       = log_to;
    dest->attribs.full = src->attribs.full;

    HANDLE process_handle = GetCurrentProcess();
    DebugResult(DuplicateHandle(process_handle,
                                src->file,
                                process_handle,
                                &dest->file,
                                0,
                                false,
                                DUPLICATE_SAME_ACCESS));

    if (dest->log_to & LOG_TO_CONSOLE)
    {
        dest->console = GetStdHandle(STD_OUTPUT_HANDLE);

        if (dest->console == INVALID_HANDLE_VALUE)
        {
            dest->console  = 0;
            dest->log_to  &= ~LOG_TO_CONSOLE;
            LogError(dest, "Failed to open console handle");
        }
        else if (!dest->attribs.full)
        {
            CONSOLE_SCREEN_BUFFER_INFO info = {0};
            DebugResult(GetConsoleScreenBufferInfo(dest->console, &info));
            dest->attribs.full = info.wAttributes;
        }
    }

    LogSuccess(dest, "%s has been duplicated from %s", dest->name, src->name);
}

void DestroyLogger(
    IN Logger *logger)
{
    Check(logger);
    LogInfo(logger, "%s was destroyed", logger->name);

    if (logger->file && (logger->log_to & LOG_TO_FILE))
    {
        DebugResult(CloseHandle(logger->file));
    }
    if (logger->console && (logger->log_to & LOG_TO_CONSOLE))
    {
        DebugResult(CloseHandle(logger->console));
    }

    ZeroMemory(logger, sizeof(Logger));
}

void __cdecl LoggerLog(
    IN       Logger     *logger,
    IN       LOG_KIND    log_kind,
    IN       const char *format,
    OPTIONAL ...)
{
    Check(logger);
    Check(format);

    if (logger->log_to)
    {
        time_t raw_time;
        time(&raw_time);
        struct tm *timeinfo = localtime(&raw_time);
        Check(timeinfo);

        va_list args;
        va_start(args, format);

        char buffer[1024] = {0};
        int  length = sprintf(buffer, "[%02I32d.%02I32d.%04I32d %02I32d:%02I32d:%02I32d]<%s>: ",
                              timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900,
                              timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
                              logger->name);

        length += vsprintf(buffer + length, format, args);

        buffer[length++] = '\r';
        buffer[length++] = '\n';

        va_end(args);

        if (logger->file && (logger->log_to & LOG_TO_FILE))
        {
            AcquireSRWLockExclusive(&gSRWLock);
            DebugResult(WriteFile(logger->file, buffer, length, 0, 0));
            // @Optimize(Roman): Use unbuffered I/O instead of calling FlushFileBuffers
            DebugResult(FlushFileBuffers(logger->file));
            ReleaseSRWLockExclusive(&gSRWLock);
        }
        if (logger->console && (logger->log_to & LOG_TO_CONSOLE))
        {
            switch (log_kind)
            {
                case LOG_KIND_INFO:
                {
                    AcquireSRWLockExclusive(&gSRWLock);
                    DebugResult(WriteConsoleA(logger->console, buffer, length, 0, 0));
                    ReleaseSRWLockExclusive(&gSRWLock);
                } break;

                case LOG_KIND_SUCCESS:
                {
                    AcquireSRWLockExclusive(&gSRWLock);
                    DebugResult(SetConsoleTextAttribute(logger->console, (logger->attribs.high << 8) | 0xA));
                    DebugResult(WriteConsoleA(logger->console, buffer, length, 0, 0));
                    DebugResult(SetConsoleTextAttribute(logger->console, logger->attribs.full));
                    ReleaseSRWLockExclusive(&gSRWLock);
                } break;

                case LOG_KIND_WARNING:
                {
                    AcquireSRWLockExclusive(&gSRWLock);
                    DebugResult(SetConsoleTextAttribute(logger->console, (logger->attribs.high << 8) | 0x6));
                    DebugResult(WriteConsoleA(logger->console, buffer, length, 0, 0));
                    DebugResult(SetConsoleTextAttribute(logger->console, logger->attribs.full));
                    ReleaseSRWLockExclusive(&gSRWLock);
                } break;

                case LOG_KIND_ERROR:
                {
                    AcquireSRWLockExclusive(&gSRWLock);
                    DebugResult(SetConsoleTextAttribute(logger->console, (logger->attribs.high << 8) | 0x4));
                    DebugResult(WriteConsoleA(logger->console, buffer, length, 0, 0));
                    DebugResult(SetConsoleTextAttribute(logger->console, logger->attribs.full));
                    ReleaseSRWLockExclusive(&gSRWLock);
                } break;
            }
        }
        if (logger->log_to & LOG_TO_DEBUG)
        {
            AcquireSRWLockExclusive(&gSRWLock);
            OutputDebugStringA(buffer);
            ReleaseSRWLockExclusive(&gSRWLock);
        }
    }
}
