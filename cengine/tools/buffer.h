//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/allocator.h"

// @NOTE(Roman): annotation for buffers
#define BUF

typedef struct BufferHeader
{
    Allocator *allocator;
    u64        count;
    u64        cap_in_bytes;
    u64        alignment_in_bytes;
    byte       data[0];
} BufferHeader;

CENGINE_FUN void *CreateBuffer(Allocator *allocator, u64 alignment_in_bytes);
CENGINE_FUN void  DestroyBuffer(void *buffer);

CENGINE_FUN void *ExpandBuffer(BufferHeader *header, u64 element_bytes);

#define BufferGetHeader(buffer) cast(BufferHeader *, cast(byte *, buffer) - sizeof(BufferHeader))
#define BufferGetCount(buffer)  ((buffer) ? BufferGetHeader(buffer)->count : 0)

#define BufferInsert(buffer, element, where)                             \
{                                                                        \
    Check(buffer);                                                       \
    BufferHeader *header = BufferGetHeader(buffer);                      \
    ++header->count;                                                     \
    (buffer) = ExpandBuffer(header, sizeof(element));                    \
    BufferHeader *new_header = BufferGetHeader(buffer);                  \
    if ((where) < new_header->count - 1)                                 \
    {                                                                    \
        MoveMemory((buffer) + (where) + 1,                               \
                   (buffer) + (where),                                   \
                   (new_header->count - 1 - (where)) * sizeof(element)); \
    }                                                                    \
    (buffer)[where] = (element);                                         \
}

#define BufferPushBack(buffer, element)  { u64 where = BufferGetCount(buffer); BufferInsert(buffer, element, where); }
#define BufferPushFront(buffer, element) BufferInsert(buffer, element, 0)

#define BufferErase(buffer, where)                                     \
{                                                                      \
    Check(buffer);                                                     \
    BufferHeader *header = BufferGetHeader(buffer);                    \
    if (header->count)                                                 \
    {                                                                  \
        --header->count;                                               \
        if ((where) < header->count)                                   \
        {                                                              \
            MoveMemory((buffer) + (where),                             \
                       (buffer) + (where) + 1,                         \
                       (header->count - (where)) * sizeof(*(buffer))); \
        }                                                              \
        ZeroMemory((buffer) + header->count, sizeof(*(buffer)));       \
    }                                                                  \
}

#define BufferEraseBack(buffer)  { u64 where = BufferGetCount(buffer); BufferErase(buffer, where); }
#define BufferEraseFront(buffer) BufferErase(buffer, 0)

#define BufferClear(buffer)                         \
{                                                   \
    Check(buffer);                                  \
    BufferHeader *header = BufferGetHeader(buffer); \
    ZeroMemory(buffer, header->count);              \
    header->count = 0;                              \
}
