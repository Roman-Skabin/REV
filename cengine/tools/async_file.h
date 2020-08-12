//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

typedef enum FILE_FLAG
{
    FILE_FLAG_CLOSED = 0,
    FILE_FLAG_OPENED = BIT(0),

    FILE_FLAG_READ   = BIT(1),
    FILE_FLAG_WRITE  = BIT(2),
} FILE_FLAG;

// @Issue(Roman): Several simultaneous read operations under the same file?

typedef struct AsyncFile
{
    HANDLE          handle;
    FILE_FLAG       flags;
    u32             offset;
    u32             size;
    OVERLAPPED      overlapped;
    char            name[_MAX_PATH];
} AsyncFile;

CENGINE_FUN void CreateAsyncFile(
    IN  const char *filename,
    IN  FILE_FLAG   flags,
    OUT AsyncFile  *file
);

CENGINE_FUN void CopyAsyncFile(
    OUT AsyncFile *dest,
    IN  AsyncFile *src
);

CENGINE_FUN void CloseAsyncFile(
    IN AsyncFile *file
);

CENGINE_FUN void ClearAsyncFile(
    IN AsyncFile *file
);

CENGINE_FUN void ReadAsyncFile(
    IN  AsyncFile *file,
    OUT void      *buffer,
    IN  u32        buffer_bytes
);

CENGINE_FUN void WriteAsyncFile(
    IN  AsyncFile *file,
    OUT void      *buffer,
    IN  u32        buffer_bytes
);

CENGINE_FUN void AppendAsyncFile(
    IN  AsyncFile *file,
    OUT void      *buffer,
    IN  u32        buffer_bytes
);

CENGINE_FUN void WaitForAsyncFile(
    IN AsyncFile *file
);

CENGINE_FUN void SetAsyncFileOffset(
    IN AsyncFile *file,
    IN u32        offset
);
