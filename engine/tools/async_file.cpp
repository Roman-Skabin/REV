//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/async_file.h"

AsyncFile::AsyncFile(const StaticString<MAX_PATH>& filename, FLAGS flags)
    : m_Handle(null),
      m_Flags(flags),
      m_Offset(0),
      m_Size(0),
      m_Overlapped{0},
      m_Name(filename)
{
    Check(flags != FLAGS::NONE);

    m_Overlapped.hEvent = CreateEventExA(null, null, CREATE_EVENT_INITIAL_SET, EVENT_ALL_ACCESS);

    u32 desired_access       = 0;
    u32 shared_mode          = 0;
    u32 creation_disposition = 0;
    u32 flags_and_attributes = 0;

    if ((m_Flags & FLAGS::READ) != FLAGS::NONE)
    {
        desired_access       = GENERIC_READ;
        shared_mode          = FILE_SHARE_READ;
        creation_disposition = OPEN_EXISTING;
        flags_and_attributes = FILE_ATTRIBUTE_READONLY;
    }
    if ((m_Flags & FLAGS::WRITE) != FLAGS::NONE)
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
           "AsyncFile doesn't exists or some unknown error has been occurred.\n"
           "Function args:\n"
           "    filename: \"%s\"\n"
           "    flags:    0x%I32X\n",
           filename, flags);

    m_Size = GetFileSize(m_Handle, null);
}

AsyncFile::AsyncFile(const AsyncFile& other)
{
    other.Wait();

    m_Flags             = other.m_Flags;
    m_Offset            = other.m_Offset;
    m_Size              = other.m_Size;
    m_Overlapped.hEvent = CreateEventExA(null, null, CREATE_EVENT_INITIAL_SET, EVENT_ALL_ACCESS);
    m_Name              = other.m_Name;

    HANDLE current_process = GetCurrentProcess();
    DebugResult(DuplicateHandle(current_process,
                                other.m_Handle,
                                current_process,
                                &m_Handle,
                                0,
                                false,
                                DUPLICATE_SAME_ACCESS));
}

AsyncFile::AsyncFile(AsyncFile&& other) noexcept
{
    CopyMemory(this, &other, StructFieldOffset(AsyncFile, m_Name));
    m_Name = other.m_Name;

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

    ZeroMemory(this, StructFieldOffset(AsyncFile, m_Name));
}

void AsyncFile::Clear()
{
    Wait();

    DebugResult(SetFilePointer(m_Handle, 0, null, FILE_BEGIN) == 0);
    m_Offset = 0;

    DebugResult(SetEndOfFile(m_Handle));
    m_Size = 0;
}

void AsyncFile::Move(const StaticString<MAX_PATH>& to_filename)
{
    Wait();

    if (MoveFileExA(m_Name, to_filename, MOVEFILE_REPLACE_EXISTING))
    {
        m_Name = to_filename;
    }
}

void AsyncFile::Copy(const StaticString<MAX_PATH>& to_filename) const
{
    CopyFileExA(m_Name, to_filename, null, null, null, 0);
}

void AsyncFile::Copy(const StaticString<MAX_PATH>& to_filename, AsyncFile& to_file, FLAGS to_flags) const
{
    if (CopyFileExA(m_Name, to_filename, null, null, null, 0))
    {
        to_file = AsyncFile(to_filename, to_flags == FLAGS::NONE ? m_Flags : to_flags);
    }
}

void AsyncFile::Read(void *buffer, u32 buffer_bytes) const
{
    CheckM((m_Flags & FLAGS::READ) != FLAGS::NONE, "You have no rights to read this file");
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

    m_Flags &= ~FLAGS::_WWEC;
}

void AsyncFile::Write(const void *buffer, u32 buffer_bytes)
{
    CheckM((m_Flags & FLAGS::WRITE) != FLAGS::NONE, "You have no rights to write to this file");
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
    
    m_Flags &= ~FLAGS::_WWEC;
}

void AsyncFile::Append(const void *buffer, u32 buffer_bytes)
{
    CheckM((m_Flags & FLAGS::WRITE) != FLAGS::NONE, "You have no rights to write to this file");
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

    m_Flags &= ~FLAGS::_WWEC;
}

void AsyncFile::Wait() const
{
    if ((m_Flags & FLAGS::_WWEC) == FLAGS::NONE)
    {
        u32 res = WAIT_FAILED;
        do
        {
            res = WaitForSingleObjectEx(m_Overlapped.hEvent, INFINITE, true);
        } while (res != WAIT_OBJECT_0 && res != WAIT_IO_COMPLETION);

        m_Flags |= FLAGS::_WWEC;
    }
}

void AsyncFile::SetOffset(u32 offset)
{
    m_Offset = offset > m_Size ? m_Size : offset;
}

AsyncFile& AsyncFile::operator=(const AsyncFile& other)
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
        m_Name   = other.m_Name;

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

AsyncFile& AsyncFile::operator=(AsyncFile&& other) noexcept
{
    if (this != &other)
    {
        CopyMemory(this, &other, StructFieldOffset(AsyncFile, m_Name));
        m_Name = other.m_Name;

        m_Handle            = null;
        m_Overlapped.hEvent = null;
    }
    return *this;
}

void WINAPI OverlappedReadCompletionRoutine(
    u32         error_code,
    u32         bytes_transfered,
    OVERLAPPED *overlapped)
{
    // @Important(Roman): NOT SAFETY!!!
    AsyncFile *file = cast<AsyncFile *>(cast<byte *>(overlapped)
                    - StructFieldOffset(AsyncFile, m_Overlapped));

    file->m_Offset += bytes_transfered;

    DebugResult(SetEvent(file->m_Overlapped.hEvent));
}

void WINAPI OverlappedWriteCompletionRoutine(
    u32         error_code,
    u32         bytes_transfered,
    OVERLAPPED *overlapped)
{
    // @Important(Roman): NOT SAFETY!!!
    AsyncFile *file = cast<AsyncFile *>(cast<byte *>(overlapped)
                    - StructFieldOffset(AsyncFile, m_Overlapped));

    file->m_Offset += bytes_transfered;

    if (file->m_Offset > file->m_Size)
    {
        file->m_Size = file->m_Offset;
    }

    DebugResult(SetEvent(file->m_Overlapped.hEvent));
}
