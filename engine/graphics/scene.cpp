// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/scene.h"
#include "graphics/graphics_api.h"
#include "asset_manager/asset_manager.h"
#include "memory/memory.h"
#include "application.h"

namespace REV
{

//
// Entity
//

REV_GLOBAL volatile s32 g_LastEntityID = REV_INVALID_ENTITY_ID;

Entity::Entity(const ConstString& name)
    : m_VertexBuffer(),
      m_IndexBuffer(),
      m_Name(name),
      m_ID(_InterlockedIncrement(&g_LastEntityID))
{
    if (m_ID == REV_INVALID_ENTITY_ID - 1)
    {
        REV_WARNING_M("This is the last Entity ID, next one gonna be invalid");
    }
}

Entity::Entity(Entity&& other)
    : m_VertexBuffer(RTTI::move(other.m_VertexBuffer)),
      m_IndexBuffer(RTTI::move(other.m_IndexBuffer)),
      m_Name(RTTI::move(other.m_Name)),
      m_ID(other.m_ID)
{
    other.m_ID = REV_INVALID_ENTITY_ID;
}

Entity::~Entity()
{
    if (Valid())
    {
        // @TODO(Roman): Destroy
        m_ID = REV_INVALID_ENTITY_ID;
    }
}

void Entity::SetVertices(const void *vertices, u64 count, u64 stride)
{
    REV_CHECK(Valid());

    MemoryManager *memory_manager = GraphicsAPI::GetMemoryManager();

    StaticString<64> vb_name(REV_CSTR_ARGS("VB_"));
    vb_name += m_Name;

    m_VertexBuffer = memory_manager->AllocateVertexBuffer(count, stride, false, vb_name.ToConstString());

    memory_manager->SetBufferData(m_VertexBuffer, vertices);
}

void Entity::SetIndices(const void *indices, u64 count, u64 stride)
{
    REV_CHECK(Valid());

    MemoryManager *memory_manager = GraphicsAPI::GetMemoryManager();

    StaticString<64> ib_name(REV_CSTR_ARGS("IB_"));
    ib_name += m_Name;

    m_IndexBuffer = memory_manager->AllocateIndexBuffer(count, stride, false, ib_name.ToConstString());

    memory_manager->SetBufferData(m_IndexBuffer, indices);
}

Entity& Entity::operator=(Entity&& other)
{
    if (this != &other)
    {
        m_VertexBuffer = RTTI::move(other.m_VertexBuffer);
        m_IndexBuffer  = RTTI::move(other.m_IndexBuffer);
        m_Name         = RTTI::move(other.m_Name);
        m_ID           = other.m_ID;

        other.m_ID = REV_INVALID_ENTITY_ID;
    }
}

//
// Scene
//

Scene::Scene(const ConstString& name)
    : m_Name(name)
{
}

void Scene::OnUnsetCurrentEx()
{
    OnUnsetCurrent();

    Application::Get()->GetForwardPlusPipeline().ResetPasses();
    AssetManager::Get()->FreeSceneAssets();
    GraphicsAPI::GetMemoryManager()->FreePerSceneMemory();
    Memory::Get()->ResetSceneArena();
}

}
