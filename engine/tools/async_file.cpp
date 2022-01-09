// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "tools/async_file.h"
#include "tools/scoped_lock.hpp"

namespace REV
{

REV_GLOBAL WorkQueue *g_AsyncFilesIOQueue = null;

REV_INTERNAL REV_INLINE u32 GetPageSize()
{
    SYSTEM_INFO sys_info{};
    GetSystemInfo(&sys_info);
    return sys_info.dwPageSize;
}

AsyncFile::AsyncFile(nullptr_t)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Size(0),
      m_Mapping(null),
      m_MappingSize(0),
      m_View(null),
      m_ViewStart(0),
      m_ViewEnd(0),
      m_Flags(FILE_FLAG_NONE),
      m_PageSize(GetPageSize()),
      m_ChunkSize(0),
      m_IOOpsCount(0),
      m_CriticalSection(),
      m_Name(null)
{
    if (!g_AsyncFilesIOQueue)
    {
        Memory *memory = Memory::Get();
        g_AsyncFilesIOQueue = new (memory->PushToPA<WorkQueue>()) WorkQueue(memory->PermanentArena(), 64);
    }
}

AsyncFile::AsyncFile(const ConstString& filename, FILE_FLAG flags)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Size(0),
      m_Mapping(null),
      m_MappingSize(0),
      m_View(null),
      m_ViewStart(0),
      m_ViewEnd(0),
      m_Flags(flags),
      m_PageSize(GetPageSize()),
      m_ChunkSize(0),
      m_IOOpsCount(0),
      m_CriticalSection(),
      m_Name(filename)
{
    REV_CHECK_M(m_Name.Length() < REV_PATH_CAPACITY, "Fileanme is to long, max available length is: %I32u", REV_PATH_CAPACITY);

    if (!g_AsyncFilesIOQueue)
    {
        Memory *memory = Memory::Get();
        g_AsyncFilesIOQueue = new (memory->PushToPA<WorkQueue>()) WorkQueue(memory->PermanentArena(), 64);
    }

    Open();
}

AsyncFile::AsyncFile(const AsyncFile& other)
    : m_Handle(INVALID_HANDLE_VALUE),
      m_Size(other.m_Size),
      m_Mapping(other.m_Mapping),
      m_MappingSize(other.m_MappingSize),
      m_View(null),
      m_ViewStart(0),
      m_ViewEnd(0),
      m_Flags(other.m_Flags),
      m_PageSize(other.m_PageSize),
      m_ChunkSize(other.m_ChunkSize),
      m_IOOpsCount(0),
      m_CriticalSection(),
      m_Name(other.m_Name)
{
    HANDLE current_process = GetCurrentProcess();
    REV_DEBUG_RESULT(DuplicateHandle(current_process, other.m_Handle, current_process, &m_Handle, 0, false, DUPLICATE_SAME_ACCESS));
}

AsyncFile::AsyncFile(AsyncFile&& other)
    : m_Handle(other.m_Handle),
      m_Size(other.m_Size),
      m_Mapping(other.m_Mapping),
      m_MappingSize(other.m_MappingSize),
      m_View(other.m_View),
      m_ViewStart(other.m_ViewStart),
      m_ViewEnd(other.m_ViewEnd),
      m_Flags(other.m_Flags),
      m_PageSize(other.m_PageSize),
      m_ChunkSize(other.m_ChunkSize),
      m_IOOpsCount(other.m_IOOpsCount),
      m_CriticalSection(RTTI::move(other.m_CriticalSection)),
      m_Name(RTTI::move(other.m_Name))
{
    other.m_Handle = INVALID_HANDLE_VALUE;
}

AsyncFile::~AsyncFile()
{
    Close();
}

bool AsyncFile::Open(const ConstString& filename, FILE_FLAG flags)
{
    REV_SCOPED_LOCK(m_CriticalSection);

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

    return m_Handle != INVALID_HANDLE_VALUE;
}

void AsyncFile::ReOpen(FILE_FLAG new_flags)
{
    REV_SCOPED_LOCK(m_CriticalSection);
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
}

bool AsyncFile::Close()
{
    REV_SCOPED_LOCK(m_CriticalSection);

    if (m_Handle != INVALID_HANDLE_VALUE)
    {
        Wait();

        if (m_View)
        {
            REV_DEBUG_RESULT(UnmapViewOfFile(m_View));
            m_View      = null;
            m_ViewStart = 0;
            m_ViewEnd   = 0;
        }

        if (m_Mapping)
        {
            REV_DEBUG_RESULT(CloseHandle(m_Mapping));
            m_Mapping     = null;
            m_MappingSize = 0;
        }

        REV_DEBUG_RESULT(CloseHandle(m_Handle));
        m_Handle = INVALID_HANDLE_VALUE;

        _InterlockedExchange64(cast(volatile s64 *, m_Size), 0);
        m_Flags = FILE_FLAG_NONE;

        return true;
    }

    return false;
}

void AsyncFile::Clear()
{
    REV_SCOPED_LOCK(m_CriticalSection);

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FILE_FLAG_WRITE, "You have no rights to clear file \"%.*s\". You must have write rights", m_Name.Length(), m_Name.Data());

