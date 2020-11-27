#pragma once

#include "core/scene.h"
#include "graphics/program_manager.h"
#include "math/mat.h"

#pragma pack(push, 1)
struct Vertex
{
    Math::v4 pos;
    Math::v4 col;
};
#pragma pack(pop)

struct CBufferData
{
    Math::m4 MVP;
    Math::v3 center;
};

class DemoScene final : public Scene
{
public:
    DemoScene();
    ~DemoScene();

    virtual void OnSetCurrent() override;
    virtual void OnUnsetCurrent() override;

    virtual void OnUpdate() override;

private:
    GPU::GraphicsProgramHandle m_GraphicsProgram;

    GPU::ResourceHandle m_VertexBuffer;
    GPU::ResourceHandle m_IndexBuffer;
    GPU::ResourceHandle m_ConstantBuffer;

    Vertex      m_VertexData[4];
    u32         m_IndexData[6];
    CBufferData m_CBufferData;

    Math::m4 m_Translation;

    StaticString<128> m_OriginalWindowTitle;
};
