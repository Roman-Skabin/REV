// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "core/common.h"
#include "memory/memory.h"
#include "tools/const_string.h"
#include "tools/static_string_builder.hpp"

namespace REV
{

REV_GLOBAL HANDLE                 g_Console         = GetStdHandle(STD_OUTPUT_HANDLE);
REV_GLOBAL CriticalSection<false> g_CriticalSection;

// @NOTE(Roman): For ComposeStackTraceMessage
#if (REV_DEVDEBUG || defined(_REV_CHECKS_BREAK)) && !defined(_REV_NO_CHECKS)
    #define REV_CSTM_ERROR_M(message, ...)       { PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::ERROR, true, false, message, __VA_ARGS__); REV_DEBUG_BREAK(); ExitProcess(1); }
    #define REV_CSTM_CHECK_M(expr, message, ...) { if (!(expr)) REV_CSTM_ERROR_M(message, __VA_ARGS__) }
    #define REV_CSTM_CHECK(expr)                 { if (!(expr)) REV_CSTM_ERROR_M(REV_CSTR(expr)) }
#elif !defined(_REV_NO_CHECKS)
    #define REV_CSTM_ERROR_M(message, ...)       { PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::ERROR, true, false, message, __VA_ARGS__); ExitProcess(1); }
    #define REV_CSTM_CHECK_M(expr, message, ...) { if (!(expr)) REV_CSTM_ERROR_M(message, __VA_ARGS__) }
    #define REV_CSTM_CHECK(expr)                 { if (!(expr)) REV_CSTM_ERROR_M(REV_CSTR(expr)) }
#else
    #define REV_CSTM_ERROR_M(message, ...)       { PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::ERROR, true, false, message, __VA_ARGS__); ExitProcess(1); }
    #define REV_CSTM_CHECK_M(expr, message, ...) {}
    #define REV_CSTM_CHECK(expr)                 {}
#endif

REV_INTERNAL void ComposeStackTraceMessage(StaticStringBuilder<2048>& builder)
{
    // @NOTE(Roman): Stack trace:
    //                   +-------------+---------------+------------------+
    //                   |    module   |    function   |      address     |
    //                   +-------------+---------------+------------------+
    //                   | module_name | function_name | function_address |
    //                   +-------------+---------------+------------------+

#if REV_PLATFORM_WIN64
    void **stack_trace     = Memory::Get()->PushToFA<void *>(REV_U16_MAX);
    u32    hash_value      = 0;
    u16    frames_captured = RtlCaptureStackBackTrace(2, REV_U16_MAX, stack_trace, &hash_value);
    REV_CSTM_CHECK(frames_captured);

    HANDLE pseudo_handle = GetCurrentProcess();
    HANDLE real_handle   = null;
    bool   result        = DuplicateHandle(pseudo_handle, pseudo_handle, pseudo_handle, &real_handle, 0, false, DUPLICATE_SAME_ACCESS);

    result = SymInitialize(real_handle, null, true);
    REV_CSTM_CHECK(result);

    builder.BuildLn("\nStack trace:");
    builder.BuildLn("    +------------------+--------------------------------------------------+--------------------+");
    builder.BuildLn("    |      module      |                     function                     |       address      |");
    builder.BuildLn("    +------------------+--------------------------------------------------+--------------------+");

    for (u16 frame = 0; frame < frames_captured; ++frame)
    {
        void *stack_frame = stack_trace[frame];

        IMAGEHLP_MODULE64 module_info = {};
        module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

        result = SymGetModuleInfo64(real_handle, SymGetModuleBase64(real_handle, cast(u64, stack_frame)), &module_info);
        REV_CSTM_CHECK(result);

        SYMBOL_INFO_PACKAGE symbol_package = {};
        symbol_package.si.SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_package.si.MaxNameLen   = sizeof(symbol_package.name);

        u64 displacement = 0;
        result = SymFromAddr(real_handle, cast(u64, stack_frame), &displacement, &symbol_package.si);

        ConstString function_name(symbol_package.si.Name, symbol_package.si.NameLen);
        if (!result)
        {
            REV_CSTM_CHECK_M(GetLastError() == ERROR_MOD_NOT_FOUND, "Unhandled sys error for SymFromAddr");
            function_name.AssignCSTR(REV_CSTR_ARGS("<unknown>"));
        }

        builder.Build("    | ");

        TextFormat saved_text_format = builder.m_TextFormat;
        {
            builder.m_TextFormat.Width         = 16;
            builder.m_TextFormat.TextAlignment = SBTA::LEFT;

            builder.Build(cast(char *, module_info.ModuleName));
        }
        builder.m_TextFormat = saved_text_format;

        builder.Build(" | ");

        saved_text_format = builder.m_TextFormat;
        {
            builder.m_TextFormat.Width         = 48;
            builder.m_TextFormat.TextAlignment = SBTA::LEFT;

            builder.Build(function_name);
        }
        builder.m_TextFormat = saved_text_format;

        builder.BuildLn(" | ", cast(void *, cast(u64, stack_frame) - displacement), " |");
    }

    builder.BuildLn("    +------------------+--------------------------------------------------+--------------------+");

    REV_CSTM_CHECK(CloseHandle(real_handle));
#else
    REV_CSTM_ERROR_M("Unhandled platform dependent code!");
#endif
}

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
        REV_DEBUG_RESULT(WriteConsoleA(g_Console, builder.BufferData(), cast(u32, builder.BufferLength()), null, null));
        REV_DEBUG_RESULT(SetConsoleTextAttribute(g_Console, cast(u16, DEBUG_COLOR::INFO)));
        OutputDebugStringA(builder.BufferData());
    #else
        REV_ERROR_M("Unhandled platform dependent code!");
    #endif
    }
    g_CriticalSection.Leave();
}

void REV_CDECL PrintDebugMessage(const char *file, u64 line, DEBUG_COLOR color, bool print_sys_error, bool reserved, const char *format, ...)
{
    // @NOTE(Roman): 1. file(line): type: message. System error [sys_error_code]: sys_error_message.
    //                  stack trace
    //               2. file(line): type: message.
    //                  stack trace

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

    if (reserved)
    {
        ComposeStackTraceMessage(builder);
    }

    g_CriticalSection.Enter();
    {
    #if REV_PLATFORM_WIN64
        REV_DEBUG_RESULT(SetConsoleTextAttribute(g_Console, cast(u16, color)));
        REV_DEBUG_RESULT(WriteConsoleA(g_Console, builder.BufferData(), cast(u32, builder.BufferLength()), null, null));
        REV_DEBUG_RESULT(SetConsoleTextAttribute(g_Console, cast(u16, DEBUG_COLOR::INFO)));
        OutputDebugStringA(builder.BufferData());
    #else
        REV_ERROR_M("Unhandled platform dependent code!");
    #endif
    }
    g_CriticalSection.Leave();
}

ConstString GetSysErrorMessage(u32 error_code)
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

u32 GetSysErrorCode()
{
#if REV_PLATFORM_WIN64
    return GetLastError();
#else
    REV_ERROR_M("Unhandled platform dependent code!");
#endif
}

}
