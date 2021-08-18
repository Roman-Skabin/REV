#pragma once

#include "core/scene.h"
#include "math/mat.h"

class DemoScene final : public REV::SceneBase
{
public:
    DemoScene(REV::Allocator *allocator);
    ~DemoScene();

    virtual void OnSetCurrent()   override;
    virtual void OnUnsetCurrent() override;

    virtual void OnSetResourcesData() override;

    virtual void OnUpdate() override;

private:
    #pragma pack(push, 1)
    struct REV_ALIGN(1) CBufferData
    {
        REV::Math::m4 mvp;
        REV::Math::v4 sun_color;
        REV::Math::v3 center;
    };
    #pragma pack(pop)

    REV::GPU::ResourceHandle m_CBuffer;
    CBufferData              m_CBufferData;
    REV::Math::m4            m_Translation;
    REV::Entity              m_Rect;
    REV::AssetHandle         m_DemoShader;
    REV::StaticString<128>   m_OriginalWindowTitle;
};