    Wait();

    if (m_View)
    {
        UnmapViewOfFile(m_View);
        m_View      = null;
        m_ViewStart = 0;
        m_ViewEnd   = 0;
    }

    REV_DEBUG_RESULT(!SetFilePointer(m_Handle, 0, null, FILE_BEGIN));
    REV_DEBUG_RESULT(SetEndOfFile(m_Handle));

    _InterlockedExchange64(cast(volatile s64 *, m_Size), 0);
}

void AsyncFile::Read(void *buffer, u64 bytes, u64 read_offset) const
{
    if (buffer && bytes)
    {
        u64 size = m_Size;

        REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "AsyncFile \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
        REV_CHECK_M(m_Flags & FILE_FLAG_READ, "You have no rights to read this file");
        REV_CHECK_M(read_offset < size, "File read offset is too high. Offset: %I64u, current file size: %I64u", read_offset, size);

        if (read_offset + bytes > size)
        {
            u64 new_bytes_to_read = size - read_offset;
            REV_WARNING_M("File read offset is %I64u, bytes to read is %I64u, file size is %I64u. You are going out of bounds. Will be read only %I64u first bytes from the read offset",
                          read_offset, bytes, size, new_bytes_to_read);
            bytes = new_bytes_to_read;
        }

        const_cast<AsyncFile *>(this)->UpdateMappingIfNeeded(read_offset, read_offset + bytes);

        u64 chunk_size = m_ChunkSize;
        if (chunk_size)
        {
            for (u64 chunk_offset = 0; chunk_offset < bytes; chunk_offset += chunk_size)
            {
                _InterlockedIncrement64(cast(volatile s64 *, &m_IOOpsCount));

                g_AsyncFilesIOQueue->AddWork([this, buffer, bytes, view_offset = read_offset - m_ViewStart, chunk_offset, chunk_size]
                {
                    byte *dest_read     = cast(byte *, buffer) + chunk_offset;
                    byte *src_read      = cast(byte *, m_View) + view_offset + chunk_offset;
                    u64   bytes_to_read = Math::min(chunk_size, bytes - chunk_offset);

                    CopyMemory(dest_read, src_read, bytes_to_read);

                    _InterlockedDecrement64(cast(volatile s64 *, &m_IOOpsCount));
                });
            }
        }
        else
        {
            _InterlockedIncrement64(cast(volatile s64 *, &m_IOOpsCount));

            g_AsyncFilesIOQueue->AddWork([this, buffer, bytes, view_offset = read_offset - m_ViewStart]
            {
                CopyMemory(buffer, cast(byte *, m_View) + view_offset, bytes);

                _InterlockedDecrement64(cast(volatile s64 *, &m_IOOpsCount));
            });
        }
    }
}

