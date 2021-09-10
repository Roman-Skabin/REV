//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/common.h"
#include "memory/memory.h"
#include "tools/const_string.h"
#include "tools/static_string_builder.hpp"

namespace REV
{

REV_GLOBAL HANDLE                 g_Console         = GetStdHandle(STD_OUTPUT_HANDLE);
REV_GLOBAL CriticalSection<false> g_CriticalSection;

void REV_CDECL PrintDebugMessage(DEBUG_COLOR color, const char *format, ...)
{
    REV_CHECK(format);

    StaticStringBuilder<2048> builder;

    va_list args;
    va_start(args, format);

    if (args) builder.BuildVA(format, args);
    else      builder.Build(format);

    va_end(args);

    builder.BuildLn('.');

    g_CriticalSection.Enter();
    {
    #if REV_PLATFORM_WIN64
        REV_DEBUG_RESULT(SetConsoleTextAttribute(g_Console, cast(u16, color)));
        REV_DEBUG_RESULT(WriteConsoleA(g_Console, builder.BufferData(), (u32)builder.BufferLength(), null, null));
        REV_DEBUG_RESULT(SetConsoleTextAttribute(g_Console, cast(u16, DEBUG_COLOR::INFO)));
        OutputDebugStringA(builder.BufferData());
    #else
        REV_ERROR_M("Unhandled platform dependent code!");
    #endif
    }
    g_CriticalSection.Leave();
}

void REV_CDECL PrintDebugMessage(const char *file, u64 line, DEBUG_COLOR color, bool print_sys_error, const char *format, ...)
{
    // @NOTE(Roman): file(line): type: message. System error [sys_error_code]: sys_error_message.
    // @NOTE(Roman): file(line): type: message.

    StaticStringBuilder<2048> builder;

    builder.Build(file, '(', line, "): ");

    switch (color)
    {
        case DEBUG_COLOR::INFO:    builder.Build("Info: ");    break;
        case DEBUG_COLOR::ERROR:   builder.Build("Error: ");   break;
        case DEBUG_COLOR::WARNING: builder.Build("Warning: "); break;
        case DEBUG_COLOR::SUCCESS: builder.Build("Success: "); break;
        default:                   REV_ERROR_M("Wrong DEBUG_COLOR value: 0x%X", color); break;
    }

    REV_CHECK(format);

    va_list args;
    va_start(args, format);

    if (args) builder.BuildVA(format, args);
    else      builder.Build(format);

    va_end(args);

    if (print_sys_error)
    {
        u32 sys_error_code = GetSysErrorCode();
        if (sys_error_code)
        {
            builder.m_IntFormat.Base = BASE::HEX;
            builder.Build(". System error [", sys_error_code, "]: ", GetSysErrorMessage(sys_error_code));
            builder.m_IntFormat.Base = BASE::DEC;
        }
    }

    builder.BuildLn('.');

    g_CriticalSection.Enter();
    {
    #if REV_PLATFORM_WIN64
        REV_DEBUG_RESULT(SetConsoleTextAttribute(g_Console, cast(u16, color)));
        REV_DEBUG_RESULT(WriteConsoleA(g_Console, builder.BufferData(), (u32)builder.BufferLength(), null, null));
        REV_DEBUG_RESULT(SetConsoleTextAttribute(g_Console, cast(u16, DEBUG_COLOR::INFO)));
        OutputDebugStringA(builder.BufferData());
    #else
        REV_ERROR_M("Unhandled platform dependent code!");
    #endif
    }
    g_CriticalSection.Leave();
}

REV_API ConstString GetSysErrorMessage(u32 error_code)
{
#if REV_PLATFORM_WIN64
    REV_CHECK(error_code != ERROR_SUCCESS);
    char *error_message          = Memory::Get()->PushToFA<char>(2048);
    u32   error_message_length   = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                  null,
                                                  error_code,
                                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                                  error_message, 2048,
                                                  null);
    return ConstString(error_message, error_message_length - REV_CSTRLEN(".\r\n"));
#else
    REV_ERROR_M("Unhandled platform dependent code!");
#endif
}

REV_API u32 GetSysErrorCode()
{
#if REV_PLATFORM_WIN64
    return GetLastError();
#else
    REV_ERROR_M("Unhandled platform dependent code!");
#endif
}

}
