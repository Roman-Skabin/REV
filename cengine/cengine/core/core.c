//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/core.h"
#include "cengine.h"

//
// Debuging
//

void __cdecl DebugF(const char *const format, ...)
{
    va_list args;
    va_start(args, format);

    if (args)
    {
        char buffer[BUFSIZ] = {'\0'};
        int len             = vsprintf(buffer, format, args);
        buffer[len - 1]     = '\n';

        OutputDebugStringA(buffer);
    }
    else
    {
        OutputDebugStringA(format);
        OutputDebugStringA("\n");
    }

    va_end(args);
}

//
// General Purpose Allocator
//

void *Allocate(size_t bytes)
{
    CheckM(bytes, "0 bytes was passed");
    void *mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
    CheckM(mem, "Heap Overflow");
    return mem;
}

void *ReAllocate(void *mem, size_t bytes)
{
    if (!mem)
    {
        return Allocate(bytes);
    }

    if (!bytes)
    {
        DeAllocate(&mem);
        return 0;
    }

    mem = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, bytes);
    CheckM(mem, "Heap Overflow");

    return mem;
}

void DeAllocate(void **mem)
{
    if (mem)
    {
        HeapFree(GetProcessHeap(), 0, *mem);
        *mem = 0;
    }
}

void *AllocateAligned(size_t bytes, size_t alignment)
{
    CheckM(bytes, "0 bytes was passed");
    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");

    bytes = ALIGN_UP(bytes, alignment);

    void *mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
    CheckM(mem, "Heap Overflow");

    return mem;
}

void *ReAllocateAligned(void *mem, size_t bytes, size_t alignment)
{
    if (!mem)
    {
        return AllocateAligned(bytes, alignment);
    }

    if (!bytes)
    {
        DeAllocateAligned(&mem);
        return 0;
    }

    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");

    bytes = ALIGN_UP(bytes, alignment);

    mem = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, bytes);
    CheckM(mem, "Heap Overflow");

    return mem;
}

void DeAllocateAligned(void **mem)
{
    if (mem)
    {
        HeapFree(GetProcessHeap(), 0, *mem);
        *mem = 0;
    }
}

//
// Stretchy Buffers
//

void buf_dealloc(BUF void *b)
{
    HeapFree(GetProcessHeap(), 0, _BUFHDR(b));
}

BUF void *buf_grow(BUF void *b, u32 new_count, size_t el_size)
{
    if (new_count > buf_cap(b))
    {
        CheckM(buf_cap(b) <= (U32_MAX - sizeof(BufHdr)) / el_size / 2, "Buffer overflow");

        u32 new_cap = __max(sizeof(BufHdr), __max(2 * buf_cap(b), new_count));
        Check(new_cap <= (U32_MAX - sizeof(BufHdr)) / el_size);

        size_t new_size = sizeof(BufHdr) + new_cap * el_size;

        BufHdr *new_buf;
        if (b)
        {
            new_buf = (BufHdr *)ReAllocateAligned(_BUFHDR(b), new_size, 8);
        }
        else
        {
            new_buf = (BufHdr *)AllocateAligned(new_size, 8);
            new_buf->count = 0;
        }
        new_buf->cap = new_cap;

        return new_buf->buf;
    }

    return b;
}
