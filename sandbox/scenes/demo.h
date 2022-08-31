#pragma once

#include "graphics/scene.h"
#include "math/mat.h"

class DemoScene final : public REV::Scene
{
public:
    DemoScene(REV::Allocator *allocator);
    ~DemoScene();

    virtual void OnSetCurrent()   override;
    virtual void OnUnsetCurrent() override;

    virtual void OnUpdate() override;

private:
    void CreateResources();
    void CreateShaders();
    void AddOpaquePass();

private:
    #pragma pack(push, 1)
    struct REV_ALIGN(1) CBufferData
    {
        REV::Math::m4 mvp;
        REV::Math::v4 sun_color;
        u32           entity_id;
    };
    #pragma pack(pop)

    REV::AssetHandle         m_DemoShader;

    REV::ResourceHandle m_CBuffer;
    REV::ResourceHandle m_WoodTexture;
    REV::ResourceHandle m_WoodTextureSampler;

    REV::Entity              m_Rect;

    REV::Math::m4            m_Translation;
    u32                      m_PickedEntity;
    CBufferData              m_CBufferData;
    REV::StaticString<128>   m_OriginalWindowTitle;
};
