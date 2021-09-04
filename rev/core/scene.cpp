//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/scene.h"
#include "graphics/graphics_api.h"
#include "asset_manager/asset_manager.h"
#include "memory/memory.h"

namespace REV
{

//
// Entity
//

void Entity::Create(u64 vcount, u64 icount)
{
    Vertex *vertex_memory = cast<Vertex *>(Memory::Get()->PushToSceneArena(vcount * sizeof(Vertex) + icount * sizeof(Index)));
    Index  *index_memory  = cast<Index *>(vertex_memory + vcount);

    vertices = ConstArray(vertex_memory, vcount);
    indices  = ConstArray(index_memory,  icount);
}

void Entity::SetData(const ConstArray<Vertex>& vertices, const ConstArray<Index>& indices)
{
    CopyMemory(this->vertices.Data(), vertices.Data(), vertices.Count() * sizeof(Vertex));
    CopyMemory(this->indices.Data(),  indices.Data(),  indices.Count()  * sizeof(Index));
}

//
// SceneBase
//

SceneBase::SceneBase(Allocator *allocator, const ConstString& name, u64 max_vertices, u64 max_indices)
    : m_Name(name),
      m_Allocator(allocator),
      m_CurrentGraphicsShader(),
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
}

void SceneBase::OnSetCurrentEx()
{
    // @Optimize(Roman): use upload pointers?
    m_Vertices = cast<Vertex *>(m_Allocator->Allocate(m_VerticesCapacity * sizeof(Vertex) + m_IndicesCapacity * sizeof(Index))),
    m_Indices  = cast<Index *>(m_Vertices + m_VerticesCapacity);

    StaticString<CACHE_LINE_SIZE> vb_name(m_Name);
    StaticString<CACHE_LINE_SIZE> ib_name(m_Name);

    vb_name.PushBack(REV_CSTR_ARGS("VB"));
    ib_name.PushBack(REV_CSTR_ARGS("IB"));

    GPU::MemoryManager *gpu_memory_manager = GraphicsAPI::GetMemoryManager();

    m_VertexBuffer = gpu_memory_manager->AllocateVertexBuffer(cast<u32>(m_VerticesCapacity), false, ConstString(vb_name.Data(), vb_name.Length()));
    m_IndexBuffer  = gpu_memory_manager->AllocateIndexBuffer(cast<u32>(m_IndicesCapacity), false, ConstString(ib_name.Data(), ib_name.Length()));

    OnSetCurrent();
}

void SceneBase::OnUnsetCurrentEx()
{
    OnUnsetCurrent();

    AssetManager::Get()->FreeSceneAssets();
    GraphicsAPI::GetMemoryManager()->FreeSceneMemory();
    m_Allocator->DeAlloc(m_Vertices);
    Memory::Get()->ResetSceneArena();
}

void SceneBase::SetCurrentGraphicsShader(AssetHandle shader_asset)
{
    REV_CHECK_M(AssetManager::Get()->GetAsset(shader_asset)->kind == ASSET_KIND_SHADER, "Asset handle passed to the SetCurrentGraphicsShader is not a handle to a shader asset");
    m_CurrentGraphicsShader = shader_asset;
}

void SceneBase::SubmitEntity(Entity *entity)
{
    if ((m_VerticesCapacity - m_VerticesCount < entity->vertices.Count())
    ||  (m_IndicesCapacity  - m_IndicesCount  < entity->indices.Count()))
    {
        FlushBatch();
    }

    CopyMemory(m_Vertices + m_VerticesCount, entity->vertices.Data(), entity->vertices.Count() * sizeof(Vertex));
    m_VerticesCount += entity->vertices.Count();

    Index index_offset = m_MaxIndex ? m_MaxIndex + 1 : 0;

    for (u64 i = 0; i < entity->indices.Count(); ++i)
    {
        m_Indices[m_IndicesCount + i] = index_offset + entity->indices[i];

        if (m_MaxIndex < entity->indices[i])
        {
            m_MaxIndex = entity->indices[i];
        }
    }

    m_IndicesCount += entity->indices.Count();
}

void SceneBase::FlushBatch()
{
    if (m_VerticesCount || m_IndicesCount)
    {
        REV_CHECK_M(m_CurrentGraphicsShader.index != REV_U64_MAX, "There is no current graphics shader. Use SceneBase::SetCurrentGraphicsShader to set it.")

        GPU::MemoryManager *gpu_memory_manager = GraphicsAPI::GetMemoryManager();
        GPU::ShaderManager *shader_manager     = GraphicsAPI::GetShaderManager();
        GPU::DeviceContext *device_context     = GraphicsAPI::GetDeviceContext();
        AssetManager       *asset_manager      = AssetManager::Get();

        if (device_context->FrameStarted())
        {
            gpu_memory_manager->SetBufferData(m_VertexBuffer, m_Vertices);
            gpu_memory_manager->SetBufferData(m_IndexBuffer, m_Indices);
            OnSetResourcesData();
        }
        else
        {
            REV_ERROR_M("FlushBatch gotta be called during the frame ONLY!!!");
        }

        Asset *graphics_shader_asset = asset_manager->GetAsset(m_CurrentGraphicsShader);

        shader_manager->SetCurrentGraphicsShader(graphics_shader_asset->shader_handle);

        shader_manager->BindVertexBuffer(graphics_shader_asset->shader_handle, m_VertexBuffer);
        shader_manager->BindIndexBuffer(graphics_shader_asset->shader_handle, m_IndexBuffer);

        shader_manager->Draw(graphics_shader_asset->shader_handle);

        ZeroMemory(m_Vertices, m_VerticesCount * sizeof(Vertex));
        ZeroMemory(m_Indices,  m_IndicesCount  * sizeof(Index));

        m_VerticesCount = 0;
        m_IndicesCount  = 0;
        m_MaxIndex      = 0;
    }
}

}
