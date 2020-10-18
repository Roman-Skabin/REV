//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/core.h"

//
// Debuging
//

global HANDLE gConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void __cdecl DebugF(DEBUG_IN debug_in, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[BUFSIZ] = {'\0'};
    int len             = vsprintf(buffer, format, args);
    buffer[len]         = '\n';

    if (cast<bool>(debug_in & DEBUG_IN::CONSOLE))
    {
        WriteConsoleA(gConsole, buffer, len, null, null);
    }
    if (cast<bool>(debug_in & DEBUG_IN::WINDBG))
    {
        OutputDebugStringA(buffer);
    }

    va_end(args);
}

void __cdecl DebugFC(DEBUG_IN debug_in, DEBUG_COLOR color, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[BUFSIZ] = {'\0'};
    int len             = vsprintf(buffer, format, args);
    buffer[len]         = '\n';

    if (cast<bool>(debug_in & DEBUG_IN::CONSOLE))
    {
        SetConsoleTextAttribute(gConsole, cast<u16>(color));
        WriteConsoleA(gConsole, buffer, len, null, null);
        SetConsoleTextAttribute(gConsole, cast<u16>(DEBUG_COLOR::INFO));
    }
    if (cast<bool>(debug_in & DEBUG_IN::WINDBG))
    {
        OutputDebugStringA(buffer);
    }

    va_end(args);
}

void __cdecl MessageF(MESSAGE_TYPE type, const char *format, ...)
{
    char title[64] = {0};

    switch (type)
    {
        case MESSAGE_TYPE::ERROR:
        {
            sprintf(title, "Error: 0x%08I32X!", GetLastError());
        } break;
        
        case MESSAGE_TYPE::WARNING:
        {
            CopyMemory(title, "Warning", CSTRLEN("Warning"));
        } break;

        case MESSAGE_TYPE::INFO:
        {
            CopyMemory(title, "Info", CSTRLEN("Info"));
        } break;
    }

    va_list args;
    va_start(args, format);

    if (args)
    {
        char buffer[BUFSIZ] = {'\0'};
        int  len            = vsprintf(buffer, format, args);

        MessageBoxA(null, buffer, title, cast<u32>(MB_OK | type));

        va_end(args);
    }
    else
    {
        MessageBoxA(null, format, title, cast<u32>(MB_OK | type));
    }

    if (type == MESSAGE_TYPE::ERROR)
    {
        ExitProcess(1);
    }
}

void __cdecl ShowDebugMessage(
     b32         message_is_expr,
     const char *file,
     u64         line,
     const char *function,
     const char *title,
     const char *format,
    ...)
{
    // @Important(Roman): last_error has to be placed
    //                    before everything else.
    u32 last_error = GetLastError();

    char box_title[64];
    sprintf(box_title, "%s: 0x%08I32X!", title, last_error);

    va_list args;
    va_start(args, format);

    if (!message_is_expr && args)
    {
        char message[1024];
        vsprintf(message, format, args);

        const char *box_message_format =
        "MESSAGE: %s\n\n"
        "FILE: %s(%I64u)\n\n"
        "FUNCTION: %s";

        char box_message[2048];
        sprintf(box_message, box_message_format, message, file, line, function);

        MessageBoxA(null, box_message, box_title, MB_OK | MB_ICONERROR);

        va_end(args);
    }
    else if (!message_is_expr)
    {
        const char *box_message_format =
        "MESSAGE: %s\n\n"
        "FILE: %s(%I64u)\n\n"
        "FUNCTION: %s";

        char box_message[2048];
        sprintf(box_message, box_message_format, format, file, line, function);

        MessageBoxA(null, box_message, box_title, MB_OK | MB_ICONERROR);
    }
    else
    {
        const char *box_message_format =
        "EXPRESSION: %s\n\n"
        "FILE: %s(%I64u)\n\n"
        "FUNCTION: %s";

        char box_message[2048];
        sprintf(box_message, box_message_format, format, file, line, function);

        MessageBoxA(null, box_message, box_title, MB_OK | MB_ICONERROR);
    }
}
