// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "tools/file.h"
#include "memory/memory.h"
#include "tools/static_string_builder.hpp"

namespace REV
{

File::File(nullptr_t)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Offset(0),
      m_Size(0),
      m_CriticalSection(),
      m_Flags(FLAG_NONE),
      m_Name(null)
{
}

File::File(const ConstString& filename, FLAG flags)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Offset(0),
      m_Size(0),
      m_CriticalSection(),
      m_Flags(flags),
      m_Name(filename)
{
    REV_CHECK_M(m_Name.Length() < REV_PATH_CAPACITY, "Fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    Open();
}

File::File(const File& other)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Offset(other.m_Offset),
      m_Size(other.m_Size),
      m_CriticalSection(),
      m_Flags(other.m_Flags),
      m_Name(other.m_Name)
{
    REV_CHECK_M(other.m_Handle != INVALID_HANDLE_VALUE, "Other file \"%.*s\" is closed", other.m_Name.Length(), other.m_Name.Data());

    HANDLE process_handle = GetCurrentProcess();
    REV_DEBUG_RESULT(DuplicateHandle(process_handle, other.m_Handle, process_handle, &m_Handle, 0, false, DUPLICATE_SAME_ACCESS));
}

File::File(File&& other)
    : m_Handle(other.m_Handle),
      m_Offset(other.m_Offset),
      m_Size(other.m_Size),
      m_CriticalSection(RTTI::move(other.m_CriticalSection)),
      m_Flags(other.m_Flags),
      m_Name(other.m_Name)
{
    other.m_Handle = INVALID_HANDLE_VALUE;
}

File::~File()
{
    // @Issue(Roman): Do we really need enter critical section in destructor?
    //                Theoretically it will be destroyed in a thread a file was created,
    //                btw destructor can be called explicitly in several threads.
    m_CriticalSection.Enter();

    Close();
    m_Offset = 0;
    m_Size   = 0;
    m_Flags  = FLAG_NONE;

    m_CriticalSection.Leave();
}

bool File::Open(const ConstString& filename, FLAG flags)
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

void File::ReOpen(FLAG new_flags)
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

        m_Handle = ReOpenFile(m_Handle, desired_access, shared_access, attributes);
        REV_CHECK(m_Handle != INVALID_HANDLE_VALUE);
    }
    m_CriticalSection.Leave();
}

void File::Close()
{
    m_CriticalSection.Enter();
    if (m_Handle != INVALID_HANDLE_VALUE)
    {
        REV_DEBUG_RESULT(CloseHandle(m_Handle));
        m_Handle = INVALID_HANDLE_VALUE;
    }
    m_CriticalSection.Leave();
}

void File::Clear()
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FLAG_WRITE, "You have no rights to clear file \"%.*s\". You must have write rights", m_Name.Length(), m_Name.Data());

    m_Offset = SetFilePointer(m_Handle, 0, null, FILE_BEGIN);
    REV_CHECK(m_Offset == 0);

    REV_DEBUG_RESULT(SetEndOfFile(m_Handle));
    m_Size = 0;

    m_CriticalSection.Leave();
}

void File::Read(void *buffer, u64 bytes)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FLAG_READ, "You have no rights to read file \"%.*s\"", m_Name.Length(), m_Name.Data());
    REV_CHECK(buffer);
    REV_CHECK_M(bytes, "Buffer size must be more than 0");

    u64 saved_offset = m_Offset;    
    LockSystemCacheFromOtherProcesses(saved_offset, bytes, true);

    u32 bytes_read = 0;
    for (u64 offset = 0; offset < bytes; offset += bytes_read)
    {
        u64 rest_bytes    = bytes - offset;
        u32 bytes_to_read = rest_bytes < REV_U32_MAX ? (u32)rest_bytes : REV_U32_MAX;

        REV_DEBUG_RESULT(ReadFile(m_Handle, (byte *)buffer + offset, bytes_to_read, &bytes_read, null));

        m_Offset += bytes_read;
    }

    UnlockSystemCacheFromOtherProcesses(saved_offset, bytes);
    m_CriticalSection.Leave();
}

