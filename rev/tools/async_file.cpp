// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "tools/async_file.h"
#include "tools/file.h"
#include "tools/static_string_builder.hpp"

namespace REV
{

AsyncFile::AsyncFile(nullptr_t)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Size(0),
      m_Flags(FILE_FLAG_NONE),
      m_CriticalSection(),
      m_APCEntries{},
      m_Name()
{
    CreateAPCEntries();
}

AsyncFile::AsyncFile(const ConstString& filename, FILE_FLAG flags)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Size(0),
      m_Flags(flags),
      m_CriticalSection(),
      m_APCEntries{},
      m_Name(filename)
{
    REV_CHECK_M(m_Name.Length() < REV_PATH_CAPACITY, "Fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    CreateAPCEntries();
    Open();
}

AsyncFile::AsyncFile(const AsyncFile& other)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Size(0),
      m_Flags(other.m_Flags),
      m_CriticalSection(),
      m_APCEntries{},
      m_Name(other.m_Name)
{
    // @TODO(Roman): Duplicate events instead of creating them?
    CreateAPCEntries();

    other.m_CriticalSection.Enter();

    other.Wait();

    m_Size = other.m_Size;

    HANDLE current_process = GetCurrentProcess();
    REV_DEBUG_RESULT(DuplicateHandle(current_process, other.m_Handle, current_process, &m_Handle, 0, false, DUPLICATE_SAME_ACCESS));

    other.m_CriticalSection.Leave();
}

AsyncFile::AsyncFile(AsyncFile&& other)
    : m_Handle(other.m_Handle),
      m_Size(other.m_Size),
      m_Flags(other.m_Flags),
      m_CriticalSection(RTTI::move(other.m_CriticalSection)),
      m_APCEntries{},
      m_Name(RTTI::move(other.m_Name))
{
    other.m_CriticalSection.Enter();

    for (u32 i = 0; i < MAX_APCS; ++i)
    {
        APCEntry *entry       = m_APCEntries       + i;
        APCEntry *other_entry = other.m_APCEntries + i;

        entry->file        = this;
        entry->in_progress = other_entry->in_progress;
        CopyMemory(&entry->overlapped, &other_entry->overlapped, sizeof(OVERLAPPED));

        other_entry->overlapped.hEvent = null;
    }
    other.m_Handle = INVALID_HANDLE_VALUE;

    other.m_CriticalSection.Leave();
}

AsyncFile::~AsyncFile()
{
    // @Issue(Roman): Do we really need enter critical section in destructor?
    //                Theoretically it will be destroyed in a thread a file was created,
    //                btw destructor can be called explicitly in several threads.
    m_CriticalSection.Enter();

    Close();
    DestroyAPCEntries();
    m_Size  = 0;
    m_Flags = FILE_FLAG_NONE;

    m_CriticalSection.Leave();
}

bool AsyncFile::Open(const ConstString& filename, FILE_FLAG flags)
{
    m_CriticalSection.Enter();
    if (m_Handle == INVALID_HANDLE_VALUE)
    {
        REV_CHECK_M(m_Name.Length() < REV_PATH_CAPACITY, "Fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
        m_Name  = filename;
        m_Flags = flags;
        Open();
    }
    else if (m_Name != filename)
    {
        REV_CHECK_M(m_Name.Length() < REV_PATH_CAPACITY, "Fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
        REV_WARNING_M("File \"%.*s\" has been already opened, so it will be closed to open a new one with a new filename: %.*s",
                      m_Name.Length(), m_Name.Data(),
                      filename.Length(), filename.Data());
        Close();
        m_Name  = filename;
        m_Flags = flags;
        Open();
    }
    else if (m_Flags != flags)
    {
        REV_WARNING_M("File \"%.*s\" is already opened, it will be reopened with new flags. Old flags: 0x%X, new flags: 0x%X",
                      m_Name.Length(), m_Name.Data(),
                      m_Flags,
                      flags);
        ReOpen(flags);
    }

    bool result = m_Handle != INVALID_HANDLE_VALUE;

    m_CriticalSection.Leave();
    return result;
}

void AsyncFile::ReOpen(FILE_FLAG new_flags)
{
    m_CriticalSection.Enter();
    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    if (m_Flags != new_flags)
    {
        m_Flags = new_flags;

        u32 desired_access = 0;
        u32 shared_access  = 0;
        u32 disposition    = 0;
        u32 attributes     = 0;
        SplitFlagsToWin32Flags(desired_access, shared_access, disposition, attributes);

        Wait();

        m_Handle = ReOpenFile(m_Handle, desired_access, shared_access, attributes);
        REV_CHECK(m_Handle != INVALID_HANDLE_VALUE);
    }
    m_CriticalSection.Leave();
}

void AsyncFile::Close()
{
    m_CriticalSection.Enter();
    if (m_Handle != INVALID_HANDLE_VALUE)
    {
        Wait();

        REV_DEBUG_RESULT(CloseHandle(m_Handle));
        m_Handle = INVALID_HANDLE_VALUE;
    }
    m_CriticalSection.Leave();
}

void AsyncFile::Clear()
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FILE_FLAG_WRITE, "You have no rights to clear file \"%.*s\". You must have write rights", m_Name.Length(), m_Name.Data());

    Wait();

    u32 offset = SetFilePointer(m_Handle, 0, null, FILE_BEGIN);
    REV_CHECK(offset == 0);

    REV_DEBUG_RESULT(SetEndOfFile(m_Handle));
    m_Size = 0;

    m_CriticalSection.Leave();
}

void AsyncFile::Read(void *buffer, u64 buffer_bytes, u64 file_offset)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "AsyncFile \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FILE_FLAG_READ, "You have no rights to read this file");
    REV_CHECK(buffer);
    REV_CHECK_M(buffer_bytes, "Buffer size must be more than 0");

    u64 saved_file_offset = file_offset;

    u32 bytes_to_read = 0;
    for (u64 buffer_offset = 0; buffer_offset < buffer_bytes; buffer_offset += bytes_to_read)
    {
        u64 rest_bytes = buffer_bytes - buffer_offset;
        bytes_to_read  = rest_bytes < REV_U32_MAX ? (u32)rest_bytes : REV_U32_MAX;

        while (true)
        {
            bool entry_found = false;

            for (u64 i = 0; i < MAX_APCS; ++i)
            {
                APCEntry *entry = m_APCEntries + i;
                if (!entry->in_progress)
                {
                    entry_found = true;

                    entry->in_progress              = true;
                    entry->overlapped.Internal      = 0;
                    entry->overlapped.InternalHigh  = 0;
                    entry->overlapped.Pointer       = cast(void *, saved_file_offset);

                    LockSystemCacheFromOtherProcesses(saved_file_offset, bytes_to_read, true);

                    REV_DEBUG_RESULT(ReadFileEx(m_Handle,
                                                cast(byte *, buffer) + buffer_offset,
                                                bytes_to_read,
                                                &entry->overlapped,
                                                OverlappedWriteCompletionRoutine));

                    u32 bytes_transfered = 0;
                    if (GetOverlappedResult(m_Handle, &entry->overlapped, &bytes_transfered, false))
                    {
                        entry->in_progress = false;
                    }

                    saved_file_offset += bytes_to_read;
                }
            }

            if (entry_found) break;
            else             Wait(false);
        }
    }

    m_CriticalSection.Leave();
}

void AsyncFile::Write(const void *buffer, u64 buffer_bytes, u64 file_offset)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "AsyncFile \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FILE_FLAG_WRITE, "You have no rights to write to this file");
    REV_CHECK(buffer);
    REV_CHECK_M(buffer_bytes, "Buffer size must be more than 0");

