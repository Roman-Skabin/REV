//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/buffer.h"

#pragma warning(push)
#pragma warning(disable: 4133)

void *CreateBuffer(Allocator *allocator, u64 alignment_in_bytes)
{
    Check(allocator);

    if (alignment_in_bytes < sizeof(void *)) alignment_in_bytes = sizeof(void *);
    CheckM(alignment_in_bytes, "alignment must be power of two");

    BufferHeader *header       = Allocate(allocator, sizeof(BufferHeader));
    header->allocator          = allocator;
    header->alignment_in_bytes = alignment_in_bytes;

    return header->data;
}

void DestroyBuffer(void *buffer)
{
    if (buffer)
    {
        BufferHeader *header = cast(byte *, buffer) - sizeof(BufferHeader);
        DeAlloc(header->allocator, header);
    }
}

void *ExpandBuffer(BufferHeader *header, u64 element_bytes)
{
    if (!header->cap_in_bytes)
    {
        header->cap_in_bytes = ALIGN_UP(header->count * element_bytes, header->alignment_in_bytes);
        header               = ReAllocate(header->allocator, &header, sizeof(BufferHeader) + header->cap_in_bytes);
    }
    else if (header->count * element_bytes > header->cap_in_bytes)
    {
        header->cap_in_bytes = 2 * header->cap_in_bytes;
        header->cap_in_bytes = ALIGN_UP(header->cap_in_bytes, header->alignment_in_bytes);
        header               = ReAllocate(header->allocator, &header, sizeof(BufferHeader) + header->cap_in_bytes);
    }
    return header->data;
}

#pragma warning(pop)
