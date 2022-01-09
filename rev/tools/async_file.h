// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/file.h"
#include "core/work_queue.h"

namespace REV
{
    // @TODO(Roman): 1. We want to read 0-10 symbols and write 5-12 symbols.
    //                  So we need to wait for writing 5-10 symbols and then read?
    //                  Or wait for read first and then write?
    //               2. We want to read/write first and forth page, so we read/write first page first
    //                  and then we need to remap our view to read/write forth one.
    //                  What about having several views to support better asynchrony?
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
        bool Close();

        void Clear();

        void Read(void *buffer, u64 bytes, u64 read_offset) const;
        void Write(const void *buffer, u64 bytes, u64 write_offset);

        REV_INLINE void Wait() const { while (m_IOOpsCount); }

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
        void Open();
        void CreateHandle(u32 desired_access, u32 shared_access, u32 disposition, u32 attributes);
        void CreateMapping(u64 wanted_mapping_size);
        void UpdateMappingIfNeeded(u64 from, u64 to);
        void SplitFlagsToWin32Flags(u32& desired_access, u32& shared_access, u32& disposition, u32& attributes);
        void FlushViewIfNeeded(u64 from, u64 to);

    private:
        HANDLE                          m_Handle;
        volatile u64                    m_Size;

        HANDLE                          m_Mapping;
        u64                             m_MappingSize;

        void                           *m_View;
        u64                             m_ViewStart; // View start file offset
        u64                             m_ViewEnd;   // View end file offset

        FILE_FLAG                       m_Flags;
        const u32                       m_PageSize;
        u64                             m_ChunkSize;
        volatile u64                    m_IOOpsCount;

        mutable CriticalSection<false>  m_CriticalSection;

        StaticString<REV_PATH_CAPACITY> m_Name;
    };
}
