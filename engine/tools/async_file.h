//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "tools/static_string.hpp"

// @Issue(Roman): Several simultaneous read operations under the same file?

// @TODO(Roman): Make an AsyncFileGroup (or kinda, maybe better or another name)
//               that contains several files that will be read or written.
//               The reason is to be able to wait for several files simultaneously,
//               not sequentially.

// @TODO(Roman): Replace StaticString with const char *?

class ENGINE_IMPEXP AsyncFile final
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

    void Read(void *buffer, u32 buffer_bytes) const;
    void Write(const void *buffer, u32 buffer_bytes);
    void Append(const void *buffer, u32 buffer_bytes);

    void Wait() const;

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