    u64 saved_file_offset = file_offset;

    u32 bytes_to_write = 0;
    for (u64 buffer_offset = 0; buffer_offset < buffer_bytes; buffer_offset += bytes_to_write)
    {
        u64 rest_bytes = buffer_bytes - buffer_offset;
        bytes_to_write = rest_bytes < REV_U32_MAX ? (u32)rest_bytes : REV_U32_MAX;

        while (true)
        {
            bool entry_found = false;

            for (u64 i = 0; i < MAX_APCS; ++i)
            {
                APCEntry *entry = m_APCEntries + i;
                if (!entry->in_progress)
                {
                    entry_found = true;

                    entry->in_progress              = true;
                    entry->overlapped.Internal      = 0;
                    entry->overlapped.InternalHigh  = 0;
                    entry->overlapped.Pointer       = cast(void *, saved_file_offset);

                    LockSystemCacheFromOtherProcesses(saved_file_offset, bytes_to_write, false);

                    REV_DEBUG_RESULT(WriteFileEx(m_Handle,
                                                 cast(byte *, buffer) + buffer_offset,
                                                 bytes_to_write,
                                                 &entry->overlapped,
                                                 OverlappedWriteCompletionRoutine));

                    // @NOTE(Roman): "The system sets the state of the event object to nonsignaled
                    //               when a call to the I/O function returns before the operation
                    //               has been completed. The system sets the state of the event
                    //               object to signaled when the operation has been completed.
                    //               When operation completed before the function returns,
                    //               the results are handled as if the operation had been performed
                    //               synchronously.

                    u32 bytes_transfered = 0;
                    if (GetOverlappedResult(m_Handle, &entry->overlapped, &bytes_transfered, false))
                    {
                        entry->in_progress = false;
                    }

                    saved_file_offset += bytes_to_write;
                }
            }

            if (entry_found) break;
            else             Wait(false);
        }
    }

