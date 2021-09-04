//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "tools/static_string.hpp"
#include "tools/const_array.hpp"
#include "tools/function.hpp"
#include "tools/critical_section.hpp"

namespace REV
{
    // @TODO(Roman): 1. if ERROR_PATH_NOT_FOUND
    //               2. #CrossPlatform
    class REV_API File final
    {
    public:
        enum FLAG : u32
        {
            FLAG_NONE     = 0,

            // General flags (required)
            FLAG_READ     = 0x1,
            FLAG_WRITE    = 0x2,

            // Open flags (optional)
            FLAG_EXISTS   = 0x10, // A file will be opened only if it exists.   (There is no sense to combine it with FILE_FLAG_NEW)
            FLAG_NEW      = 0x20, // Creates a file only if it does not exists. (There is no sense to open a file with this flag, but without FILE_FLAG_WRITE).
            FLAG_TRUNCATE = 0x40, // Truncate a file on open. Must have FILE_FLAG_WRITE.

            // Other flags (optional)
            FLAG_RAND     = 0x100, // Optimized for a random access. There is no sence to combine it with FILE_FLAG_SEQ.
            FLAG_SEQ      = 0x200, // Optimized for a sequential access. There is no sence to combine it with FILE_FLAG_RAND.
            FLAG_TEMP     = 0x400, // File will be deleted on close.
            FLAG_FLUSH    = 0x800, // Flush data immediatly after write.

            // Default combinations
            FLAG_RW       = FLAG_READ | FLAG_WRITE,
            FLAG_RES      = FLAG_READ | FLAG_EXISTS | FLAG_SEQ,
        };

        enum FIND_RESULT : u32
        {
            FIND_RESULT_FOUND = 1,
            FIND_RESULT_CONTINUE,
            FIND_RESULT_BREAK,
        };

    public:
        REV_NOINLINE File(nullptr_t = null);
        REV_NOINLINE File(const ConstString& filename, FLAG flags);
        REV_INLINE   File(const StaticString<REV_PATH_CAPACITY>& filename, FLAG flags) : File(filename.ToConstString(), flags) {}
        REV_NOINLINE File(const File& other);
        REV_NOINLINE File(File&& other);

        ~File();

        REV_NOINLINE bool Open(const ConstString& filename, FLAG flags);
        REV_INLINE   bool Open(const StaticString<REV_PATH_CAPACITY>& filename, FLAG flags) { return Open(filename.ToConstString(), flags); }
        REV_NOINLINE void ReOpen(FLAG new_flags);
        REV_NOINLINE void Close();

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

        static bool Exists(const StaticString<REV_PATH_CAPACITY>& filename);
        static bool Exists(const ConstString& filename);

        void GetTimings(u64& creation_time, u64& last_access_time, u64& last_write_time) const;
        u64  CreationTime() const;
        u64  LastAccessTime() const;
        u64  LastWriteTime() const;

        static void Find(const StaticString<REV_PATH_CAPACITY>& filename, const Function<FIND_RESULT(const ConstString& found_filename, bool file_not_found)>& FindFileCallback);

        void SetOffset(s64 offset);

        void GoToEOF();
        REV_INLINE bool IsEOF()  const { return m_Offset == m_Size;               }
        REV_INLINE bool Empty()  const { return !m_Size;                          }
        REV_INLINE bool Opened() const { return m_Handle != INVALID_HANDLE_VALUE; }

        REV_INLINE s64  Offset() const { return m_Offset; }
        REV_INLINE u64  Size()   const { return m_Size;   }
        REV_INLINE FLAG Flags()  const { return m_Flags;  }

        REV_INLINE const StaticString<REV_PATH_CAPACITY>& Name() const { return m_Name; }
        REV_INLINE       StaticString<REV_PATH_CAPACITY>& Name()       { return m_Name; }

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
        FLAG                            m_Flags;
        StaticString<REV_PATH_CAPACITY> m_Name;
    };
    REV_ENUM_OPERATORS(File::FLAG)

    struct REV_API Path final
    {
        ConstString path           = null;
        ConstString exists         = null;
        ConstString does_not_exist = null;

        REV_INLINE Path(const ConstString& path)                     : path(path) { REV_CHECK_M(path.Length() < REV_PATH_CAPACITY, "Path is to long, max available length is: %I32u", REV_PATH_CAPACITY); }
        REV_INLINE Path(const StaticString<REV_PATH_CAPACITY>& path) : path(path.ToConstString()) {}
        REV_INLINE Path(const Path& other)                           : path(other.path),             exists(other.exists),             does_not_exist(other.does_not_exist)             {}
        REV_INLINE Path(Path&& other)                                : path(RTTI::move(other.path)), exists(RTTI::move(other.exists)), does_not_exist(RTTI::move(other.does_not_exist)) {}

        REV_INLINE ~Path() {}

        bool  Exists();
        Path& Inspect();
        Path& Create();

        template<typename ...T>
        Path& PrintWarningIfDoesNotExist(const T... args)
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
        static bool Exists(const StaticString<REV_PATH_CAPACITY>& path);

    private:
        void _PrintWarningIfDoesNotExist(const ConstString& warning_message = null);
    };
}
