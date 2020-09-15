//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

enum class FILE_FLAG
{
    NONE  = 0,

    READ  = BIT(0),
    WRITE = BIT(1),
};
ENUM_CLASS_OPERATORS(FILE_FLAG);

// @Issue(Roman): Several simultaneous read operations under the same file?

class ENGINE_IMPEXP AsyncFile final
{
public:
    // @NOTE(Roman): It would be better if you pass non-null filename_len,
    //               otherwise strlen will be used.
    AsyncFile(in const char *filename, in_opt u64 filename_len, in FILE_FLAG flags);
    AsyncFile(in const AsyncFile& other);
    AsyncFile(in AsyncFile&& other) noexcept;

    ~AsyncFile();

    void Clear();

    void Read(out void *buffer, in u32 buffer_bytes) const;
    void Write(in const void *buffer, in u32 buffer_bytes);
    void Append(in const void *buffer, in u32 buffer_bytes);

    void Wait() const;

    void SetOffset(in u32 offset);

    u32         GetOffset() const { return m_Offset; }
    u32         GetSize()   const { return m_Size;   }
    const char *GetName()   const { return m_Name;   }

    AsyncFile& operator=(in const AsyncFile& other);
    AsyncFile& operator=(in AsyncFile&& other) noexcept;

private:
    friend void WINAPI OverlappedReadCompletionRoutine(
        in     u32         error_code,
        in     u32         number_of_bytes_transfered,
        in_out OVERLAPPED *overlapped
    );

    friend void WINAPI OverlappedWriteCompletionRoutine(
        in     u32         error_code,
        in     u32         number_of_bytes_transfered,
        in_out OVERLAPPED *overlapped
    );

private:
    HANDLE             m_Handle;
    FILE_FLAG          m_Flags;
    u32                m_Offset;
    u32                m_Size;
    mutable OVERLAPPED m_Overlapped;
    char               m_Name[_MAX_PATH];
};
