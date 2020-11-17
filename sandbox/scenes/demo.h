#pragma once

#include "core/scene.h"
#include "renderer/gpu_program_manager.h"
#include "math/mat.h"

#pragma pack(push, 1)
struct Vertex
{
    Math::v4 pos;
    Math::v4 col;
};
#pragma pack(pop)

struct ENGINE_ALIGN(256) CBuffer_MVPMatrix
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
    IGraphicsProgram *m_GraphicsProgram;

    IGPUResource     *m_VertexBuffer;
    IGPUResource     *m_IndexBuffer;
    IGPUResource     *m_ConstantBuffer;

    Vertex            m_VertexData[4];
    u32               m_IndexData[6];
    CBuffer_MVPMatrix m_CBufferData;

    Math::m4          m_Translation;

    StaticString<128> m_OriginalWindowTitle;
};
