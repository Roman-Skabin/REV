// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "asset_manager/asset_manager.h"

namespace REV
{
    using EntityID = u32;

    #define REV_INVALID_ENTITY_ID REV_U32_MAX

    // @Cleanup(Roman): Temporary
    struct REV_API Entity
    {
        ConstArray<Vertex>  vertices;
        ConstArray<Index>   indices;
        EntityID            ID;

        REV_INLINE Entity()                    : vertices(null),           indices(null),          ID(++s_LastID) {}
        REV_INLINE Entity(const Entity& other) : vertices(other.vertices), indices(other.indices), ID(++s_LastID) {}
        REV_INLINE Entity(Entity&& other)      : vertices(other.vertices), indices(other.indices), ID(other.ID)   { other.ID = REV_INVALID_ENTITY_ID; }

        REV_INLINE ~Entity() {}

        void Create(u64 vcount, u64 icount);
        void SetData(const ConstArray<Vertex>& vertices, const ConstArray<Index>& indices);

        REV_INLINE Entity& operator=(const Entity& other) { if (this != &other) { vertices = other.vertices; indices = other.indices; ID = ++s_LastID;                                   } return *this; }
        REV_INLINE Entity& operator=(Entity&& other)      { if (this != &other) { vertices = other.vertices; indices = other.indices; ID = other.ID;   other.ID = REV_INVALID_ENTITY_ID; } return *this; }
    
    private:
        static EntityID s_LastID;
    };

    REV_INLINE operator==(const Entity& left, const Entity& right) { return left.ID == right.ID; }
    REV_INLINE operator!=(const Entity& left, const Entity& right) { return left.ID != right.ID; }

    class REV_API Scene
    {
    protected:
        Scene(const ConstString& name);

    public:
        virtual ~Scene() {}

        virtual void OnSetCurrent()   = 0;
        virtual void OnUnsetCurrent() = 0;

        virtual void OnUpdate() = 0;

        void OnUnsetCurrentEx();

    private:
        REV_DELETE_CONSTRS_AND_OPS(Scene);
        Scene() = delete;
    
    protected:
        ConstString m_Name;
    };
}