void File::Write(const void *buffer, u64 bytes)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FLAG_WRITE, "You have no rights to write to file \"%.*s\"", m_Name.Length(), m_Name.Data());
    REV_CHECK(buffer);
    REV_CHECK_M(bytes, "Buffer size must be more than 0");

    u64 saved_offset = m_Offset;
    LockSystemCacheFromOtherProcesses(saved_offset, bytes, false);

    u32 bytes_written = 0;
    for (u64 offset = 0; offset < bytes; offset += bytes_written)
    {
        u64 rest_bytes     = bytes - offset;
        u32 bytes_to_write = rest_bytes < REV_U32_MAX ? (u32)rest_bytes : REV_U32_MAX;

        REV_DEBUG_RESULT(WriteFile(m_Handle, (byte *)buffer + offset, bytes_to_write, &bytes_written, null));

        m_Offset += bytes_written;
    }

    if (m_Offset > m_Size) m_Size = m_Offset;

    UnlockSystemCacheFromOtherProcesses(saved_offset, bytes);
    m_CriticalSection.Leave();
}

void File::Append(const void *buffer, u64 bytes)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FLAG_WRITE, "You have no rights to write to file \"%.*s\"", m_Name.Length(), m_Name.Data());
    REV_CHECK(buffer);
    REV_CHECK_M(bytes, "Buffer size must be more than 0");

    REV_DEBUG_RESULT(SetFilePointerEx(m_Handle, LARGE_INTEGER{0}, (LARGE_INTEGER *)&m_Offset, FILE_END));

    u64 saved_offset = m_Offset;
    LockSystemCacheFromOtherProcesses(saved_offset, bytes, false);

    u32 bytes_written = 0;
    for (u64 offset = 0; offset < bytes; offset += bytes_written)
    {
        u64 rest_bytes     = bytes - offset;
        u32 bytes_to_write = rest_bytes < REV_U32_MAX ? (u32)rest_bytes : REV_U32_MAX;

        REV_DEBUG_RESULT(WriteFile(m_Handle, (byte *)buffer + offset, bytes_to_write, &bytes_written, null));

        m_Offset += bytes_written;
    }

    if (m_Offset > m_Size) m_Size = m_Offset;

    UnlockSystemCacheFromOtherProcesses(saved_offset, bytes);
    m_CriticalSection.Leave();
}

void File::Flush()
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FLAG_WRITE, "You have no rights to flush file \"%.*s\". You must have write rights", m_Name.Length(), m_Name.Data());

    if (m_Flags & FLAG_FLUSH)
    {
        REV_WARNING_M("There is no sense of using Flush function with FLAG_FLUSH set, file: %.*s", m_Name.Length(), m_Name.Data());
    }
    else
    {
        REV_DEBUG_RESULT(FlushFileBuffers(m_Handle));
    }

    m_CriticalSection.Leave();
}