    m_CriticalSection.Leave();
}

void AsyncFile::Append(const void *buffer, u64 buffer_bytes)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "AsyncFile \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FILE_FLAG_WRITE, "You have no rights to write to this file");
    REV_CHECK(buffer);
    REV_CHECK_M(buffer_bytes, "Buffer size must be more than 0");

    u64 saved_file_offset = m_Size;

    u32 bytes_to_write = 0;
    for (u64 buffer_offset = 0; buffer_offset < buffer_bytes; buffer_offset += bytes_to_write)
    {
        u64 rest_bytes = buffer_bytes - buffer_offset;
        bytes_to_write = rest_bytes < REV_U32_MAX ? (u32)rest_bytes : REV_U32_MAX;

        while (true)
        {
            bool entry_found = false;

            for (u64 i = 0; i < MAX_APCS; ++i)
            {
                APCEntry *entry = m_APCEntries + i;
                if (!entry->in_progress)
                {
                    entry_found = true;

                    entry->in_progress             = true;
                    entry->overlapped.Internal     = 0;
                    entry->overlapped.InternalHigh = 0;
                    entry->overlapped.Pointer      = cast(void *, saved_file_offset);

                    LockSystemCacheFromOtherProcesses(saved_file_offset, bytes_to_write, false);

                    REV_DEBUG_RESULT(WriteFileEx(m_Handle,
                                                 cast(byte *, buffer) + buffer_offset,
                                                 bytes_to_write,
                                                 &entry->overlapped,
                                                 OverlappedWriteCompletionRoutine));

                    u32 bytes_transfered = 0;
                    if (GetOverlappedResult(m_Handle, &entry->overlapped, &bytes_transfered, false))
                    {
                        entry->in_progress = false;
                    }

                    saved_file_offset += bytes_to_write;
                }
            }

            if (entry_found) break;
            else             Wait(false);
        }
    }

    m_CriticalSection.Leave();
}

