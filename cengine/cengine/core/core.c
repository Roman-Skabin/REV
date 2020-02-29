//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/core.h"

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

#if DEBUG
    u64 gAllocationsPerFrame   = 0;
    u64 gReAllocationsPerFrame = 0;
    u64 gDeAllocationsPerFrame = 0;
#endif

void *Allocate(u64 bytes)
{
    CheckM(bytes, "0 bytes was passed");

    void *mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
    CheckM(mem, "Heap Overflow");

#if DEBUG
    ++gAllocationsPerFrame;
#endif
    return mem;
}

void *ReAllocate(void *mem, u64 bytes)
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

#if DEBUG
    ++gReAllocationsPerFrame;
#endif
    return mem;
}

void DeAllocate(void **mem)
{
    if (mem)
    {
        HeapFree(GetProcessHeap(), 0, *mem);
        *mem = 0;
    #if DEBUG
        ++gDeAllocationsPerFrame;
    #endif
    }
}

void *AllocateAligned(u64 bytes, u64 alignment)
{
    CheckM(bytes, "0 bytes was passed");
    CheckM(IS_POW_2(alignment), "Alignment must be power of 2");

    bytes = ALIGN_UP(bytes, alignment);

    void *mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
    CheckM(mem, "Heap Overflow");

#if DEBUG
    ++gAllocationsPerFrame;
#endif
    return mem;
}

void *ReAllocateAligned(void *mem, u64 bytes, u64 alignment)
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

#if DEBUG
    ++gReAllocationsPerFrame;
#endif
    return mem;
}

void DeAllocateAligned(void **mem)
{
    if (mem)
    {
        HeapFree(GetProcessHeap(), 0, *mem);
        *mem = 0;
    #if DEBUG
        ++gDeAllocationsPerFrame;
    #endif
    }
}

//
// Stretchy Buffers
//

void buf_dealloc(BUF void *b)
{
    BufHdr *bufhdr = _BUFHDR(b);
    DeAllocateAligned(&bufhdr);
}

BUF void *buf_grow(BUF void *b, u64 new_count, u64 el_size)
{
    if (new_count > buf_cap(b))
    {
        CheckM(buf_cap(b) <= (U64_MAX - sizeof(BufHdr)) / el_size / 2, "Buffer overflow");

        u64 new_cap = __max(2 * buf_cap(b), new_count);
        Check(new_cap <= (U64_MAX - sizeof(BufHdr)) / el_size);

        u64 new_size = sizeof(BufHdr) + new_cap * el_size;

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
