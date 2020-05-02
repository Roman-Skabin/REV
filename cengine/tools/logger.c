//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/logger.h"
#include <time.h>

void CreateLogger(Logger *logger, const char *name, const char *filename, LOG_TO log_to)
{
    logger->log_to = log_to;
    logger->name   = name;

    if ((logger->log_to & LOG_TO_FILE) && filename)
    {
        logger->file = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);

        if (logger->file == INVALID_HANDLE_VALUE)
        {
            Error(logger, "Failed to open log file");
            logger->file = 0;
        }
    }

    if (logger->log_to & LOG_TO_CONSOLE)
    {
        logger->console = GetStdHandle(STD_OUTPUT_HANDLE);

        if (logger->console == INVALID_HANDLE_VALUE)
        {
            Error(logger, "Failed to open console handle");
            logger->console = 0;
        }
    }

    Success(logger, "%s was created", logger->name);
}

void DestroyLogger(Logger *logger)
{
    Log(logger, "%s was destroyed", logger->name);
    if (logger->file)    DebugResult(CloseHandle(logger->file));
    if (logger->console) DebugResult(CloseHandle(logger->console));
    ZeroMemory(logger, sizeof(Logger));
}

void __cdecl LoggerLog(Logger *logger, const char *format, ...)
{
    if (logger->log_to != LOG_TO_NONE)
    {
        va_list args;
        va_start(args, format);

        time_t raw_time;
        time(&raw_time);
        struct tm *timeinfo = localtime(&raw_time);
        Check(timeinfo);

        char buffer[BUFSIZ]  = {0};
        int  length          = sprintf(buffer, "[%02I32d:%02I32d:%02I32d]<%s>: ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, logger->name);
             length         += vsprintf(buffer + length, format, args);

        va_end(args);

        if (logger->file && logger->file != INVALID_HANDLE_VALUE && (logger->log_to & LOG_TO_FILE))
        {
            DebugResult(WriteFile(logger->file, buffer, length, 0, 0));
        }
        if (logger->console && logger->console != INVALID_HANDLE_VALUE && (logger->log_to & LOG_TO_CONSOLE))
        {
            DebugResult(WriteConsoleA(logger->console, buffer, length, 0, 0));
            DebugResult(FlushConsoleInputBuffer(logger->console));
        }
        if (logger->log_to & LOG_TO_DEBUG)
        {
            OutputDebugStringA(buffer);
        }
    }
}