void AsyncFile::Wait(bool wait_for_all_apcs) const
{
    m_CriticalSection.Enter();

    HANDLE events[MAX_APCS] = {null};
    u32    events_count     = 0;

    for (u32 i = 0; i < MAX_APCS; ++i)
    {
        APCEntry *entry = m_APCEntries + i;
        if (entry->in_progress)
        {
            events[events_count++] = entry->overlapped.hEvent;
        }
    }

    if (events_count)
    {
        u32 wait_result = WaitForMultipleObjectsEx(events_count, events, wait_for_all_apcs, INFINITE, true);
        REV_CHECK(wait_result == WAIT_IO_COMPLETION);
    }

    m_CriticalSection.Leave();
}

void AsyncFile::Flush()
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FILE_FLAG_WRITE, "You have no rights to flush file \"%.*s\". You must have write rights", m_Name.Length(), m_Name.Data());

    if (m_Flags & FILE_FLAG_FLUSH)
    {
        REV_WARNING_M("There is no sense of using Flush function with FILE_FLAG_FLUSH set, file: %.*s", m_Name.Length(), m_Name.Data());
    }
    else
    {
        Wait();
        REV_DEBUG_RESULT(FlushFileBuffers(m_Handle));
    }

    m_CriticalSection.Leave();
}

void AsyncFile::Copy(const ConstString& dest_filename, bool copy_if_exists) const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(dest_filename.Length() < REV_PATH_CAPACITY, "Destination fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    if (m_Name != dest_filename)
    {
        Wait();
        if (!CopyFileA(m_Name.Data(), dest_filename.Data(), !copy_if_exists))
        {
            u32 error_code = GetSysErrorCode();
            switch (error_code)
            {
                case ERROR_PATH_NOT_FOUND:
                {
                    Path path(dest_filename.SubString(0, dest_filename.RFind('/', 1)));
                    path.Inspect();
                    path.PrintWarningIfDoesNotExist("Destination path to file \"", dest_filename, "\" does not exist, so it will be created");
                    path.Create();

                    REV_DEBUG_RESULT(CopyFileA(m_Name.Data(), dest_filename.Data(), !copy_if_exists));
                } break;

                case ERROR_ALREADY_EXISTS:
                {
                    REV_WARNING_M("File already exists: %.*s", dest_filename.Length(), dest_filename.Data());
                } break;

                default:
                {
                    REV_WARNING_MS("File \"%.*s\" can not be copied to \"%.*s\"",
                                   m_Name.Length(), m_Name.Data(),
                                   dest_filename.Length(), dest_filename.Data());
                } break;
            }
        }
    }

    m_CriticalSection.Leave();
}

void AsyncFile::Move(const ConstString& dest_filename, bool move_if_exists)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(dest_filename.Length() < REV_PATH_CAPACITY, "Destination fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    if (m_Name != dest_filename)
    {
        Wait();
        if (!MoveFileExA(m_Name.Data(), dest_filename.Data(), move_if_exists ? MOVEFILE_REPLACE_EXISTING : 0))
        {
            u32 error_code = GetSysErrorCode();
            switch (error_code)
            {
                case ERROR_PATH_NOT_FOUND:
                {
                    Path path(dest_filename.SubString(0, dest_filename.RFind('/', 1)));
                    path.Inspect();
                    path.PrintWarningIfDoesNotExist("Destination path to file \"", dest_filename, "\" does not exist, so it will be created");
                    path.Create();
                    REV_DEBUG_RESULT(MoveFileExA(m_Name.Data(), dest_filename.Data(), move_if_exists ? MOVEFILE_REPLACE_EXISTING : 0));
                } break;

                case ERROR_ALREADY_EXISTS:
                {
                    REV_WARNING_M("File already exists: %.*s", dest_filename.Length(), dest_filename.Data());
                } break;

                default:
                {
                    REV_WARNING_MS("File \"%.*s\" can not be moved to \"%.*s\"",
                                   m_Name.Length(), m_Name.Data(),
                                   dest_filename.Length(), dest_filename.Data());
                } break;
            }
        }
        else
        {
            m_Name = dest_filename;
        }
    }

    m_CriticalSection.Leave();
}