void AsyncFile::Write(const void *buffer, u64 bytes, u64 write_offset)
{
    if (buffer && bytes)
    {
        u64 size = m_Size;

        REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "AsyncFile \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
        REV_CHECK_M(m_Flags & FILE_FLAG_WRITE, "You have no rights to write to this file");

        UpdateMappingIfNeeded(write_offset, write_offset + bytes);

        u64 chunk_size = m_ChunkSize;
        if (chunk_size)
        {
            for (u64 chunk_offset = 0; chunk_offset < bytes; chunk_offset += chunk_size)
            {
                _InterlockedIncrement64(cast(volatile s64 *, &m_IOOpsCount));

                g_AsyncFilesIOQueue->AddWork([this, buffer, bytes, write_offset, chunk_offset, chunk_size]
                {
                    u64 view_offset = write_offset - m_ViewStart + chunk_offset;

                    byte *dest_write     = cast(byte *, m_View) + view_offset;
                    byte *src_write      = cast(byte *, buffer) + chunk_offset;
                    u64   bytes_to_write = Math::min(chunk_size, bytes - chunk_offset);

                    CopyMemory(dest_write, src_write, bytes_to_write);

                    u64 new_size = write_offset + bytes_to_write;
                    if (new_size > m_Size)
                    {
                        _InterlockedExchange64(cast(volatile s64 *, &m_Size), new_size);
                    }

                    FlushViewIfNeeded(view_offset, bytes_to_write);

                    _InterlockedDecrement64(cast(volatile s64 *, &m_IOOpsCount));
                });
            }
        }
        else
        {
            _InterlockedIncrement64(cast(volatile s64 *, &m_IOOpsCount));

            g_AsyncFilesIOQueue->AddWork([this, buffer, bytes, write_offset]
            {
                u64 view_offset = write_offset - m_ViewStart;

                CopyMemory(cast(byte *, m_View) + view_offset, buffer, bytes);

                u64 new_size = write_offset + bytes;
                if (new_size > m_Size)
                {
                    _InterlockedExchange64(cast(volatile s64 *, &m_Size), new_size);
                }

                FlushViewIfNeeded(view_offset, bytes);

                _InterlockedDecrement64(cast(volatile s64 *, &m_IOOpsCount));
            });
        }
    }
}

void AsyncFile::Flush()
{
    REV_SCOPED_LOCK(m_CriticalSection);

    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_CHECK_M(m_Flags & FILE_FLAG_WRITE, "You have no rights to flush file \"%.*s\". You must have write rights", m_Name.Length(), m_Name.Data());

    if (m_Flags & FILE_FLAG_FLUSH)
    {
        REV_WARNING_M("There is no sense of using Flush function with FILE_FLAG_FLUSH set, file: %.*s", m_Name.Length(), m_Name.Data());
    }
    else
    {
        Wait();

        // @NOTE(Roman): Flush data
        REV_DEBUG_RESULT(FlushViewOfFile(cast(byte *, m_View) + m_ViewStart, m_ViewEnd - m_ViewStart));

        // @NOTE(Roman): Flush metadata (+ all the buffers?)
        // REV_DEBUG_RESULT(FlushFileBuffers(m_Handle));
    }
}

void AsyncFile::Copy(const ConstString& dest_filename, bool copy_if_exists) const
{
    REV_SCOPED_LOCK(m_CriticalSection);

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
}

void AsyncFile::Move(const ConstString& dest_filename, bool move_if_exists)
{
    REV_SCOPED_LOCK(m_CriticalSection);

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
}

void AsyncFile::Delete()
{
    if (Close() && !DeleteFileA(m_Name.Data()))
    {
        REV_WARNING_MS("File \"%.*s\" can not be deleted", m_Name.Length(), m_Name.Data());
    }
}

void AsyncFile::GetTimings(u64& creation_time, u64& last_access_time, u64& last_write_time) const
{
    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());
    REV_DEBUG_RESULT(GetFileTime(m_Handle,
                                 cast(FILETIME *, &creation_time),
                                 cast(FILETIME *, &last_access_time),
                                 cast(FILETIME *, &last_write_time)));
}

