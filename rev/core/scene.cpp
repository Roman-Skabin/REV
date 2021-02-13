//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/scene.h"
#include "graphics/graphics_api.h"
#include "asset_manager/asset_manager.h"

namespace REV
{

//
//
//

void Entity::Create(u64 vcount, u64 icount)
{
    this->vcount = vcount;
    this->icount = icount;

    vertices = cast<Vertex *>(allocator->Allocate(vcount * sizeof(Vertex) + icount * sizeof(Index)));
    indices  = cast<Index *>(cast<byte *>(vertices) + vcount * sizeof(Vertex));
}

void Entity::Destroy()
{
    allocator->DeAlloc(vertices);
}

//
//
//

SceneBase::SceneBase(Allocator *allocator, const ConstString& name, const StaticString<MAX_PATH>& file_with_shaders, u64 max_vertices, u64 max_indices)
    : m_Name(name),
      m_Allocator(allocator),
      m_GraphicsProgram(GraphicsAPI::GetProgramManager()->CreateGraphicsProgram(file_with_shaders)),
      m_VertexBuffer(),
      m_IndexBuffer(),
      m_Vertices(null),
      m_VerticesCount(0),
      m_VerticesCapacity(max_vertices),
      m_Indices(null),
      m_IndicesCount(0),
      m_IndicesCapacity(max_indices),
      m_MaxIndex(0)
{
    // @Optimize(Roman): use upload pointers?
    m_Vertices = cast<Vertex *>(Memory::Get()->PushToPermanentArea(m_VerticesCapacity * sizeof(Vertex) + m_IndicesCapacity * sizeof(Index))),
    m_Indices  = cast<Index *>(m_Vertices + m_VerticesCapacity * sizeof(Vertex));
}

void SceneBase::OnSetCurrentEx()
{
    GPU::MemoryManager  *gpu_memory_manager = GraphicsAPI::GetMemoryManager();
    GPU::ProgramManager *program_manager    = GraphicsAPI::GetProgramManager();

    StaticString<64> vb_name("VB_", REV_CSTRLEN("VB_"));
    StaticString<64> ib_name("IB_", REV_CSTRLEN("IB_"));

    vb_name += m_Name;
    ib_name += m_Name;

    m_VertexBuffer = gpu_memory_manager->AllocateVertexBuffer(cast<u32>(m_VerticesCapacity), vb_name);
    m_IndexBuffer  = gpu_memory_manager->AllocateIndexBuffer(cast<u32>(m_IndicesCapacity), ib_name);

    AssetManager::Get()->ParseREVAMFile(m_Name);

    for (Asset& asset : AssetManager::Get()->GetSceneAssets())
    {
        program_manager->AttachResource(m_GraphicsProgram, asset.resource);
    }

    OnSetCurrent();
}

void SceneBase::OnUnsetCurrentEx()
{
    AssetManager::Get()->FreeSceneAssets();
    OnUnsetCurrent();
}

void SceneBase::SubmitEntity(Entity *entity)
{
    if ((m_VerticesCapacity - m_VerticesCount < entity->vcount)
    ||  (m_IndicesCapacity  - m_IndicesCount  < entity->icount))
    {
        FlushBatch();
    }

    CopyMemory(m_Vertices + m_VerticesCount, entity->vertices, entity->vcount * sizeof(Vertex));
    m_VerticesCount += entity->vcount;

    Index index_offset = m_MaxIndex ? m_MaxIndex + 1 : 0;

    for (u64 i = 0; i < entity->icount; ++i)
    {
        m_Indices[m_IndicesCount + i] = index_offset + entity->indices[i];

        if (m_MaxIndex < entity->indices[i])
        {
            m_MaxIndex = entity->indices[i];
        }
    }

    m_IndicesCount += entity->icount;
}

void SceneBase::FlushBatch()
{
    if (m_VerticesCount || m_IndicesCount)
    {
        GPU::MemoryManager  *gpu_memory_manager = GraphicsAPI::GetMemoryManager();
        GPU::ProgramManager *program_manager    = GraphicsAPI::GetProgramManager();
        GPU::Renderer       *renderer           = GraphicsAPI::GetRenderer();

        if (renderer->FrameStarted())
        {
            gpu_memory_manager->SetBufferData(m_VertexBuffer, m_Vertices);
            gpu_memory_manager->SetBufferData(m_IndexBuffer, m_Indices);
            OnSetResourcesData();
        }
        else
        {
            REV_FAILED_M("FlushBatch gotta be called during the frame ONLY!!!");
        }

        program_manager->SetCurrentGraphicsProgram(m_GraphicsProgram);

        program_manager->BindVertexBuffer(m_GraphicsProgram, m_VertexBuffer);
        program_manager->BindIndexBuffer(m_GraphicsProgram, m_IndexBuffer);

        program_manager->Draw(m_GraphicsProgram);

        ZeroMemory(m_Vertices, m_VerticesCount * sizeof(Vertex));
        ZeroMemory(m_Indices,  m_IndicesCount  * sizeof(Index));

        m_VerticesCount = 0;
        m_IndicesCount  = 0;
        m_MaxIndex      = 0;
    }
}

}
