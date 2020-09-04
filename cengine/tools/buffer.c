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

    if (alignment_in_bytes < CENGINE_DEFAULT_ALIGNMENT) alignment_in_bytes = CENGINE_DEFAULT_ALIGNMENT;
    CheckM(alignment_in_bytes, "alignment must be power of two");

    BufferHeader *header       = AllocateAligned(allocator, sizeof(BufferHeader), CENGINE_DEFAULT_ALIGNMENT);
    header->allocator          = allocator;
    header->alignment_in_bytes = alignment_in_bytes;

    return header->data;
}

void _DestroyBuffer(void **buffer)
{
    if (buffer && *buffer)
    {
        BufferHeader *header = cast(byte *, *buffer) - sizeof(BufferHeader);
        DeAllocA(header->allocator, header);
        *buffer = null;
    }
}

void *ExpandBuffer(BufferHeader *header, u64 element_bytes)
{
    u64 size_in_bytes = header->count * element_bytes;

    if (size_in_bytes < header->cap_in_bytes)
    {
        header->cap_in_bytes = ALIGN_UP(2 * header->cap_in_bytes, header->alignment_in_bytes);
        header               = ReAllocateAligned(header->allocator, &header, sizeof(BufferHeader) + header->cap_in_bytes, CENGINE_DEFAULT_ALIGNMENT);
    }
    else if (size_in_bytes > header->cap_in_bytes)
    {
        header->cap_in_bytes = ALIGN_UP(size_in_bytes, header->alignment_in_bytes);
        header               = ReAllocateAligned(header->allocator, &header, sizeof(BufferHeader) + header->cap_in_bytes, CENGINE_DEFAULT_ALIGNMENT);
    }
    return header->data;
}

#pragma warning(pop)
