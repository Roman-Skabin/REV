//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/core.h"

//
// Debuging
//

global HANDLE gConsole;

void __cdecl DebugF(DEBUG_IN debug_in, const char *format, ...)
{
    if (!gConsole)
    {
        gConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    va_list args;
    va_start(args, format);

    char buffer[BUFSIZ] = {'\0'};
    int len             = vsprintf(buffer, format, args);
    buffer[len]         = '\n';

    if (debug_in & DEBUG_IN_CONSOLE)
    {
        WriteConsoleA(gConsole, buffer, len, 0, 0);
    }
    if (debug_in & DEBUG_IN_DEBUG)
    {
        OutputDebugStringA(buffer);
    }

    va_end(args);
}

void __cdecl MessageF(MESSAGE_TYPE type, const char *format, ...)
{
    const char *title = 0;

    switch (type)
    {
        case MESSAGE_TYPE_ERROR:
        {
            title = "Error";
        } break;
        
        case MESSAGE_TYPE_WARNING:
        {
            title = "Warning";
        } break;

        case MESSAGE_TYPE_INFO:
        {
            title = "Info";
        } break;
    }

    va_list args;
    va_start(args, format);

    if (args)
    {
        char buffer[BUFSIZ] = {'\0'};
        int  len            = vsprintf(buffer, format, args);

        MessageBoxA(0, buffer, title, MB_OK | type);
    }
    else
    {
        MessageBoxA(0, format, title, MB_OK | type);
    }

    va_end(args);

    if (type == MESSAGE_TYPE_ERROR)
    {
        ExitProcess(1);
    }
}
