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
    #pragma pack(push, 1)
    struct REV_ALIGN(1) CBufferData
    {
        REV::Math::m4 mvp;
        REV::Math::v4 sun_color;
        REV::Math::v3 center;
        u32           entity_id;
    };
    #pragma pack(pop)

    REV::GPU::ResourceHandle m_CBuffer;
    CBufferData              m_CBufferData;
    REV::Math::m4            m_Translation;
    REV::Entity              m_Rect;
    REV::AssetHandle         m_DemoShader;
    u32                      m_PickedEntity;
    REV::StaticString<128>   m_OriginalWindowTitle;
};
