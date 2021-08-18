//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "memory/memory.h"

namespace REV
{

REV_GLOBAL Memory *g_Memory = null;

Memory *Memory::Create(u64 frame_arena_capacity, u64 scene_arena_capacity, u64 permanent_arena_capacity)
{
    REV_CHECK_M(!g_Memory, "Memory is already created. Use Memory::Get() function instead");
    REV_LOCAL Memory memory(frame_arena_capacity, scene_arena_capacity, permanent_arena_capacity);
    g_Memory = &memory;
    return g_Memory;
}

Memory *Memory::Get()
{
    REV_CHECK_M(g_Memory, "Memory is not created yet");
    return g_Memory;
}

Memory::Memory(u64 frame_arena_capacity, u64 scene_arena_capacity, u64 permanent_arena_capacity)
    : m_FrameArena(),
      m_SceneArena(),
      m_PermanentArena()
{
    REV_CHECK_M(frame_arena_capacity,     "Frame arena capacity must be greater than 0");
    REV_CHECK_M(scene_arena_capacity,     "Scene arena capacity must be greater than 0");
    REV_CHECK_M(permanent_arena_capacity, "Permanent arena capacity must be greater than 0");

    frame_arena_capacity     = AlignUp(frame_arena_capacity,     PAGE_SIZE);
    scene_arena_capacity     = AlignUp(scene_arena_capacity,     PAGE_SIZE);
    permanent_arena_capacity = AlignUp(permanent_arena_capacity, PAGE_SIZE);

    u64 memory_capacity = frame_arena_capacity + scene_arena_capacity + permanent_arena_capacity;
    REV_CHECK(memory_capacity <= MAX_MEMORY);

#if REV_PLATFORM_WIN64
    byte *base = cast<byte *>(VirtualAlloc(null, memory_capacity, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    REV_CHECK_M(base, "Physical memory overflow");
#else
    byte *base = mmap(null, memory_capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    REV_CHECK_M(base != MAP_FAILED, "Physical memory overflow");
#endif

    m_FrameArena.m_Base     = base;
    m_SceneArena.m_Base     = base + frame_arena_capacity;
    m_PermanentArena.m_Base = base + frame_arena_capacity + scene_arena_capacity;

    m_FrameArena.m_Capacity     = frame_arena_capacity;
    m_SceneArena.m_Capacity     = scene_arena_capacity;
    m_PermanentArena.m_Capacity = permanent_arena_capacity;
}

Memory::~Memory()
{
    if (m_FrameArena.m_Base)
    {
    #if REV_PLATFORM_WIN64
        REV_DEBUG_RESULT(VirtualFree(m_FrameArena.m_Base, 0, MEM_RELEASE));
    #else
        REV_DEBUG_RESULT(!munmap(m_FrameArena.m_Base, m_FrameArena.m_Capacity + m_SceneArena.m_Capacity + m_PermanentArena.m_Capacity));
    #endif
    }
}

}
