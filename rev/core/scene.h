//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "graphics/program_manager.h"
#include "math/vec.h"
#include "tools/const_string.h"
#include "tools/static_string.hpp"
#include "core/allocator.h"

namespace REV
{
    // @TODO(Roman): Better place for Vertex?
    #pragma pack(push, 1)
    struct REV_ALIGN(1) Vertex
    {
        Math::v4 position;
        Math::v4 normal;
        Math::v2 tex_coord;
    };
    #pragma pack(pop)

    typedef u32 Index;

    // @Cleanup(Roman): Temporary
    struct REV_API Entity
    {
        Vertex    *vertices;
        u64        vcount;
        Index     *indices;
        u64        icount;
        Allocator *allocator;

        void Create(u64 vcount, u64 icount);
        void Destroy();
    };

    class REV_API SceneBase
    {
    protected:
        SceneBase(Allocator *allocator, const ConstString& name, const StaticString<REV_PATH_CAPACITY>& file_with_shaders, u64 max_vertices, u64 max_indices);

    public:
        virtual ~SceneBase() {}

        virtual void OnSetCurrent()   = 0;
        virtual void OnUnsetCurrent() = 0;

        virtual void OnSetResourcesData() = 0;

        virtual void OnUpdate() = 0;

        void OnSetCurrentEx();
        void OnUnsetCurrentEx();

        void SubmitEntity(Entity *entity);

        void FlushBatch();

    private:
        REV_DELETE_CONSTRS_AND_OPS(SceneBase);
        SceneBase() = delete;
    
    protected:
        ConstString                m_Name;
        Allocator                 *m_Allocator;

        GPU::GraphicsProgramHandle m_GraphicsProgram;
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
