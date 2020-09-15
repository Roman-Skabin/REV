//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/async_file.h"

AsyncFile::AsyncFile(in const char *filename, in_opt u64 filename_len, in FILE_FLAG flags)
    : m_Flags(flags),
      m_Offset(0),
      m_Overlapped{0}
{
    Check(filename);
    Check(flags != FILE_FLAG::NONE);

    if (!filename_len) filename_len = strlen(filename);
    CheckM(filename_len && filename_len < ArrayCount(m_Name),
           "Filename is too long. Max length is %I64u",
           ArrayCount(m_Name) - 1);

    m_Overlapped.hEvent = CreateEventExA(null, null, CREATE_EVENT_INITIAL_SET, EVENT_ALL_ACCESS);
    CopyMemory(m_Name, filename, filename_len);
    m_Name[filename_len] = '\0';

    u32 desired_access       = 0;
    u32 shared_mode          = 0;
    u32 creation_disposition = 0;
    u32 flags_and_attributes = 0;

    if ((m_Flags & FILE_FLAG::READ) != FILE_FLAG::NONE)
    {
        desired_access       = GENERIC_READ;
        shared_mode          = FILE_SHARE_READ;
        creation_disposition = OPEN_EXISTING;
        flags_and_attributes = FILE_ATTRIBUTE_READONLY;
    }
    if ((m_Flags & FILE_FLAG::WRITE) != FILE_FLAG::NONE)
    {
        desired_access       |= GENERIC_WRITE;
        shared_mode          |= FILE_SHARE_WRITE;
        creation_disposition  = CREATE_ALWAYS;
        flags_and_attributes  = FILE_ATTRIBUTE_NORMAL;
    }

    flags_and_attributes |= FILE_FLAG_OVERLAPPED
                         |  FILE_FLAG_RANDOM_ACCESS;

    m_Handle = CreateFileA(filename,
                           desired_access,
                           shared_mode,
                           null,
                           creation_disposition,
                           flags_and_attributes,
                           null);

    CheckM(m_Handle != INVALID_HANDLE_VALUE,
           "AsyncFile doesn't exists or some error has been occurred.\n"
           "Function args:\n"
           "    filename:     \"%s\"\n"
           "    filename_len: 0x%I64u\n"
           "    flags:        0x%I32X\n",
           filename, filename_len, flags);

    m_Size = GetFileSize(m_Handle, null);
}

AsyncFile::AsyncFile(in const AsyncFile& other)
{
    other.Wait();

    m_Flags             = other.m_Flags;
    m_Offset            = other.m_Offset;
    m_Size              = other.m_Size;
    m_Overlapped.hEvent = CreateEventExA(null, null, CREATE_EVENT_INITIAL_SET, EVENT_ALL_ACCESS);
    CopyMemory(m_Name, other.m_Name, sizeof(m_Name));

    HANDLE current_process = GetCurrentProcess();
    DebugResult(DuplicateHandle(current_process,
                                other.m_Handle,
                                current_process,
                                &m_Handle,
                                0,
                                false,
                                DUPLICATE_SAME_ACCESS));
}

AsyncFile::AsyncFile(in AsyncFile&& other) noexcept
{
    CopyMemory(this, &other, sizeof(AsyncFile));
    m_Handle            = null;
    m_Overlapped.hEvent = null;
}

AsyncFile::~AsyncFile()
{
    Wait();

    if (m_Handle)
    {
        DebugResult(CloseHandle(m_Handle));
    }

    if (m_Overlapped.hEvent)
    {
        DebugResult(CloseHandle(m_Overlapped.hEvent));
    }

    ZeroMemory(this, sizeof(AsyncFile));
}

void AsyncFile::Clear()
{
    Wait();

    DebugResult(SetFilePointer(m_Handle, 0, null, FILE_BEGIN) == 0);
    m_Offset = 0;

    DebugResult(SetEndOfFile(m_Handle));
    m_Size = 0;
}

void AsyncFile::Read(out void *buffer, in u32 buffer_bytes) const
{
    CheckM((m_Flags & FILE_FLAG::READ) != FILE_FLAG::NONE, "You have no rights to read this file");
    Check(buffer);
    CheckM(buffer_bytes, "Buffer size must be more than 0");

    Wait();

    m_Overlapped.Internal     = 0;
    m_Overlapped.InternalHigh = 0;
    m_Overlapped.Offset       = m_Offset;
    m_Overlapped.OffsetHigh   = 0;

    DebugResult(ReadFileEx(m_Handle,
                           buffer,
                           buffer_bytes,
                           &m_Overlapped,
                           OverlappedReadCompletionRoutine));
}