void File::Copy(const ConstString& dest_filename, bool copy_if_exists) const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(dest_filename.Length() < REV_PATH_CAPACITY, "Destination fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    if (m_Name != dest_filename)
    {
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

void File::Move(const ConstString& dest_filename, bool move_if_exists)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(dest_filename.Length() < REV_PATH_CAPACITY, "Destination fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    if (m_Name != dest_filename)
    {
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

void File::Delete()
{
    m_CriticalSection.Enter();

    m_Offset = 0;
    m_Size   = 0;

    Close();

    if (!DeleteFileA(m_Name.Data()))
    {
        REV_WARNING_MS("File \"%.*s\" can not be deleted", m_Name.Length(), m_Name.Data());
    }

    m_CriticalSection.Leave();
}

void File::Copy(const ConstString& src_filename, const ConstString& dest_filename, bool copy_if_exists)
{
    REV_CHECK_M(src_filename.Length()  < REV_PATH_CAPACITY, "Source fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    REV_CHECK_M(dest_filename.Length() < REV_PATH_CAPACITY, "Destination fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    if (src_filename != dest_filename)
    {
        if (!CopyFileA(src_filename.Data(), dest_filename.Data(), !copy_if_exists))
        {
            u32 sys_error = GetSysErrorCode();
            switch (sys_error)
            {
                case ERROR_FILE_NOT_FOUND:
                {
                    Path src_path(src_filename);
                    src_path.Inspect();
                    src_path.PrintWarningIfDoesNotExist("Source file does not exist");
                } break;

                case ERROR_PATH_NOT_FOUND:
                {
                    Path dest_path(dest_filename.SubString(0, dest_filename.RFind('/')));
                    dest_path.Inspect();
                    dest_path.PrintWarningIfDoesNotExist("Destination path to file \"", dest_filename, "\" does not exist, so it will be created");
                    dest_path.Create();

                    REV_DEBUG_RESULT(CopyFileA(src_filename.Data(), dest_filename.Data(), !copy_if_exists));
                } break;

                case ERROR_ALREADY_EXISTS:
                {
                    REV_WARNING_M("Destination file already exists: %.*s", dest_filename.Length(), dest_filename.Data());
                } break;

                default:
                {
                    REV_WARNING_MS("File \"%.*s\" can not be copied to \"%.*s\"",
                                   src_filename.Length(), src_filename.Data(),
                                   dest_filename.Length(), dest_filename.Data());
                } break;
            }
        }
    }
}

void File::Move(const ConstString& src_filename, const ConstString& dest_filename, bool move_if_exists)
{
    REV_CHECK_M(src_filename.Length()  < REV_PATH_CAPACITY, "Source fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    REV_CHECK_M(dest_filename.Length() < REV_PATH_CAPACITY, "Destination fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    if (src_filename != dest_filename)
    {
        if (!MoveFileExA(src_filename.Data(), dest_filename.Data(), move_if_exists ? MOVEFILE_REPLACE_EXISTING : 0))
        {
            u32 sys_error = GetSysErrorCode();
            switch (sys_error)
            {
                case ERROR_FILE_NOT_FOUND:
                {
                    Path src_path(src_filename);
                    src_path.Inspect();
                    src_path.PrintWarningIfDoesNotExist("Source file does not exist");
                } break;

                case ERROR_PATH_NOT_FOUND:
                {
                    Path dest_path(dest_filename.SubString(0, dest_filename.RFind('/')));
                    dest_path.Inspect();
                    dest_path.PrintWarningIfDoesNotExist("Destination path to file \"", dest_filename, "\" does not exist, so it will be created");
                    dest_path.Create();

                    REV_DEBUG_RESULT(MoveFileExA(src_filename.Data(), dest_filename.Data(), move_if_exists ? MOVEFILE_REPLACE_EXISTING : 0));
                } break;

                case ERROR_ALREADY_EXISTS:
                {
                    REV_WARNING_M("Destination file already exists: %.*s", dest_filename.Length(), dest_filename.Data());
                } break;

                default:
                {
                    REV_WARNING_MS("File \"%.*s\" can not be moved to \"%.*s\"",
                                   src_filename.Length(), src_filename.Data(),
                                   dest_filename.Length(), dest_filename.Data());
                } break;
            }
        }
    }
}

void File::Delete(const ConstString& filename)
{
    REV_CHECK_M(filename.Length() < REV_PATH_CAPACITY, "Fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    if (!DeleteFileA(filename.Data()))
    {
        u32 sys_error = GetSysErrorCode();
        switch (sys_error)
        {
            case ERROR_FILE_NOT_FOUND:
            {
                Path src_path(filename);
                src_path.Inspect();
                src_path.PrintWarningIfDoesNotExist("File does not exist");
            } break;

            case ERROR_PATH_NOT_FOUND:
            {
                Path dest_path(filename.SubString(0, filename.RFind('/')));
                dest_path.Inspect();
                dest_path.PrintWarningIfDoesNotExist("Path to the file \"", filename, "\" does not exist");
            } break;

            default:
            {
                REV_WARNING_MS("File \"%.*s\" can not be deleted", filename.Length(), filename.Data());
            } break;
        }
    }
}

bool File::Exists(const ConstString& filename)
{
    REV_CHECK_M(filename.Length() < REV_PATH_CAPACITY, "Fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    return PathFileExistsA(filename.Data());
}

void File::GetTimings(u64& creation_time, u64& last_access_time, u64& last_write_time) const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_DEBUG_RESULT(GetFileTime(m_Handle,
                                 cast(FILETIME *, &creation_time),
                                 cast(FILETIME *, &last_access_time),
                                 cast(FILETIME *, &last_write_time)));

    m_CriticalSection.Leave();
}

u64 File::CreationTime() const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME creation_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, &creation_time, null, null));

    m_CriticalSection.Leave();
    return *cast(u64 *, &creation_time);
}

u64 File::LastAccessTime() const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME last_access_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, null, &last_access_time, null));

    m_CriticalSection.Leave();
    return *cast(u64 *, &last_access_time);
}

u64 File::LastWriteTime() const
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME last_write_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, null, null, &last_write_time));

    m_CriticalSection.Leave();
    return *cast(u64 *, &last_write_time);
}

