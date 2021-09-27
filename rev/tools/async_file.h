// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/file.h"
#include "tools/tuple.hpp"

namespace REV
{
    // @TODO(Roman): #CrossPlatform
    class REV_API AsyncFile final
    {
    public:
        AsyncFile(nullptr_t = null);
        AsyncFile(const ConstString& filename, FILE_FLAG flags);
        template<u64 capacity> REV_INLINE AsyncFile(const StaticString<capacity>& filename, FILE_FLAG flags) : AsyncFile(filename.ToConstString(), flags) {}
        AsyncFile(const AsyncFile& other);
        AsyncFile(AsyncFile&& other);

        ~AsyncFile();

        bool Open(const ConstString& filename, FILE_FLAG flags);
        template<u64 capacity> REV_INLINE bool Open(const StaticString<capacity>& filename, FILE_FLAG flags) { return Open(filename.ToConstString(), flags); }
        void ReOpen(FILE_FLAG new_flags);
        void Close();

        void Clear();

        void Read(void *buffer, u64 buffer_bytes, u64 file_offset);
        void Write(const void *buffer, u64 buffer_bytes, u64 file_offset);
        void Append(const void *buffer, u64 buffer_bytes);

        void Wait(bool wait_for_all_apcs = true) const;

        template<typename ...AF, typename = RTTI::enable_if_t<RTTI::are_same_v<AsyncFile, AF...>>>
        static void WaitForAll(const AF& ...async_files)
        {
            HANDLE  events[MAX_APCS * sizeof...(async_files)] = {null};
            HANDLE *events_it = events;

            Tuple<const AF&...>(async_files...).ForEach([&events_it](u64 index, const AF& async_file)
            {
                for (u32 i = 0; i < MAX_APCS; ++i)
                {
                    APCEntry *entry = async_file.m_APCEntries + i;
                    if (entry->in_progress) *events_it++ = entry->overlapped.hEvent;
                }
            });

            u32 events_count = events_it - events;
            if (events_count)
            {
                u32 wait_result = WaitForMultipleObjectsEx(events_count, events, true, INFINITE, true);
                REV_CHECK(wait_result == WAIT_IO_COMPLETION);
            }
        }

        // @NOTE(Roman): Has no effect if FILE_FLAG_FLUSH set.
        void Flush();

        REV_NOINLINE void Copy(const ConstString& dest_filename, bool copy_if_exists = true) const;
        REV_NOINLINE void Move(const ConstString& dest_filename, bool move_if_exists = true);
        REV_INLINE   void Rename(const ConstString& new_filename) { Move(new_filename); }
        REV_NOINLINE void Delete();

        void GetTimings(u64& creation_time, u64& last_access_time, u64& last_write_time) const;
        u64  CreationTime() const;
        u64  LastAccessTime() const;
        u64  LastWriteTime() const;

        REV_INLINE bool Empty()  const { return !m_Size;                          }
        REV_INLINE bool Opened() const { return m_Handle != INVALID_HANDLE_VALUE; }
        REV_INLINE bool Closed() const { return m_Handle == INVALID_HANDLE_VALUE; }

        REV_INLINE u64       Size()  const { return m_Size;  }
        REV_INLINE FILE_FLAG Flags() const { return m_Flags; }

        REV_INLINE const StaticString<REV_PATH_CAPACITY>& Name() const { return m_Name; }

        AsyncFile& operator=(nullptr_t);
        AsyncFile& operator=(const AsyncFile& other);
        AsyncFile& operator=(AsyncFile&& other);

    private:
        void CreateAPCEntries();
        void DestroyAPCEntries();
        void Open();
        void SplitFlagsToWin32Flags(u32& desired_access, u32& shared_access, u32& disposition, u32& attributes);
        void LockSystemCacheFromOtherProcesses(u64 offset, u64 bytes, bool shared) const;
        void UnlockSystemCacheFromOtherProcesses(u64 offset, u64 bytes) const;

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
        enum : u32 { MAX_APCS = 16 };

        struct APCEntry
        {
            AsyncFile  *file        = null;
            bool        in_progress = false;
            OVERLAPPED  overlapped  = {0};
        };

        HANDLE                          m_Handle;
        u64                             m_Size;
        FILE_FLAG                       m_Flags;
        mutable CriticalSection<false>  m_CriticalSection;
        mutable APCEntry                m_APCEntries[MAX_APCS];
        StaticString<REV_PATH_CAPACITY> m_Name;
    };
}