void AsyncFile::Delete()
{
    m_CriticalSection.Enter();

    Close();
    m_Size = 0;

    if (!DeleteFileA(m_Name.Data()))
    {
        REV_WARNING_MS("File \"%.*s\" can not be deleted", m_Name.Length(), m_Name.Data());
    }

    m_CriticalSection.Leave();
}

void AsyncFile::GetTimings(u64& creation_time, u64& last_access_time, u64& last_write_time) const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_DEBUG_RESULT(GetFileTime(m_Handle,
                                 cast(FILETIME *, &creation_time),
                                 cast(FILETIME *, &last_access_time),
                                 cast(FILETIME *, &last_write_time)));

    m_CriticalSection.Leave();
}

u64 AsyncFile::CreationTime() const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME creation_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, &creation_time, null, null));

    m_CriticalSection.Leave();
    return *cast(u64 *, &creation_time);
}

u64 AsyncFile::LastAccessTime() const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME last_access_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, null, &last_access_time, null));

    m_CriticalSection.Leave();
    return *cast(u64 *, &last_access_time);
}

u64 AsyncFile::LastWriteTime() const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME last_write_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, null, null, &last_write_time));

    m_CriticalSection.Leave();
    return *cast(u64 *, &last_write_time);
}

AsyncFile& AsyncFile::operator=(nullptr_t)
{
    Close();
    return *this;
}

AsyncFile& AsyncFile::operator=(const AsyncFile& other)
{
    m_CriticalSection.Enter();
    other.m_CriticalSection.Enter();

    if (this != &other)
    {
        REV_CHECK_M(other.m_Handle != INVALID_HANDLE_VALUE, "Other file \"%.*s\" is closed", other.m_Name.Length(), other.m_Name.Data());

        Close();
        other.Wait();

        HANDLE process_handle = GetCurrentProcess();
        REV_DEBUG_RESULT(DuplicateHandle(process_handle, other.m_Handle, process_handle, &m_Handle, 0, false, DUPLICATE_SAME_ACCESS));

        m_Size  = other.m_Size;
        m_Flags = other.m_Flags;
        m_Name  = other.m_Name;
    }

    other.m_CriticalSection.Leave();
    m_CriticalSection.Leave();

    return *this;
}

AsyncFile& AsyncFile::operator=(AsyncFile&& other)
{
    m_CriticalSection.Enter();
    other.m_CriticalSection.Enter();

    if (this != &other)
    {
        Close();

        m_Handle          = other.m_Handle;
        m_Size            = other.m_Size;
        m_Flags           = other.m_Flags;
        m_CriticalSection = RTTI::move(other.m_CriticalSection);
        m_Name            = RTTI::move(other.m_Name);

        for (u32 i = 0; i < MAX_APCS; ++i)
        {
            APCEntry *entry       = m_APCEntries       + i;
            APCEntry *other_entry = other.m_APCEntries + i;

            REV_DEBUG_RESULT(CloseHandle(entry->overlapped.hEvent));

            entry->in_progress = other_entry->in_progress;
            CopyMemory(&entry->overlapped, &other_entry->overlapped, sizeof(OVERLAPPED));

            other_entry->overlapped.hEvent = null;
        }

        other.m_Handle = INVALID_HANDLE_VALUE;
    }

    other.m_CriticalSection.Leave();
    m_CriticalSection.Leave();

    return *this;
}

