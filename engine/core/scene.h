//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"
#include "tools/array.hpp"
#include "tools/const_string.h"

class ENGINE_API Scene
{
protected:
    explicit Scene(const ConstString& name = ConstString("Scene", CSTRLEN("Scene")))
        : m_Name(name)
    {}

    Scene(const Scene& other)     : m_Name(other.m_Name)             {}
    Scene(Scene&& other) noexcept : m_Name(RTTI::move(other.m_Name)) {}

    Scene& operator=(const Scene& other)     { m_Name = other.m_Name;             return *this; }
    Scene& operator=(Scene&& other) noexcept { m_Name = RTTI::move(other.m_Name); return *this; }

public:
    virtual ~Scene() {}

    void Destroy() { this->~Scene(); }

    virtual void OnSetCurrent()   {}
    virtual void OnUnsetCurrent() {}

    virtual void OnUpdate() {}

    constexpr const ConstString& GetName() const { return m_Name; }
    constexpr       ConstString& GetName()       { return m_Name; }

protected:
    ConstString m_Name;
};

class ENGINE_API SceneManager final
{
public:
    static SceneManager *Create(u64 capacity);
    static SceneManager *Get();

private:
    SceneManager(u64 capacity);

public:
    ~SceneManager();

    template<typename SceneInstance, typename ...ConstructorArgs, typename = RTTI::enable_if_t<RTTI::is_base_of_v<Scene, SceneInstance>>>
    Scene *PushScene(const ConstructorArgs&... args)
    {
        // @TODO(Roman): if (m_Scenes.size + sizeof(SceneInstance) > m_Scenes.cap) Grow();
        Check(m_Scenes.size + sizeof(SceneInstance) < m_Scenes.cap);
        ArenaBlock *block = m_Scenes.base + m_Scenes.size;
        block->size = sizeof(SceneInstance);
        CopyMemory(block->scene, &SceneInstance(args...), block->size);
        m_Scenes.size += StructFieldOffset(ArenaBlock, scene) + sizeof(SceneInstance);
        return block->scene;
    }

    void SetCurrentScene(Scene *scene);

    Scene *FindScene(const ConstString& name) const;

    constexpr const Scene *CurrentScene() const { return m_CurrentScene; }
    constexpr       Scene *CurrentScene()       { return m_CurrentScene; }

private:
    constexpr void *operator new(size_t)    { return Memory::Get()->PushToPA<SceneManager>(); }
    constexpr void  operator delete(void *) {}

    SceneManager(const SceneManager&) = delete;
    SceneManager(SceneManager&&)      = delete;

    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager& operator=(SceneManager&&)      = delete;

private:
    struct ArenaBlock
    {
        u64   size;
        #pragma warning(suppress: 4200)
        Scene scene[0];
    };

    struct Arena
    {
        ArenaBlock *base = null;
        u64         size = 0;
        u64         cap  = 0;
    };

    Scene *m_CurrentScene;
    Arena  m_Scenes;

    static SceneManager *s_SceneManager;
};
