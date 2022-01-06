// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/static_string.hpp"
#include "tools/critical_section.hpp"
#include "tools/function.hpp"

namespace REV
{
    enum FILE_FLAG : u32
    {
        FILE_FLAG_NONE     = 0,

        // General flags (required)
        FILE_FLAG_READ     = 0x1,
        FILE_FLAG_WRITE    = 0x2,

        // Open flags (optional)
        FILE_FLAG_EXISTS   = 0x10, // A file will be opened only if it exists.   (There is no sense to combine it with FILE_FLAG_NEW)
        FILE_FLAG_NEW      = 0x20, // Creates a file only if it does not exists. (There is no sense to open a file with this flag, but without FILE_FLAG_WRITE).
        FILE_FLAG_TRUNCATE = 0x40, // Truncate a file on open. Must have FILE_FLAG_WRITE.

        // Other flags (optional)
        FILE_FLAG_RAND     = 0x100, // Optimized for a random access (prefetch 2 times less data than without). There is no sence to combine it with FILE_FLAG_SEQ.
        FILE_FLAG_SEQ      = 0x200, // Optimized for a sequential access (prefetch 2 times more data than without). There is no sence to combine it with FILE_FLAG_RAND.
        FILE_FLAG_TEMP     = 0x400, // File will be deleted on close.
        FILE_FLAG_FLUSH    = 0x800, // Flush data immediatly after write.

        // Default combinations
        FILE_FLAG_RW       = FILE_FLAG_READ | FILE_FLAG_WRITE,
        FILE_FLAG_RES      = FILE_FLAG_READ | FILE_FLAG_EXISTS | FILE_FLAG_SEQ,
    };
    REV_ENUM_OPERATORS(FILE_FLAG)

    // @NOTE(Roman): Must return true if you want to break find loop. For example if the file you were searching for has been found.
    #define REV_FIND_FILE_CALLBACK(name) bool name(const ConstString& found_filename)
    typedef REV_FIND_FILE_CALLBACK(FindFileCallback);

    // @TODO(Roman): Rewrite with file views and WorkQueue
    class REV_API File final
    {
    public:
        File(nullptr_t = null);
        File(const ConstString& filename, FILE_FLAG flags);
        template<u64 capacity> REV_INLINE File(const StaticString<capacity>& filename, FILE_FLAG flags) : File(filename.ToConstString(), flags) {}
        File(const File& other);
        File(File&& other);

        ~File();

        bool Open(const ConstString& filename, FILE_FLAG flags);
        template<u64 capacity> REV_INLINE bool Open(const StaticString<capacity>& filename, FILE_FLAG flags) { return Open(filename.ToConstString(), flags); }
        void ReOpen(FILE_FLAG new_flags);
        void Close();

        void Clear();

        void Read(void *buffer, u64 bytes);
        void Write(const void *buffer, u64 bytes);
        void Append(const void *buffer, u64 bytes);

        // @NOTE(Roman): Has no effect if FILE_FLAG_FLUSH set.
        void Flush();

        REV_NOINLINE void Copy(const ConstString& dest_filename, bool copy_if_exists = true) const;
        REV_NOINLINE void Move(const ConstString& dest_filename, bool move_if_exists = true);
        REV_INLINE   void Rename(const ConstString& new_filename) { Move(new_filename); }
        REV_NOINLINE void Delete();

        // @NOTE(Roman): These are not thread-safety versions
        static            void Copy(const ConstString& src_filename, const ConstString& dest_filename, bool copy_if_exists = true);
        static            void Move(const ConstString& src_filename, const ConstString& dest_filename, bool move_if_exists = true);
        static REV_INLINE void Rename(const ConstString& filename, const ConstString& new_filename) { Move(filename, new_filename, true); }
        static            void Delete(const ConstString& filename);

        static bool Exists(const ConstString& filename);
        template<u64 capacity> static REV_INLINE bool Exists(const StaticString<capacity>& filename) { return Exists(filename.ToConstString()); }

