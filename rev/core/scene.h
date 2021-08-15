//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "graphics/shader_manager.h"
#include "math/vec.h"
#include "tools/const_string.h"
#include "tools/static_string.hpp"
#include "core/allocator.h"

namespace REV
{
    typedef u32 Index;

    // @Cleanup(Roman): Temporary
    struct REV_API Entity
    {
        ConstArray<Vertex>  vertices;
        ConstArray<Index>   indices;
        Allocator          *allocator;

        REV_INLINE Entity(Allocator *allocator)
            : vertices(null),
              indices(null),
              allocator(allocator)
        {
        }

        REV_INLINE ~Entity() {}

        void Create(u64 vcount, u64 icount);
        void Destroy();

        void SetData(const ConstArray<Vertex>& vertices, const ConstArray<Index>& indices);
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

        void SetCurrentGraphicsShader(AssetHandle shader_asset);

        void SubmitEntity(Entity *entity);

        void FlushBatch();

    private:
        REV_DELETE_CONSTRS_AND_OPS(SceneBase);
        SceneBase() = delete;
    
    protected:
        ConstString                m_Name;
        Allocator                 *m_Allocator;

        AssetHandle               m_CurrentGraphicsShader;

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