u64 AsyncFile::CreationTime() const
{
    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME creation_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, &creation_time, null, null));

    return *cast(u64 *, &creation_time);
}

u64 AsyncFile::LastAccessTime() const
{
    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME last_access_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, null, &last_access_time, null));

    return *cast(u64 *, &last_access_time);
}

u64 AsyncFile::LastWriteTime() const
{
    REV_CHECK_M(m_Handle != INVALID_HANDLE_VALUE, "File \"%.*s\" is closed", m_Name.Length(), m_Name.Data());

    FILETIME last_write_time = {0};
    REV_DEBUG_RESULT(GetFileTime(m_Handle, null, null, &last_write_time));

    return *cast(u64 *, &last_write_time);
}

AsyncFile& AsyncFile::operator=(nullptr_t)
{
    Close();
    return *this;
}

AsyncFile& AsyncFile::operator=(const AsyncFile& other)
{
    if (this != &other)
    {
        REV_SCOPED_LOCK(m_CriticalSection);
        REV_SCOPED_LOCK(other.m_CriticalSection);

        Close();
        other.Wait();

        HANDLE current_process = GetCurrentProcess();
        REV_DEBUG_RESULT(DuplicateHandle(current_process, other.m_Handle, current_process, &m_Handle, 0, false, DUPLICATE_SAME_ACCESS));

        m_Size        = other.m_Size;
        m_Mapping     = other.m_Mapping;
        m_MappingSize = other.m_MappingSize;
        m_View        = null;
        m_ViewStart   = 0;
        m_ViewEnd     = 0;
        m_Flags       = other.m_Flags;
        m_ChunkSize   = other.m_ChunkSize;
        m_IOOpsCount  = 0;
        m_Name        = other.m_Name;
    }
    return *this;
}

AsyncFile& AsyncFile::operator=(AsyncFile&& other)
{
    if (this != &other)
    {
        REV_SCOPED_LOCK(m_CriticalSection);
        Close();

        REV_SCOPED_LOCK(other.m_CriticalSection);

        m_Handle      = other.m_Handle;
        m_Size        = other.m_Size;
        m_Mapping     = other.m_Mapping;
        m_MappingSize = other.m_MappingSize;
        m_View        = other.m_View;
        m_ViewStart   = other.m_ViewStart;
        m_ViewEnd     = other.m_ViewEnd;
        m_Flags       = other.m_Flags;
        m_ChunkSize   = other.m_ChunkSize;
        m_Name        = RTTI::move(other.m_Name);

        other.m_Handle = INVALID_HANDLE_VALUE;
    }
    return *this;
}

void AsyncFile::Open()
{
    REV_SCOPED_LOCK(m_CriticalSection);

    u32 desired_access = 0;
    u32 shared_access  = 0;
    u32 disposition    = 0;
    u32 attributes     = 0;
    SplitFlagsToWin32Flags(desired_access, shared_access, disposition, attributes);

    CreateHandle(desired_access, shared_access, disposition, attributes);
}

