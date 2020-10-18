#include "components/demo.h"
#include "application.h"
#include "math/vec.h"
#include "math/color.h"

#pragma pack(push, 1)
struct Vertex
{
    v4 pos;
    v4 col;
};
#pragma pack(pop)

DemoComponent::DemoComponent()
    : AppComponent("DemoComponent"),
      m_Application(Application::Get()),
      m_GraphicsProgram(GraphicsAPI::GetGPUProgramManager()->CreateGraphicsProgram("../sandbox/assets/shaders/shader.vert",
                                                                                   "../sandbox/assets/shaders/shader.pixel")),
      m_VertexBuffer(null),
      m_IndexBuffer(null)
{
    strcpy(m_OriginalWindowTitle, m_Application->GetWindow().Title());

    IGPUMemoryManager *gpu_memory_manager = GraphicsAPI::GetGPUMemoryManager();

    Vertex vertices[] =
    {
        { v4(-0.5f, -0.5f, 0.0f, 0.0f), v4(1.0f, 0.0f, 0.0f, 1.0f) },
        { v4(-0.5f,  0.5f, 0.0f, 0.0f), v4(0.0f, 1.0f, 0.0f, 1.0f) },
        { v4( 0.5f,  0.5f, 0.0f, 0.0f), v4(0.0f, 0.0f, 1.0f, 1.0f) },
        { v4( 0.5f, -0.5f, 0.0f, 0.0f), v4(0.0f, 0.0f, 0.0f, 1.0f) }
    };
    m_VertexBuffer = gpu_memory_manager->AllocateVB(cast<u32>(ArrayCount(vertices)), sizeof(Vertex));
    m_VertexBuffer->SetDataImmediate(vertices);

    u32 indices[] =
    {
        0, 1, 2,
        0, 2, 3
    };
    m_IndexBuffer = gpu_memory_manager->AllocateIB(cast<u32>(ArrayCount(indices)));
    m_IndexBuffer->SetDataImmediate(indices);
}

DemoComponent::~DemoComponent()
{
}

void DemoComponent::OnAttach()
{
}

void DemoComponent::OnDetach()
{
}

void DemoComponent::OnUpdate()
{
    const Keyboard& keyboard = m_Application->GetInput()->GetKeyboard();
    Window&         window   = m_Application->GetWindow();
    Timer&          timer    = m_Application->GetTimer();

    local f32 last_print_time;
    if (!window.Fullscreened() && timer.Seconds() - last_print_time >= 0.1)
    {
        f32 FPS = timer.TicksPerSecond() / cast<f32>(timer.DeltaTicks());

        char buffer[64];
        sprintf(buffer, "%s - FPS: %f - MSPF: %f", m_OriginalWindowTitle, FPS, 1000.0f / FPS);

        window.SetTitle(buffer);

        last_print_time = timer.Seconds();
    }

    if (keyboard[KEY::F11].Pressed())
    {
        window.RequstFullscreen(!window.Fullscreened());
    }
    else if (keyboard[KEY::V].Pressed())
    {
        IRenderer *renderer = GraphicsAPI::GetRenderer();
        renderer->SetVSync(!renderer->VSyncEnabled());
    }

    GraphicsAPI::GetGPUProgramManager()->SetCurrentGraphicsProgram(m_GraphicsProgram);

    m_GraphicsProgram->BindVertexBuffer(m_VertexBuffer);
    m_GraphicsProgram->BindIndexBuffer(m_IndexBuffer);
    m_GraphicsProgram->DrawIndices();
}
