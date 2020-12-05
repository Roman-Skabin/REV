//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/scene.h"

namespace REV
{

SceneManager *SceneManager::s_SceneManager = null;

SceneManager *SceneManager::Create(u64 capacity)
{
    REV_CHECK_M(!s_SceneManager, "Scene manager is already created. Use SceneManager::Get() function instead");
    s_SceneManager = new SceneManager(capacity);
    return s_SceneManager;
}

SceneManager *SceneManager::Get()
{
    REV_CHECK_M(s_SceneManager, "Scene manager is not created yet");
    return s_SceneManager;
}

SceneManager::SceneManager(u64 capacity)
    : m_CurrentScene(null),
      m_Scenes{null, 0, capacity}
{
    m_Scenes.base = cast<ArenaBlock *>(Memory::Get()->PushToPermanentArea(capacity));
}

SceneManager::~SceneManager()
{
    for (ArenaBlock *it = m_Scenes.base;
         it < m_Scenes.base + m_Scenes.size;
         it += StructFieldOffset(ArenaBlock, scene) + it->size)
    {
        it->scene->Destroy();
    }
}

void SceneManager::SetCurrentScene(Scene *scene)
{
    REV_CHECK_M(m_Scenes.base->scene <= scene && scene < (m_Scenes.base + m_Scenes.size)->scene,
           "This scene does not belong to this Scene Manager.");
    if (m_CurrentScene) m_CurrentScene->OnUnsetCurrent();
    m_CurrentScene = scene;
    m_CurrentScene->OnSetCurrent();
}

Scene *SceneManager::FindScene(const ConstString& name) const
{
    for (ArenaBlock *it = m_Scenes.base;
         it < m_Scenes.base + m_Scenes.size;
         it += StructFieldOffset(ArenaBlock, scene) + it->size)
    {
        if (it->scene->GetName() == name)
        {
            return it->scene;
        }
    }
    return null;
}

}
