//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/core.h"

//
// Debuging
//

void __cdecl DebugF(const char *const format, ...)
{
    va_list args;
    va_start(args, format);

    if (args)
    {
        char buffer[BUFSIZ] = {'\0'};
        int len             = vsprintf(buffer, format, args);
        buffer[len - 1]     = '\n';

        OutputDebugStringA(buffer);
    }
    else
    {
        OutputDebugStringA(format);
        OutputDebugStringA("\n");
    }

    va_end(args);
}