void AsyncFile::CreateHandle(u32 desired_access, u32 shared_access, u32 disposition, u32 attributes)
{
    REV_SCOPED_LOCK(m_CriticalSection);

    m_Handle = CreateFileA(m_Name.Data(), desired_access, shared_access, null, disposition, attributes, null);
    if (m_Handle == INVALID_HANDLE_VALUE)
    {
        u32 error = GetSysErrorCode();
        switch (error)
        {
            case ERROR_FILE_NOT_FOUND:
            {
                REV_CHECK(m_Flags & FILE_FLAG_EXISTS);

                Path path(m_Name.SubString(0, m_Name.RFind('/', 1)));
                path.Inspect();
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

void AsyncFile::CreateMapping(u64 wanted_mapping_size)
{
    REV_SCOPED_LOCK(m_CriticalSection);

    if (m_Handle != INVALID_HANDLE_VALUE && wanted_mapping_size)
    {
        m_MappingSize = AlignUp(wanted_mapping_size, m_PageSize);

        u32 page_access = PAGE_READONLY;
        if (m_Flags & FILE_FLAG_WRITE) page_access = PAGE_READWRITE;

        m_Mapping = CreateFileMappingA(m_Handle, null, page_access, m_MappingSize >> 32, m_MappingSize & REV_U32_MAX, m_Name.Data());
        if (!m_Mapping)
        {
            REV_ERROR_M("Mapping to file \"%.*s\" can not be created", m_Name.Length(), m_Name.Data());
        }
        else if (GetSysErrorCode() == ERROR_ALREADY_EXISTS)
        {
            void *view = MapViewOfFile(m_Mapping, FILE_MAP_READ, 0, 0, 0);

            MEMORY_BASIC_INFORMATION memory_basic_info = {};
            VirtualQueryEx(GetCurrentProcess(), view, &memory_basic_info, sizeof(MEMORY_BASIC_INFORMATION));

            if (m_MappingSize > memory_basic_info.RegionSize)
            {
                REV_DEBUG_RESULT(UnmapViewOfFile(view));
                REV_DEBUG_RESULT(CloseHandle(m_Mapping));

                m_Mapping = CreateFileMappingA(m_Handle, null, page_access, m_MappingSize >> 32, m_MappingSize & REV_U32_MAX, m_Name.Data());
                REV_CHECK(m_Mapping);
            }
            else if (m_MappingSize < memory_basic_info.RegionSize)
            {
                // @Important(Roman): Mapping already exists. Returned a handle to already created mapping with current size, not specified.
                REV_WARNING_M("Mapping to file \"%.*s\" already exists. Existed mapping size: %I64u, required mapping size: %I64u",
                            m_Name.Length(), m_Name.Data(),
                            memory_basic_info.RegionSize,
                            m_MappingSize);

                m_MappingSize = memory_basic_info.RegionSize;

                REV_DEBUG_RESULT(UnmapViewOfFile(view));
            }
            else
            {
                // @NOTE(Roman): Does nothing, existed mapping's size is equals to required mapping size, so we're good with this one.
                REV_DEBUG_RESULT(UnmapViewOfFile(view));
            }
        }
    }
}

REV_INTERNAL u64 AdjustStart(u64 old_start, u64 new_start, u64 page_size)
{
    if (old_start             > new_start) return new_start;
    if (old_start + page_size < new_start) return new_start;
    return old_start;
}

REV_INTERNAL u64 AdjustEnd(u64 old_end, u64 new_end, u64 page_size)
{
    if (old_end             < new_end) return new_end;
    if (old_end - page_size > new_end) return new_end;
    return old_end;
}

void AsyncFile::UpdateMappingIfNeeded(u64 from, u64 to)
{
    REV_SCOPED_LOCK(m_CriticalSection);

    if (m_ViewStart <= from && to <= m_ViewEnd) return;

    u64 map_start = AlignDown(from, m_PageSize);
    u64 map_end   = AlignUp(to, m_PageSize);

    if (m_View)
    {
        map_start = AdjustStart(m_ViewStart, map_start, m_PageSize);
        map_end   = AdjustEnd(m_ViewEnd, map_end, m_PageSize);

        // @NOTE(Roman): Wait while the view is in use.
        Wait();

        REV_DEBUG_RESULT(UnmapViewOfFile(m_View));
        m_View = null;
    }

    // @NOTE(Roman): Prefetch x2: https://devblogs.microsoft.com/oldnewthing/20120120-00/?p=8493
    if (m_Flags & FILE_FLAG_SEQ) map_end += map_end - map_start;

    if (m_MappingSize < map_end)
    {
        if (m_Mapping)
        {
            REV_DEBUG_RESULT(CloseHandle(m_Mapping));
            m_Mapping = null;
        }
        CreateMapping(map_end);
    }

    u32 map_flags = FILE_MAP_READ;
    if (m_Flags & FILE_FLAG_WRITE) map_flags |= FILE_MAP_WRITE;

    m_View = MapViewOfFile(m_Mapping, map_flags, map_start >> 32, map_start & REV_U32_MAX, map_end - map_start);
    REV_CHECK_M(m_View, "View of file \"%.*s\" can not be mapped", m_Name.Length(), m_Name.Data());

    m_ViewStart = map_start;
    m_ViewEnd   = map_end;
}

void AsyncFile::SplitFlagsToWin32Flags(u32& desired_access, u32& shared_access, u32& disposition, u32& attributes)
{
    FILE_FLAG flags = m_Flags;

    if (flags & FILE_FLAG_READ)
    {
        desired_access  = GENERIC_READ;
        shared_access   = FILE_SHARE_READ;
        attributes     |= FILE_ATTRIBUTE_READONLY;
    }
    if (flags & FILE_FLAG_WRITE)
    {
        desired_access |=  GENERIC_WRITE;
        shared_access  |=  FILE_SHARE_WRITE;
        attributes     &= ~FILE_ATTRIBUTE_READONLY;
        attributes     |=  FILE_ATTRIBUTE_NORMAL;
    }
    REV_DEBUG_RESULT_M(flags & FILE_FLAG_RW, "A file must have FLAG_READ and/or FILE_WRITE flags");

    // @NOTE(Roman): CREATE_NEW        - if (exists) fails ,    ERROR_FILE_EXISTS    else creates
    //               CREATE_ALWAYS     - if (exists) truncates, ERROR_ALREADY_EXISTS else creates
    //               OPEN_EXISTING     - if (exists) succeeds                        else fails,  ERROR_FILE_NOT_FOUND
    //               OPEN_ALWAYS       - if (exists) succeeds,  ERROR_ALREADY_EXISTS else creates
    //               TRUNCATE_EXISTING - if (exists) truncates                       else fails,  ERROR_FILE_NOT_FOUND. Must have GENERIC_WRITE.
    if (flags & FILE_FLAG_EXISTS)
    {
        if (flags & FILE_FLAG_TRUNCATE)
        {
            REV_DEBUG_RESULT_M(flags & FILE_FLAG_WRITE, "For truncating an existing file, you have to open it with the write access");
            disposition = TRUNCATE_EXISTING;
        }
        else
        {
            disposition = OPEN_EXISTING;
        }
    }
    else if (flags & FILE_FLAG_NEW)
    {
        disposition = CREATE_NEW;
    }
    else
    {
        if (flags & FILE_FLAG_TRUNCATE) disposition = CREATE_ALWAYS;
        else                              disposition = OPEN_ALWAYS;
    }

    /**/ if (flags & FILE_FLAG_RAND) attributes |= FILE_FLAG_RANDOM_ACCESS;
    else if (flags & FILE_FLAG_SEQ)  attributes |= FILE_FLAG_SEQUENTIAL_SCAN;

    if (flags & FILE_FLAG_TEMP)
    {
        shared_access |= FILE_SHARE_DELETE;
        attributes    |= FILE_ATTRIBUTE_TEMPORARY
                      |  FILE_FLAG_DELETE_ON_CLOSE;
    }
    if (flags & FILE_FLAG_FLUSH)
    {
        attributes |= FILE_FLAG_WRITE_THROUGH;
    }
}

void AsyncFile::FlushViewIfNeeded(u64 from, u64 to)
{
    REV_SCOPED_LOCK(m_CriticalSection);

    if (m_Flags & FILE_FLAG_FLUSH)
    {
        // @NOTE(Roman): Flush data
        REV_DEBUG_RESULT(FlushViewOfFile(cast(byte *, m_View) + from, to - from));

        // @NOTE(Roman): Flush metadata (+ all the buffers?)
        // REV_DEBUG_RESULT(FlushFileBuffers(m_Handle));
    }
}

}
