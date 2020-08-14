//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/async_file.h"

void CreateAsyncFile(
    in  const char *filename,
    in  FILE_FLAG   flags,
    out AsyncFile  *file)
{
    Check(filename);
    Check(flags);

    u64 filename_len = strlen(filename);
    CheckM(filename_len && filename_len < ArrayCount(file->name),
           "Filename is too long. Max length is %I64u",
           ArrayCount(file->name) - 1);

    file->flags             = flags;
    file->offset            = 0;
    file->overlapped.hEvent = CreateEventExA(null, null, CREATE_EVENT_INITIAL_SET, EVENT_ALL_ACCESS);
    CopyMemory(file->name, filename, filename_len);
    file->name[filename_len] = '\0';

    u32 desired_access       = 0;
    u32 shared_mode          = 0;
    u32 creation_disposition = 0;
    u32 flags_and_attributes = 0;

    if (file->flags & FILE_FLAG_READ)
    {
        desired_access       = GENERIC_READ;
        shared_mode          = FILE_SHARE_READ;
        creation_disposition = OPEN_EXISTING;
        flags_and_attributes = FILE_ATTRIBUTE_READONLY;
    }
    if (file->flags & FILE_FLAG_WRITE)
    {
        desired_access       |= GENERIC_WRITE;
        shared_mode          |= FILE_SHARE_WRITE;
        creation_disposition  = CREATE_ALWAYS;
        flags_and_attributes  = FILE_ATTRIBUTE_NORMAL;
    }

    flags_and_attributes |= FILE_FLAG_OVERLAPPED
                         |  FILE_FLAG_RANDOM_ACCESS;

    file->handle = CreateFileA(filename,
                               desired_access,
                               shared_mode,
                               null,
                               creation_disposition,
                               flags_and_attributes,
                               null);

    CheckM(file->handle != INVALID_HANDLE_VALUE,
           "AsyncFile doesn't exists or some error has been occurred.\n"
           "Function args:\n"
           "    filename: \"%s\"\n"
           "    flags:    0x%I32X\n"
           "    file:     0x%p",
           filename, flags, file);

    file->flags |= FILE_FLAG_OPENED;

    file->size = GetFileSize(file->handle, null);
}

void CopyAsyncFile(
    out AsyncFile *dest,
    in  AsyncFile *src)
{
    Check(dest);
    Check(src);
    CheckM(src->flags & FILE_FLAG_OPENED, "File is not opened");

    WaitForAsyncFile(src);

    dest->flags             = src->flags & ~FILE_FLAG_OPENED;
    dest->offset            = src->offset;
    dest->size              = src->size;
    dest->overlapped.hEvent = CreateEventExA(null, null, CREATE_EVENT_INITIAL_SET, EVENT_ALL_ACCESS);
    CopyMemory(dest->name, src->name, sizeof(dest->name));

    HANDLE current_process = GetCurrentProcess();
    DebugResult(DuplicateHandle(current_process,
                                src->handle,
                                current_process,
                                &dest->handle,
                                0,
                                false,
                                DUPLICATE_SAME_ACCESS));

    dest->flags |= FILE_FLAG_OPENED;
}

void CloseAsyncFile(
    in AsyncFile *file)
{
    if (file)
    {
        CheckM(file->flags & FILE_FLAG_OPENED, "File is not opened");

        WaitForAsyncFile(file);

        if (file->handle)
        {
            DebugResult(CloseHandle(file->handle));
        }

        if (file->overlapped.hEvent)
        {
            DebugResult(CloseHandle(file->overlapped.hEvent));
        }

        ZeroMemory(file, sizeof(AsyncFile));
    }
}

void ClearAsyncFile(
    in AsyncFile *file)
{
    Check(file);
    CheckM(file->flags & FILE_FLAG_OPENED, "File is not opened");

    WaitForAsyncFile(file);

    DebugResult(SetFilePointer(file->handle, 0, null, FILE_BEGIN) == 0);
    file->offset = 0;

    DebugResult(SetEndOfFile(file->handle));
    file->size = 0;
}

