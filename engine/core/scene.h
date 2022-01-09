// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "asset_manager/asset_manager.h"

namespace REV
{
    // @Cleanup(Roman): Temporary
    struct REV_API Entity
    {
        ConstArray<Vertex>  vertices;
        ConstArray<Index>   indices;
        u32                 ID;

        REV_INLINE  Entity() : vertices(null), indices(null), ID(++s_LastID) {}
        REV_INLINE ~Entity() {}
        REV_INLINE  Entity(const Entity& other) : vertices(other.vertices), indices(other.indices), ID(++s_LastID) {}
        REV_INLINE  Entity(Entity&& other)      : vertices(other.vertices), indices(other.indices), ID(other.ID)   {}

        void Create(u64 vcount, u64 icount);
        void SetData(const ConstArray<Vertex>& vertices, const ConstArray<Index>& indices);

        REV_INLINE Entity& operator=(const Entity& other) { if (this != &other) { vertices = other.vertices; indices = other.indices; ID = ++s_LastID; } return *this; }
        REV_INLINE Entity& operator=(Entity&& other)      { if (this != &other) { vertices = other.vertices; indices = other.indices; ID = other.ID;   } return *this; }
    
    private:
        static u32 s_LastID;
    };

    class REV_API SceneBase
    {
    protected:
        SceneBase(Allocator *allocator, const ConstString& name, u64 max_vertices, u64 max_indices);

    public:
        virtual ~SceneBase() {}

        virtual void OnSetCurrent()   = 0;
        virtual void OnUnsetCurrent() = 0;

        virtual void OnSetResourcesData() = 0;

        virtual void OnUpdate() = 0;

        void OnSetCurrentEx();
        void OnUnsetCurrentEx();
        void OnCopyDefaultResourcesToReadBackResources();

        void SetCurrentGraphicsShader(AssetHandle shader_asset);

        void SubmitEntity(Entity *entity);

        void FlushBatch();

    private:
        REV_DELETE_CONSTRS_AND_OPS(SceneBase);
        SceneBase() = delete;
    
    protected:
        ConstString                m_Name;
        Allocator                 *m_Allocator;

        AssetHandle                m_CurrentGraphicsShader;

        GPU::ResourceHandle        m_VertexBuffer;
        GPU::ResourceHandle        m_IndexBuffer;

        Vertex                    *m_Vertices;
        u64                        m_VerticesCount;
        const u64                  m_VerticesCapacity;

        Index                     *m_Indices;
        u64                        m_IndicesCount;
        const u64                  m_IndicesCapacity;
        Index                      m_MaxIndex;
    };
}