void File::Find(const ConstString& filename, const Function<FIND_RESULT(const ConstString& found_filename, bool file_not_found)>& FindFileCallback)
{
    REV_CHECK_M(filename.Length() < REV_PATH_CAPACITY, "Fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);

    WIN32_FIND_DATAA win32_find_data = {};
    HANDLE           find_handle     = FindFirstFileExA(filename.Data(),
                                                        FindExInfoBasic, &win32_find_data,
                                                        FindExSearchNameMatch, null,
                                                        FIND_FIRST_EX_CASE_SENSITIVE | FIND_FIRST_EX_LARGE_FETCH);
    u32              sys_error       = GetSysErrorCode();
    FIND_RESULT      find_result     = FIND_RESULT_CONTINUE;
    bool             file_not_found  = false;

    do
    {
        ConstString found_filename(win32_find_data.cFileName, strlen(win32_find_data.cFileName));
        file_not_found = sys_error == ERROR_FILE_NOT_FOUND || sys_error == ERROR_PATH_NOT_FOUND;

        find_result = FindFileCallback(found_filename, file_not_found);
        if (find_result != FIND_RESULT_CONTINUE) break;

        FindNextFileA(find_handle, &win32_find_data);
        sys_error = GetSysErrorCode();

    } while (sys_error != ERROR_NO_MORE_FILES && !file_not_found);

    if (find_handle != INVALID_HANDLE_VALUE)
    {
        REV_DEBUG_RESULT(FindClose(find_handle));
    }

    if (file_not_found)
    {
        Path path(filename);
        path.Inspect();
        path.PrintWarningIfDoesNotExist("There are no files matching following wildcard");
    }
    else if (find_result != FIND_RESULT_FOUND)
    {
        if (sys_error == ERROR_NO_MORE_FILES)
        {
            REV_WARNING_M("There are no more files matching following wildcard: \"%.*s\"",
                          filename.Length(), filename.Data());
        }
        else
        {
            REV_CHECK(sys_error == ERROR_SUCCESS);
        }
    }
}

void File::SetOffset(s64 offset)
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    LARGE_INTEGER new_offset = {0};
    new_offset.QuadPart = offset;

    REV_DEBUG_RESULT(SetFilePointerEx(m_Handle, new_offset, (LARGE_INTEGER *)&m_Offset, offset < 0 ? FILE_END : FILE_BEGIN));

    m_CriticalSection.Leave();
}

void File::GoToEOF()
{
    m_CriticalSection.Enter();

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    REV_DEBUG_RESULT(SetFilePointerEx(m_Handle, LARGE_INTEGER{0}, (LARGE_INTEGER *)&m_Offset, FILE_END));

    m_CriticalSection.Leave();
}

File& File::operator=(nullptr_t)
{
    Close();
    return *this;
}

