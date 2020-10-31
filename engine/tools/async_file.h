//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "tools/static_string.hpp"

// @Issue(Roman): Several simultaneous read operations under the same file?

class ENGINE_API AsyncFile final
{
public:
    enum class FLAGS
    {
        NONE  = 0,
        READ  = BIT(0),
        WRITE = BIT(1),

        // @NOTE(Roman): Internal
        _WWEC = BIT(31),
    };

public:
    AsyncFile(const StaticString<MAX_PATH>& filename, FLAGS flags);
    AsyncFile(const AsyncFile& other);
    AsyncFile(AsyncFile&& other) noexcept;

    ~AsyncFile();

    void Clear();

    void Move(const StaticString<MAX_PATH>& to_filename);
    void Copy(const StaticString<MAX_PATH>& to_filename) const;
    void Copy(const StaticString<MAX_PATH>& to_filename, AsyncFile& to_file, FLAGS to_flags = FLAGS::NONE) const;

    void Read(void *buffer, u32 buffer_bytes) const;
    void Write(const void *buffer, u32 buffer_bytes);
    void Append(const void *buffer, u32 buffer_bytes);

    void Wait() const;

    template<typename ...AsyncFiles, typename = RTTI::enable_if_t<RTTI::are_same_v<AsyncFile, AsyncFiles...>>>
    static void WaitForAll(AsyncFiles& ...async_files)
    {
        HANDLE m_Events[sizeof...(async_files)] = {null};

        u32 events_count = 0;
        (..., ((async_files.m_Flags & AsyncFile::FLAGS::_WWEC) == AsyncFile::FLAGS::NONE ? m_Events[events_count++] = async_files.m_Overlapped.hEvent : 0));

        u32 res = WAIT_FAILED;
        do
        {
            res = WaitForMultipleObjectsEx(events_count, m_Events, true, INFINITE, true);
        } while (res != WAIT_OBJECT_0 && res != WAIT_IO_COMPLETION);

        (..., (async_files.m_Flags |= FLAGS::_WWEC));
    }

    void SetOffset(u32 offset);

    constexpr       u32                     Offset() const { return m_Offset; }
    constexpr       u32                     Size()   const { return m_Size;   }
    constexpr const StaticString<MAX_PATH>& Name()   const { return m_Name;   }

    AsyncFile& operator=(const AsyncFile& other);
    AsyncFile& operator=(AsyncFile&& other) noexcept;

private:
    friend void WINAPI OverlappedReadCompletionRoutine(
        u32         error_code,
        u32         bytes_transfered,
        OVERLAPPED *overlapped
    );

    friend void WINAPI OverlappedWriteCompletionRoutine(
        u32         error_code,
        u32         bytes_transfered,
        OVERLAPPED *overlapped
    );

private:
    HANDLE                 m_Handle;
    FLAGS                  m_Flags;
    u32                    m_Offset;
    u32                    m_Size;
    mutable OVERLAPPED     m_Overlapped;
    StaticString<MAX_PATH> m_Name;
};

ENUM_CLASS_OPERATORS(AsyncFile::FLAGS);
