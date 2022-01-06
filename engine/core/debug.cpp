// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "memory/memory.h"
#include "tools/const_string.h"
#include "tools/string_builder.h"

namespace REV
{

REV_GLOBAL HANDLE                 g_Console         = GetStdHandle(STD_OUTPUT_HANDLE);
REV_GLOBAL CriticalSection<false> g_CriticalSection;

// @NOTE(Roman): For ComposeStackTraceMessage
#ifndef _REV_NO_CHECKS
    #define REV_CSTM_ERROR_M(message, ...)       { PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::ERROR, true, false, message, __VA_ARGS__); return false; }

    #define REV_CSTM_CHECK_M(expr, message, ...) { if (!(expr)) REV_CSTM_ERROR_M(message, __VA_ARGS__) }
    #define REV_CSTM_CHECK(expr)                 { if (!(expr)) REV_CSTM_ERROR_M(REV_CSTR(expr)) }
    
    #define REV_CSTM_DEBUG_RESULT(expr)                 REV_CSTM_CHECK(expr)
    #define REV_CSTM_DEBUG_RESULT_M(expr, message, ...) REV_CSTM_CHECK_M(expr, message, __VA_ARGS__)
#else
    #define REV_CSTM_ERROR_M(message, ...)       { PrintDebugMessage(__FILE__, __LINE__, DEBUG_COLOR::ERROR, true, false, message, __VA_ARGS__); return false; }

    #define REV_CSTM_CHECK_M(expr, message, ...) {}
    #define REV_CSTM_CHECK(expr)                 {}

    #define REV_CSTM_DEBUG_RESULT(expr)                 { expr; }
    #define REV_CSTM_DEBUG_RESULT_M(expr, message, ...) { expr; }
#endif

REV_INTERNAL bool ComposeStackTraceMessage(StringBuilder& builder)
{
    // @NOTE(Roman): Stack trace:
    //                   module_filename(line): symbol_name
    //                   or
    //                   module_filename(address): symbol_name

#if REV_PLATFORM_WIN64
    void **stack_trace     = Memory::Get()->PushToFA<void *>(REV_U16_MAX);
    u32    hash_value      = 0;
    u16    frames_captured = 0;
    REV_CSTM_DEBUG_RESULT(frames_captured = RtlCaptureStackBackTrace(2, REV_U16_MAX, stack_trace, &hash_value));

    HANDLE pseudo_process_handle = GetCurrentProcess();
    HANDLE real_process_handle   = null;
    REV_CSTM_DEBUG_RESULT(DuplicateHandle(pseudo_process_handle, pseudo_process_handle, pseudo_process_handle, &real_process_handle, 0, false, DUPLICATE_SAME_ACCESS));

    REV_CSTM_DEBUG_RESULT(SymInitialize(real_process_handle, null, true));

    builder.BuildLn("\nStack trace:");
    for (u16 frame = 0; frame < frames_captured; ++frame)
    {
        void *stack_frame = stack_trace[frame];

        // @NOTE(Roman): It just fails randomly so sometimes we won't have stack trace.
        //               Ok, maybe not that randomly: it fails mostly when we're trying
        //               to print stack trace for a worker thread (not Main Thread).
        DWORD64 module_base = 0;
        REV_CSTM_DEBUG_RESULT_M(module_base = SymGetModuleBase64(real_process_handle, cast(u64, stack_frame)), "SymGetModuleBase64 has failed randomly again");

        IMAGEHLP_MODULE64 module_info{};
        module_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        REV_CSTM_DEBUG_RESULT(SymGetModuleInfo64(real_process_handle, module_base, &module_info));

        SYMBOL_INFO *symbol_info  = cast(SYMBOL_INFO *, Memory::Get()->PushToFrameArena(sizeof(SYMBOL_INFO) + MAX_SYM_NAME));
        symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol_info->MaxNameLen   = MAX_SYM_NAME;

        ConstString function_name(REV_CSTR_ARGS("<unknown>"));
        if (SymFromAddr(real_process_handle, cast(u64, stack_frame), null, symbol_info))
        {
            function_name.AssignCSTR(symbol_info->Name, symbol_info->NameLen);
        }
        else
        {
            REV_CSTM_CHECK_M(GetLastError() == ERROR_MOD_NOT_FOUND, "Unhandled sys error for SymFromAddr");
        }

        IMAGEHLP_LINE64 line_info{};
        line_info.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        u32 displacement = 0;
        if (SymGetLineFromAddr64(real_process_handle, cast(u64, stack_frame), &displacement , &line_info))
        {
            ConstString filename_with_path(line_info.FileName, strlen(line_info.FileName));
            u64         slash_index   = filename_with_path.RFind('\\');
            ConstString just_filename = filename_with_path.SubString(slash_index + 1);

            builder.BuildLn("    ", just_filename, '(', line_info.LineNumber, "): ", function_name);
        }
        else
        {
            builder.Build("    ", ConstString(module_info.ModuleName, strlen(module_info.ModuleName)));
            switch (module_info.SymType)
            {
                case SymCoff:   builder.Build(".coff"); break;
                case SymPdb:    builder.Build(".pdb");  break;
                case SymExport: builder.Build(".dll");  break;
                case SymSym:    builder.Build(".sym");  break;
                case SymDia:    builder.Build(".dia");  break;
            }

            PointerFormat saved_pointer_format = builder.m_PointerFormat;
            {
                builder.m_PointerFormat.Decorate = false;
                builder.BuildLn('(', stack_frame, "): ", function_name);
            }
            builder.m_PointerFormat = saved_pointer_format;
        }
    }

    REV_CSTM_CHECK(CloseHandle(real_process_handle));
#else
    REV_CSTM_ERROR_M("Unhandled platform dependent code!");
#endif

    return true;
}

void REV_CDECL PrintDebugMessage(DEBUG_COLOR color, const char *format, ...)
{
    REV_CHECK(format);

    // @TODO(Roman): #Explain why do we need separate allocator
    //               and why should we use arenas or any other existing allocators.
    Allocator     allocator(null, MB(1), true, ConstString(REV_CSTR_ARGS(__FUNCTION__)), false);
    StringBuilder builder(&allocator, 1024);

    va_list args;
    va_start(args, format);

    if (args) builder.BuildVA(format, args);
    else      builder.Build(format);

    va_end(args);

    builder.BuildLn();

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

void REV_CDECL PrintDebugMessage(const char *file, u64 line, DEBUG_COLOR color, bool print_sys_error, bool print_stack_trace, const char *format, ...)
{
    // @NOTE(Roman): 1. file(line): type: message. System error [sys_error_code]: sys_error_message.
    //                  stack trace
    //               2. file(line): type: message.
    //                  stack trace

    // @TODO(Roman): #Explain why do we need separate allocator
    //               and why should we use arenas or any other existing allocators.
    Allocator     allocator(null, MB(1), true, ConstString(REV_CSTR_ARGS(__FUNCTION__)), false);
    StringBuilder builder(&allocator, 1024);

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

    u64 builder_buffer_save_point = builder.BufferLength();
    if (print_stack_trace && !ComposeStackTraceMessage(builder))
    {
        builder.Buffer().Erase(builder_buffer_save_point, builder.BufferLength());
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
