//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "tools/const_string.h"
#include "tools/static_string.hpp"

namespace REV
{
    // @Issue(Roman): Several simultaneous read operations under the same file?
    // @TODO(Roman): #CrossPlatform
    class REV_API AsyncFile final
    {
    public:
        enum FLAGS
        {
            FLAG_NONE   = 0,
            FLAG_READ   = BIT(0),
            FLAG_WRITE  = BIT(1),
            FLAG_DELETE = BIT(2),

            // @NOTE(Roman): Internal
            _FLAG_WWEC  = BIT(31),
        };

    public:
        AsyncFile(const ConstString& filename, FLAGS flags);
        AsyncFile(const StaticString<REV_PATH_CAPACITY>& filename, FLAGS flags);
        AsyncFile(const AsyncFile& other);
        AsyncFile(AsyncFile&& other) noexcept;

        ~AsyncFile();

        void Clear();

        void Move(const StaticString<REV_PATH_CAPACITY>& to_filename);
        void Copy(const StaticString<REV_PATH_CAPACITY>& to_filename) const;
        void Copy(const StaticString<REV_PATH_CAPACITY>& to_filename, AsyncFile& to_file, FLAGS to_flags = FLAG_NONE) const;
        void Rename(const StaticString<REV_PATH_CAPACITY>& to_filename);
        void Delete();

        void Read(void *buffer, u32 buffer_bytes) const;
        void Write(const void *buffer, u32 buffer_bytes);
        void Append(const void *buffer, u32 buffer_bytes);

        void Wait() const;

        template<typename ...AsyncFiles, typename = RTTI::enable_if_t<RTTI::are_same_v<AsyncFile, AsyncFiles...>>>
        static void WaitForAll(AsyncFiles& ...async_files)
        {
            HANDLE m_Events[sizeof...(async_files)] = {null};

            u32 events_count = 0;
            (..., ((async_files.m_Flags & _FLAG_WWEC) ? m_Events[events_count++] = async_files.m_Overlapped.hEvent : 0));

            u32 res = WAIT_FAILED;
            do
            {
                res = WaitForMultipleObjectsEx(events_count, m_Events, true, INFINITE, true);
            } while (res != WAIT_OBJECT_0 && res != WAIT_IO_COMPLETION);

            (..., (async_files.m_Flags |= _FLAG_WWEC));
        }

        REV_INLINE void SetOffset(u32 offset) { m_Offset = offset < m_Size ? offset : m_Size; }

        REV_INLINE       u32                              Offset() const { return m_Offset; }
        REV_INLINE       u32                              Size()   const { return m_Size;   }
        REV_INLINE const StaticString<REV_PATH_CAPACITY>& Name()   const { return m_Name;   }

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
        HANDLE                          m_Handle;
        FLAGS                           m_Flags;
        u32                             m_Offset;
        u32                             m_Size;
        mutable OVERLAPPED              m_Overlapped;
        StaticString<REV_PATH_CAPACITY> m_Name;
    };

    REV_ENUM_CLASS_OPERATORS(AsyncFile::FLAGS);
}
