#pragma once

#include "core/scene.h"
#include "graphics/program_manager.h"
#include "math/mat.h"

#pragma pack(push, 1)
struct Vertex
{
    REV::Math::v4 pos;
    REV::Math::v4 col;
};
#pragma pack(pop)

struct CBufferData
{
    REV::Math::m4 MVP;
    REV::Math::v3 center;
};

class DemoScene final : public REV::Scene
{
public:
    DemoScene();
    ~DemoScene();

    virtual void OnSetCurrent() override;
    virtual void OnUnsetCurrent() override;

    virtual void OnUpdate() override;

private:
    REV::GPU::GraphicsProgramHandle m_GraphicsProgram;

    REV::GPU::ResourceHandle m_VertexBuffer;
    REV::GPU::ResourceHandle m_IndexBuffer;
    REV::GPU::ResourceHandle m_ConstantBuffer;

    Vertex      m_VertexData[4];
    REV::u32    m_IndexData[6];
    CBufferData m_CBufferData;

    REV::Math::m4 m_Translation;

    REV::StaticString<128> m_OriginalWindowTitle;
};