void AsyncFile::CreateAPCEntries()
{
    for (u32 i = 0; i < MAX_APCS; ++i)
    {
        APCEntry *entry = m_APCEntries + i;

        entry->file              = this;
        entry->overlapped.hEvent = CreateEventExA(null, null, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
        REV_CHECK(entry->overlapped.hEvent);
    }
}

void AsyncFile::DestroyAPCEntries()
{
    for (u32 i = 0; i < MAX_APCS; ++i)
    {
        APCEntry *entry = m_APCEntries + i;

        entry->file        = null;
        entry->in_progress = false;

        if (entry->overlapped.hEvent)
        {
            REV_DEBUG_RESULT(CloseHandle(entry->overlapped.hEvent));
            entry->overlapped.hEvent = null;
        }
    }
}

void AsyncFile::Open()
{
    u32 desired_access = 0;
    u32 shared_access  = 0;
    u32 disposition    = 0;
    u32 attributes     = 0;
    SplitFlagsToWin32Flags(desired_access, shared_access, disposition, attributes);

    m_Handle = CreateFileA(m_Name.Data(), desired_access, shared_access, null, disposition, attributes, null);
    if (m_Handle == INVALID_HANDLE_VALUE)
    {
        u32 error = GetSysErrorCode();
        switch (error)
        {
            case ERROR_FILE_NOT_FOUND:
            {
                Path path(m_Name.SubString(0, m_Name.RFind('/', 1)));
                path.Inspect();

                REV_CHECK(m_Flags & FILE_FLAG_EXISTS);
                path.PrintWarningIfDoesNotExist("File has been tried to be opened with flag FILE_FLAG_EXISTS but it does not exist");
            } break;

            case ERROR_PATH_NOT_FOUND:
            {
                Path path(m_Name.SubString(0, m_Name.RFind('/', 1)));
                path.Inspect();
                path.PrintWarningIfDoesNotExist("Path to file \"", m_Name, "\" does not exist, so it will be created");
                path.Create();

                m_Handle = CreateFileA(m_Name.Data(), desired_access, shared_access, null, disposition, attributes, null);
                REV_CHECK(m_Handle != INVALID_HANDLE_VALUE);
            } break;

            case ERROR_FILE_EXISTS:
            {
                REV_WARNING_M("File \"%.*s\" has been created with FILE_FLAG_NEW that guarantees a file creation only if it does NOT exist, but it does exist",
                              m_Name.Length(), m_Name.Data());
            } break;

            default:
            {
                REV_WARNING_MS("File \"%.*s\" can not be opened", m_Name.Length(), m_Name.Data());
            } break;
        }
    }
    else
    {
        REV_DEBUG_RESULT(GetFileSizeEx(m_Handle, (LARGE_INTEGER *)&m_Size));
    }
}

void AsyncFile::SplitFlagsToWin32Flags(u32& desired_access, u32& shared_access, u32& disposition, u32& attributes)
{
    attributes = FILE_FLAG_OVERLAPPED;

    if (m_Flags & FILE_FLAG_READ)
    {
        desired_access |= GENERIC_READ;
        shared_access  |= FILE_SHARE_READ;
        attributes     |= FILE_ATTRIBUTE_READONLY;
    }
    if (m_Flags & FILE_FLAG_WRITE)
    {
        desired_access |=  GENERIC_WRITE;
        shared_access  |=  FILE_SHARE_WRITE;
        attributes     &= ~FILE_ATTRIBUTE_READONLY;
        attributes     |=  FILE_ATTRIBUTE_NORMAL;
    }
    REV_DEBUG_RESULT_M(m_Flags & FILE_FLAG_RW, "A file must have FLAG_READ and/or FILE_WRITE flags");

    // @NOTE(Roman): CREATE_NEW        - if (exists) fails ,    ERROR_FILE_EXISTS    else creates
    //               CREATE_ALWAYS     - if (exists) truncates, ERROR_ALREADY_EXISTS else creates
    //               OPEN_EXISTING     - if (exists) succeeds                        else fails,  ERROR_FILE_NOT_FOUND
    //               OPEN_ALWAYS       - if (exists) succeeds,  ERROR_ALREADY_EXISTS else creates
    //               TRUNCATE_EXISTING - if (exists) truncates                       else fails,  ERROR_FILE_NOT_FOUND. Must have GENERIC_WRITE.
    if (m_Flags & FILE_FLAG_EXISTS)
    {
        if (m_Flags & FILE_FLAG_TRUNCATE)
        {
            REV_DEBUG_RESULT_M(m_Flags & FILE_FLAG_WRITE, "For truncating an existing file, you have to open it with the write access");
            disposition = TRUNCATE_EXISTING;
        }
        else
        {
            disposition = OPEN_EXISTING;
        }
    }
    else if (m_Flags & FILE_FLAG_NEW)
    {
        disposition = CREATE_NEW;
    }
    else
    {
        if (m_Flags & FILE_FLAG_TRUNCATE) disposition = CREATE_ALWAYS;
        else                              disposition = OPEN_ALWAYS;
    }

    /**/ if (m_Flags & FILE_FLAG_RAND) attributes |= FILE_FLAG_RANDOM_ACCESS;
    else if (m_Flags & FILE_FLAG_SEQ)  attributes |= FILE_FLAG_SEQUENTIAL_SCAN;

    if (m_Flags & FILE_FLAG_TEMP)
    {
        shared_access |= FILE_SHARE_DELETE;
        attributes    |= FILE_ATTRIBUTE_TEMPORARY
                      |  FILE_FLAG_DELETE_ON_CLOSE;
    }
    if (m_Flags & FILE_FLAG_FLUSH)
    {
        attributes |= FILE_FLAG_WRITE_THROUGH;
    }
}

void AsyncFile::LockSystemCacheFromOtherProcesses(u64 offset, u64 bytes, bool shared) const
{
    ULARGE_INTEGER bytes_to_lock;
    bytes_to_lock.QuadPart = bytes;

    OVERLAPPED overlapped = {0};
    overlapped.Pointer    = (void *)offset;

    REV_DEBUG_RESULT(LockFileEx(m_Handle,
                                shared ? 0 : LOCKFILE_EXCLUSIVE_LOCK,
                                0,
                                bytes_to_lock.LowPart,
                                bytes_to_lock.HighPart,
                                &overlapped));
}

void AsyncFile::UnlockSystemCacheFromOtherProcesses(u64 offset, u64 bytes) const
{
    ULARGE_INTEGER bytes_to_unlock;
    bytes_to_unlock.QuadPart = bytes;

    OVERLAPPED overlapped = {0};
    overlapped.Pointer    = (void *)offset;

    REV_DEBUG_RESULT(UnlockFileEx(m_Handle,
                                  0,
                                  bytes_to_unlock.LowPart,
                                  bytes_to_unlock.HighPart,
                                  &overlapped));
}

void WINAPI OverlappedReadCompletionRoutine(u32 error_code, u32 bytes_transfered, OVERLAPPED *overlapped)
{
    AsyncFile::APCEntry *entry = cast(AsyncFile::APCEntry *, cast(byte *, overlapped) - REV_StructFieldOffset(AsyncFile::APCEntry, overlapped));

    entry->file->m_CriticalSection.Enter();

    entry->file->UnlockSystemCacheFromOtherProcesses(cast(u64, overlapped->Pointer), bytes_transfered);
    entry->in_progress = false;

    entry->file->m_CriticalSection.Leave();
}

void WINAPI OverlappedWriteCompletionRoutine(u32 error_code, u32 bytes_transfered, OVERLAPPED *overlapped)
{
    AsyncFile::APCEntry *entry = cast(AsyncFile::APCEntry *, cast(byte *, overlapped) - REV_StructFieldOffset(AsyncFile::APCEntry, overlapped));

    entry->file->m_CriticalSection.Enter();

    u64 old_offset = cast(u64, overlapped->Pointer);
    u64 new_offset = old_offset + bytes_transfered;

    if (new_offset > entry->file->m_Size)
    {
        entry->file->m_Size = new_offset;
    }

    entry->file->UnlockSystemCacheFromOtherProcesses(old_offset, bytes_transfered);
    entry->in_progress = false;

    entry->file->m_CriticalSection.Leave();
}

}
