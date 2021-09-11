//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "memory/arena.h"

namespace REV
{
    class REV_API Memory final
    {
    public:
        enum : u64 { MAX_MEMORY = 0xFFFFFFFFFFFFF000ui64 };

        using Marker = Arena::Marker;

        static Memory *Create(u64 frame_arena_capacity, u64 scene_arena_capacity, u64 permanent_arena_capacity);
        static Memory *Get();

    private:
        Memory(u64 frame_arena_capacity, u64 scene_arena_capacity, u64 permanent_arena_capacity);

    public:
        ~Memory();

        REV_INLINE void   *PushToFrameArena(u64 bytes)                             { return m_FrameArena.PushBytes(bytes);                   }
        REV_INLINE void   *PushToFrameArenaAligned(u64 bytes, u64 alignment)       { return m_FrameArena.PushBytesAligned(bytes, alignment); }
        REV_INLINE void    ResetFrameArena(Marker marker = 0)                      { return m_FrameArena.Clear(marker);                      }
        REV_INLINE Marker  SaveFrameArenaMarker()                            const { return m_FrameArena.SaveMarker();                       }

        REV_INLINE void   *PushToSceneArena(u64 bytes)                             { return m_SceneArena.PushBytes(bytes);                   }
        REV_INLINE void   *PushToSceneArenaAligned(u64 bytes, u64 alignment)       { return m_SceneArena.PushBytesAligned(bytes, alignment); }
        REV_INLINE void    ResetSceneArena(Marker marker = 0)                      { return m_SceneArena.Clear(marker);                      }
        REV_INLINE Marker  SaveSceneArenaMarker()                            const { return m_SceneArena.SaveMarker();                       }

        REV_INLINE void *PushToPermanentArena(u64 bytes)                       { return m_PermanentArena.PushBytes(bytes);                   }
        REV_INLINE void *PushToPermanentArenaAligned(u64 bytes, u64 alignment) { return m_PermanentArena.PushBytesAligned(bytes, alignment); }

        template<typename T> REV_INLINE T *PushToFA(u64 count = 1)                     { return m_FrameArena.Push<T>(count);             }
        template<typename T> REV_INLINE T *PushToFAA(u64 count = 1, u64 alignment = 0) { return m_FrameArena.PushA<T>(count, alignment); }

        template<typename T> REV_INLINE T *PushToSA(u64 count = 1)                     { return m_SceneArena.Push<T>(count);             }
        template<typename T> REV_INLINE T *PushToSAA(u64 count = 1, u64 alignment = 0) { return m_SceneArena.PushA<T>(count, alignment); }

        template<typename T> REV_INLINE T *PushToPA(u64 count = 1)                     { return m_PermanentArena.Push<T>(count);             }
        template<typename T> REV_INLINE T *PushToPAA(u64 count = 1, u64 alignment = 0) { return m_PermanentArena.PushA<T>(count, alignment); }

        const Arena& FrameArena()     const { return m_FrameArena;     }
        const Arena& SceneArena()     const { return m_SceneArena;     }
        const Arena& PermanentArena() const { return m_PermanentArena; }

        Arena& FrameArena()     { return m_FrameArena;     }
        Arena& SceneArena()     { return m_SceneArena;     }
        Arena& PermanentArena() { return m_PermanentArena; }

    private:
        REV_DELETE_CONSTRS_AND_OPS(Memory);

    private:
        Arena m_FrameArena;
        Arena m_SceneArena;
        Arena m_PermanentArena;
    };
}