        void GetTimings(u64& creation_time, u64& last_access_time, u64& last_write_time) const;
        u64  CreationTime() const;
        u64  LastAccessTime() const;
        u64  LastWriteTime() const;

        // @NOTE(Roman): Returns false if there are no files matching following wildcard. Otherwise returns true.
        static bool Find(const ConstString& filename_wildcard, const Function<FindFileCallback>& Callback, bool case_sensitive_search = false);
        template<u64 capacity> static REV_INLINE bool Find(const StaticString<capacity>& filename_wildcard, const Function<FindFileCallback>& Callback, bool case_sensitive_search = false) { return Find(filename_wildcard.ToConstString(), Callback, case_sensitive_search); }

        void SetOffset(s64 offset);

        void GoToEOF();
        REV_INLINE bool IsEOF()  const { return m_Offset == m_Size;               }
        REV_INLINE bool Empty()  const { return !m_Size;                          }
        REV_INLINE bool Opened() const { return m_Handle != INVALID_HANDLE_VALUE; }
        REV_INLINE bool Closed() const { return m_Handle == INVALID_HANDLE_VALUE; }

        REV_INLINE s64       Offset() const { return m_Offset; }
        REV_INLINE u64       Size()   const { return m_Size;   }
        REV_INLINE FILE_FLAG Flags()  const { return m_Flags;  }

        REV_INLINE const StaticString<REV_PATH_CAPACITY>& Name() const { return m_Name; }

        File& operator=(nullptr_t);
        File& operator=(const File& other);
        File& operator=(File&& other);

    private:
        void Open();
        void SplitFlagsToWin32Flags(u32& desired_access, u32& shared_access, u32& disposition, u32& attributes);
        void LockSystemCacheFromOtherProcesses(u64 offset, u64 bytes, bool shared);
        void UnlockSystemCacheFromOtherProcesses(u64 offset, u64 bytes);

    private:
        HANDLE                          m_Handle;
        u64                             m_Offset;
        u64                             m_Size;
        mutable CriticalSection<true>   m_CriticalSection;
        FILE_FLAG                       m_Flags;
        StaticString<REV_PATH_CAPACITY> m_Name;
    };

    struct REV_API Path final
    {
        ConstString path           = null;
        ConstString exists         = null;
        ConstString does_not_exist = null;

        REV_INLINE Path(const ConstString& path)                     : path(path) { REV_CHECK_M(path.Length() < REV_PATH_CAPACITY, "Path is to long, max available length is: %I32u", REV_PATH_CAPACITY); }
        template<u64 capacity> REV_INLINE Path(const StaticString<capacity>& path) : Path(path.ToConstString()) {}
        REV_INLINE Path(const Path& other)                           : path(other.path),             exists(other.exists),             does_not_exist(other.does_not_exist)             {}
        REV_INLINE Path(Path&& other)                                : path(RTTI::move(other.path)), exists(RTTI::move(other.exists)), does_not_exist(RTTI::move(other.does_not_exist)) {}

        REV_INLINE ~Path() {}

        bool  Exists();
        Path& Inspect();
        Path& Create();

        template<typename ...T>
        Path& PrintWarningIfDoesNotExist(const T&... args)
        {
            if constexpr (sizeof...(args) > 0)
            {
                StaticStringBuilder<1024> builder;
                builder.Build(args...);
                _PrintWarningIfDoesNotExist(builder.ToString().ToConstString());
            }
            else
            {
                _PrintWarningIfDoesNotExist();
            }
            return *this;
        }

        static bool Exists(const ConstString& path);
        template<u64 capacity> static REV_INLINE bool Exists(const StaticString<capacity>& path) { return Exists(path.ToConstString()); }

    private:
        void _PrintWarningIfDoesNotExist(const ConstString& warning_message = null);
    };
}