File& File::operator=(const File& other)
{
    m_CriticalSection.Enter();

    if (this != &other)
    {
        REV_CHECK_M(other.m_Handle != INVALID_HANDLE_VALUE, "Other file \"%.*s\" is closed", other.m_Name.Length(), other.m_Name.Data());

        Close();

        HANDLE process_handle = GetCurrentProcess();
        REV_DEBUG_RESULT(DuplicateHandle(process_handle, other.m_Handle, process_handle, &m_Handle, 0, false, DUPLICATE_SAME_ACCESS));

        m_Offset = other.m_Offset;
        m_Size   = other.m_Size;
        m_Flags  = other.m_Flags;
        m_Name   = other.m_Name;
    }

    m_CriticalSection.Leave();
    return *this;
}

File& File::operator=(File&& other)
{
    m_CriticalSection.Enter();

    if (this != &other)
    {
        Close();

        m_Handle = other.m_Handle;
        m_Offset = other.m_Offset;
        m_Size   = other.m_Size;
        m_Flags  = other.m_Flags;
        m_Name   = RTTI::move(other.m_Name);

        other.m_Handle = INVALID_HANDLE_VALUE;
    }

    m_CriticalSection.Leave();
    return *this;
}

void File::Open()
{
    m_CriticalSection.Enter();

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

                REV_CHECK(m_Flags & FLAG_EXISTS);
                path.PrintWarningIfDoesNotExist("File has been tried to be opened with flag FLAG_EXISTS but it does not exist");
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
                REV_WARNING_M("File \"%.*s\" has been created with FLAG_NEW that guarantees a file creation only if it does NOT exist, but it does exist",
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
        LARGE_INTEGER pos = {0};
        REV_DEBUG_RESULT(SetFilePointerEx(m_Handle, pos, (LARGE_INTEGER *)&m_Offset, FILE_CURRENT));
        REV_DEBUG_RESULT(GetFileSizeEx(m_Handle, (LARGE_INTEGER *)&m_Size));
    }

    m_CriticalSection.Leave();
}

void File::SplitFlagsToWin32Flags(u32& desired_access, u32& shared_access, u32& disposition, u32& attributes)
{
    if (m_Flags & FLAG_READ)
    {
        desired_access  = GENERIC_READ;
        shared_access   = FILE_SHARE_READ;
        attributes     |= FILE_ATTRIBUTE_READONLY;
    }
    if (m_Flags & FLAG_WRITE)
    {
        desired_access |=  GENERIC_WRITE;
        shared_access  |=  FILE_SHARE_WRITE;
        attributes     &= ~FILE_ATTRIBUTE_READONLY;
        attributes     |=  FILE_ATTRIBUTE_NORMAL;
    }
    REV_DEBUG_RESULT_M(m_Flags & FLAG_RW, "A file must have FLAG_READ and/or FILE_WRITE flags");

    // @NOTE(Roman): CREATE_NEW        - if (exists) fails ,    ERROR_FILE_EXISTS    else creates
    //               CREATE_ALWAYS     - if (exists) truncates, ERROR_ALREADY_EXISTS else creates
    //               OPEN_EXISTING     - if (exists) succeeds                        else fails,  ERROR_FILE_NOT_FOUND
    //               OPEN_ALWAYS       - if (exists) succeeds,  ERROR_ALREADY_EXISTS else creates
    //               TRUNCATE_EXISTING - if (exists) truncates                       else fails,  ERROR_FILE_NOT_FOUND. Must have GENERIC_WRITE.
    if (m_Flags & FLAG_EXISTS)
    {
        if (m_Flags & FLAG_TRUNCATE)
        {
            REV_DEBUG_RESULT_M(m_Flags & FLAG_WRITE, "For truncating an existing file, you have to open it with the write access");
            disposition = TRUNCATE_EXISTING;
        }
        else
        {
            disposition = OPEN_EXISTING;
        }
    }
    else if (m_Flags & FLAG_NEW)
    {
        disposition = CREATE_NEW;
    }
    else
    {
        if (m_Flags & FLAG_TRUNCATE) disposition = CREATE_ALWAYS;
        else                         disposition = OPEN_ALWAYS;
    }

    /**/ if (m_Flags & FLAG_RAND) attributes |= FILE_FLAG_RANDOM_ACCESS;
    else if (m_Flags & FLAG_SEQ)  attributes |= FILE_FLAG_SEQUENTIAL_SCAN;

    if (m_Flags & FLAG_TEMP)
    {
        shared_access |= FILE_SHARE_DELETE;
        attributes    |= FILE_ATTRIBUTE_TEMPORARY
                      |  FILE_FLAG_DELETE_ON_CLOSE;
    }
    if (m_Flags & FLAG_FLUSH)
    {
        attributes |= FILE_FLAG_WRITE_THROUGH;
    }
}

