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
    in  const char *filename,
    in  FILE_FLAG   flags,
    out AsyncFile  *file
);

CENGINE_FUN void CopyAsyncFile(
    out AsyncFile *dest,
    in  AsyncFile *src
);

CENGINE_FUN void CloseAsyncFile(
    in AsyncFile *file
);

CENGINE_FUN void ClearAsyncFile(
    in AsyncFile *file
);

CENGINE_FUN void ReadAsyncFile(
    in  AsyncFile *file,
    out void      *buffer,
    in  u32        buffer_bytes
);

CENGINE_FUN void WriteAsyncFile(
    in  AsyncFile *file,
    out void      *buffer,
    in  u32        buffer_bytes
);

CENGINE_FUN void AppendAsyncFile(
    in  AsyncFile *file,
    out void      *buffer,
    in  u32        buffer_bytes
);

CENGINE_FUN void WaitForAsyncFile(
    in AsyncFile *file
);

CENGINE_FUN void SetAsyncFileOffset(
    in AsyncFile *file,
    in u32        offset
);
