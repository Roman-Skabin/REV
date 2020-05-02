//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/core.h"

//
// Debuging
//

void __cdecl DebugF(const char *format, ...)
{
#if DEBUG
    va_list args;
    va_start(args, format);

    if (args)
    {
        char buffer[BUFSIZ] = {'\0'};
        int len             = vsprintf(buffer, format, args);
        buffer[len]         = '\n';

        OutputDebugStringA(buffer);
    }
    else
    {
        OutputDebugStringA(format);
        OutputDebugStringA("\n");
    }

    va_end(args);
#endif
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

        case MESSAGE_TYPE_INFO:
        {
            title = "Info";
        } break;

        case MESSAGE_TYPE_WARNING:
        {
            title = "Warning";
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
