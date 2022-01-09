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

EntityID Entity::s_LastID = REV_INVALID_ENTITY_ID;

void Entity::Create(u64 vcount, u64 icount)
{
    Vertex *vertex_memory = cast(Vertex *, Memory::Get()->PushToSceneArena(vcount * sizeof(Vertex) + icount * sizeof(Index)));
    Index  *index_memory  = cast(Index *, vertex_memory + vcount);
    
    vertices = ConstArray(vertex_memory, vcount);
    indices  = ConstArray(index_memory,  icount);
}

void Entity::SetData(const ConstArray<Vertex>& vertices, const ConstArray<Index>& indices)
{
    CopyMemory(this->vertices.Data(), vertices.Data(), vertices.Count() * sizeof(Vertex));
    CopyMemory(this->indices.Data(),  indices.Data(),  indices.Count()  * sizeof(Index));
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

    Application::Get()->GetForwardPlusPipeline()->ResetPasses();
    AssetManager::Get()->FreeSceneAssets();
    GraphicsAPI::GetMemoryManager()->FreeSceneMemory();
    Memory::Get()->ResetSceneArena();
}

}