void File::LockSystemCacheFromOtherProcesses(u64 offset, u64 bytes, bool shared)
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

void File::UnlockSystemCacheFromOtherProcesses(u64 offset, u64 bytes)
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

//
// Path
//

bool Path::Exists()
{
    return does_not_exist.Empty()
        && PathFileExistsA(path.Data());
}

Path& Path::Inspect()
{
    StaticString<REV_PATH_CAPACITY> null_terminated_subpath;

    for (ConstString subpath = path;
         subpath.Length();
         subpath = subpath.SubString(0, subpath.RFind('/', 1)))
    {
        null_terminated_subpath = subpath;

        if (Path::Exists(null_terminated_subpath))
        {
            exists.AssignCSTR(subpath.Data(), subpath.Length() + 1);
            does_not_exist.AssignCSTR(path.Data() + subpath.Length() + 1, path.Length() - subpath.Length() - 1);
            break;
        }
    }

    return *this;
}

Path& Path::Create()
{
    if (!CreateDirectoryA(path.Data(), null))
    {
        switch (GetSysErrorCode())
        {
            case ERROR_ALREADY_EXISTS:
            {
                // @NOTE(Roman): Do nothing, already exists.
            } break;

            case ERROR_PATH_NOT_FOUND:
            {
                Path subpath(path.SubString(0, path.RFind('/', 1)));
                subpath.Create();

                REV_DEBUG_RESULT(CreateDirectoryA(path.Data(), null));
            } break;

            default:
            {
                REV_WARNING_MS("Path can not be created");
            } break;
        }
    }
    return *this;
}

bool Path::Exists(const ConstString& path)
{
    REV_CHECK_M(path.Length() < REV_PATH_CAPACITY, "Path is to long, max available length is: %I32u", REV_PATH_CAPACITY);
    return PathFileExistsA(path.Data());
}

void Path::_PrintWarningIfDoesNotExist(const ConstString& warning_message)
{
    if (does_not_exist.Length())
    {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (console != INVALID_HANDLE_VALUE)
        {
            StaticStringBuilder<2048> builder;
            builder.Build("Debug message(Warning): ");

            if (warning_message.Length()) builder.Build(warning_message);
            else                          builder.Build("Path does not exist");
            builder.Build(': "');

            REV_DEBUG_RESULT(SetConsoleTextAttribute(console, cast(u16, DEBUG_COLOR::WARNING)));
            REV_DEBUG_RESULT(WriteConsoleA(console, builder.BufferData(), (DWORD)builder.BufferLength(), null, null));
            if (exists.Length())
            {
                REV_DEBUG_RESULT(SetConsoleTextAttribute(console, cast(u16, DEBUG_COLOR::SUCCESS)));
                REV_DEBUG_RESULT(WriteConsoleA(console, exists.Data(), (DWORD)exists.Length(), null, null));
            }
            REV_DEBUG_RESULT(SetConsoleTextAttribute(console, cast(u16, DEBUG_COLOR::ERROR)));
            REV_DEBUG_RESULT(WriteConsoleA(console, does_not_exist.Data(), (DWORD)does_not_exist.Length(), null, null));
            REV_DEBUG_RESULT(SetConsoleTextAttribute(console, cast(u16, DEBUG_COLOR::INFO)));
            REV_DEBUG_RESULT(WriteConsoleA(console, REV_CSTR_ARGS("\"\n"), null, null));
        }
    }
}

}