internal void WINAPI OverlappedReadCompletionRoutine(
    in     u32         error_code,
    in     u32         number_of_bytes_transfered,
    in out OVERLAPPED *overlapped)
{
    // @Important(Roman): NOT SAFETY!!!
    AsyncFile *file = cast(AsyncFile *, cast(byte *, overlapped)
                    - StructFieldOffset(AsyncFile, overlapped));

    file->offset += number_of_bytes_transfered;

    DebugResult(SetEvent(file->overlapped.hEvent));
}

void ReadAsyncFile(
    in  AsyncFile *file,
    out void      *buffer,
    in  u32        buffer_bytes)
{
    Check(file);
    CheckM(file->flags & FILE_FLAG_OPENED, "File is not opened");
    CheckM(file->flags & FILE_FLAG_READ, "You have no rights to read this file");
    Check(buffer);
    CheckM(buffer_bytes, "Buffer size must be more than 0");

    WaitForAsyncFile(file);

    file->overlapped.Internal     = 0;
    file->overlapped.InternalHigh = 0;
    file->overlapped.Offset       = file->offset;
    file->overlapped.OffsetHigh   = 0;

    DebugResult(ReadFileEx(file->handle,
                           buffer,
                           buffer_bytes,
                           &file->overlapped,
                           OverlappedReadCompletionRoutine));
}

internal void WINAPI OverlappedWriteCompletionRoutine(
    in     u32         error_code,
    in     u32         number_of_bytes_transfered,
    in out OVERLAPPED *overlapped)
{
    // @Important(Roman): NOT SAFETY!!!
    AsyncFile *file = cast(AsyncFile *, cast(byte *, overlapped)
                    - StructFieldOffset(AsyncFile, overlapped));

    file->offset += number_of_bytes_transfered;

    if (file->offset > file->size)
    {
        file->size = file->offset;
    }

    DebugResult(SetEvent(file->overlapped.hEvent));
}

void WriteAsyncFile(
    in  AsyncFile *file,
    out void      *buffer,
    in  u32        buffer_bytes)
{
    Check(file);
    CheckM(file->flags & FILE_FLAG_OPENED, "File is not opened");
    CheckM(file->flags & FILE_FLAG_WRITE, "You have no rights to write to this file");
    Check(buffer);
    CheckM(buffer_bytes, "Buffer size must be more than 0");

    WaitForAsyncFile(file);

    file->overlapped.Internal     = 0;
    file->overlapped.InternalHigh = 0;
    file->overlapped.Offset       = file->offset;
    file->overlapped.OffsetHigh   = 0;

    DebugResult(WriteFileEx(file->handle,
                            buffer,
                            buffer_bytes,
                            &file->overlapped,
                            OverlappedWriteCompletionRoutine));
}

void AppendAsyncFile(
    in  AsyncFile *file,
    out void      *buffer,
    in  u32        buffer_bytes)
{
    Check(file);
    CheckM(file->flags & FILE_FLAG_OPENED, "File is not opened");
    CheckM(file->flags & FILE_FLAG_WRITE, "You have no rights to write to this file");
    Check(buffer);
    CheckM(buffer_bytes, "Buffer size must be more than 0");

    WaitForAsyncFile(file);

    file->offset = file->size;

    file->overlapped.Internal     = 0;
    file->overlapped.InternalHigh = 0;
    file->overlapped.Offset       = file->offset;
    file->overlapped.OffsetHigh   = 0;

    DebugResult(WriteFileEx(file->handle,
                            buffer,
                            buffer_bytes,
                            &file->overlapped,
                            OverlappedWriteCompletionRoutine));
}

void WaitForAsyncFile(
    in AsyncFile *file)
{
    Check(file);
    CheckM(file->flags & FILE_FLAG_OPENED, "File is not opened");

    u32 res = WAIT_FAILED;
    do
    {
        res = WaitForSingleObjectEx(file->overlapped.hEvent, INFINITE, true);
    } while (res != WAIT_OBJECT_0 && res != WAIT_IO_COMPLETION);
}

void SetAsyncFileOffset(
    in AsyncFile *file,
    in u32        offset)
{
    Check(file);
    CheckM(file->flags & FILE_FLAG_OPENED, "File is not opened");

    file->offset = offset > file->size ? file->size : offset;
}
