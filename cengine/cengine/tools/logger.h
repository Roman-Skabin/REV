//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

typedef enum LOG_TO
{
    LOG_TO_NONE,
    LOG_TO_FILE    = BIT(1),
    LOG_TO_CONSOLE = BIT(2),
    LOG_TO_DEBUG   = BIT(3),
} LOG_TO;

typedef struct Logger
{
    const char *name;
    HANDLE file;
    HANDLE console;
    LOG_TO log_to;
} Logger;

CEXTERN void         CreateLogger(Logger *logger, const char *name, const char *filename, LOG_TO log_to);
CEXTERN void         DestroyLogger(Logger *logger);
CEXTERN void __cdecl LoggerLog(Logger *logger, const char *format, ...);

#define Log(logger, format, ...)     LoggerLog(logger, format"\r\n", __VA_ARGS__)
#define Error(logger, format, ...)   LoggerLog(logger, "Error: "format"!!!\r\n", __VA_ARGS__)
#define Warning(logger, format, ...) LoggerLog(logger, "Warning: "format"!\r\n", __VA_ARGS__)
#define Success(logger, format, ...) LoggerLog(logger, "Success: "format"\r\n", __VA_ARGS__)