void AsyncFile::Write(in const void *buffer, in u32 buffer_bytes)
{
    CheckM((m_Flags & FILE_FLAG::WRITE) != FILE_FLAG::NONE, "You have no rights to write to this file");
    Check(buffer);
    CheckM(buffer_bytes, "Buffer size must be more than 0");

    Wait();

    m_Overlapped.Internal     = 0;
    m_Overlapped.InternalHigh = 0;
    m_Overlapped.Offset       = m_Offset;
    m_Overlapped.OffsetHigh   = 0;

    DebugResult(WriteFileEx(m_Handle,
                            buffer,
                            buffer_bytes,
                            &m_Overlapped,
                            OverlappedWriteCompletionRoutine));
}

void AsyncFile::Append(in const void *buffer, in u32 buffer_bytes)
{
    CheckM((m_Flags & FILE_FLAG::WRITE) != FILE_FLAG::NONE, "You have no rights to write to this file");
    Check(buffer);
    CheckM(buffer_bytes, "Buffer size must be more than 0");

    Wait();

    m_Offset = m_Size;

    m_Overlapped.Internal     = 0;
    m_Overlapped.InternalHigh = 0;
    m_Overlapped.Offset       = m_Offset;
    m_Overlapped.OffsetHigh   = 0;

    DebugResult(WriteFileEx(m_Handle,
                            buffer,
                            buffer_bytes,
                            &m_Overlapped,
                            OverlappedWriteCompletionRoutine));
}

void AsyncFile::Wait() const
{
    u32 res = WAIT_FAILED;
    do
    {
        res = WaitForSingleObjectEx(m_Overlapped.hEvent, INFINITE, true);
    } while (res != WAIT_OBJECT_0 && res != WAIT_IO_COMPLETION);
}

void AsyncFile::SetOffset(in u32 offset)
{
    m_Offset = offset > m_Size ? m_Size : offset;
}

AsyncFile& AsyncFile::operator=(in const AsyncFile& other)
{
    if (this != &other)
    {
        other.Wait();
        Wait();

        if (m_Handle)
        {
            DebugResult(CloseHandle(m_Handle));
        }

        m_Flags  = other.m_Flags;
        m_Offset = other.m_Offset;
        m_Size   = other.m_Size;
        CopyMemory(m_Name, other.m_Name, sizeof(m_Name));

        HANDLE current_process = GetCurrentProcess();
        DebugResult(DuplicateHandle(current_process,
                                    other.m_Handle,
                                    current_process,
                                    &m_Handle,
                                    0,
                                    false,
                                    DUPLICATE_SAME_ACCESS));
    }
    return *this;
}

AsyncFile& AsyncFile::operator=(in AsyncFile&& other) noexcept
{
    if (this != &other)
    {
        CopyMemory(this, &other, sizeof(AsyncFile));
        m_Handle            = null;
        m_Overlapped.hEvent = null;
    }
    return *this;
}

void WINAPI OverlappedReadCompletionRoutine(
    in     u32         error_code,
    in     u32         number_of_bytes_transfered,
    in_out OVERLAPPED *overlapped)
{
    // @Important(Roman): NOT SAFETY!!!
    AsyncFile *file = cast<AsyncFile *>(cast<byte *>(overlapped)
                    - StructFieldOffset(AsyncFile, m_Overlapped));

    file->m_Offset += number_of_bytes_transfered;

    DebugResult(SetEvent(file->m_Overlapped.hEvent));
}

void WINAPI OverlappedWriteCompletionRoutine(
    in     u32         error_code,
    in     u32         number_of_bytes_transfered,
    in_out OVERLAPPED *overlapped)
{
    // @Important(Roman): NOT SAFETY!!!
    AsyncFile *file = cast<AsyncFile *>(cast<byte *>(overlapped)
                    - StructFieldOffset(AsyncFile, m_Overlapped));

    file->m_Offset += number_of_bytes_transfered;

    if (file->m_Offset > file->m_Size)
    {
        file->m_Size = file->m_Offset;
    }

    DebugResult(SetEvent(file->m_Overlapped.hEvent));
}
