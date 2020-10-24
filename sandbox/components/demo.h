#pragma once

#include "core/app_component.h"
#include "renderer/gpu_program_manager.h"

class DemoComponent final : public AppComponent
{
public:
    DemoComponent();
    ~DemoComponent();

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    virtual void OnUpdate() override;

private:
    Application      *m_Application;
    IGraphicsProgram *m_GraphicsProgram;
    IGPUResource     *m_VertexBuffer;
    IGPUResource     *m_IndexBuffer;
    StaticString<128> m_OriginalWindowTitle;
};
