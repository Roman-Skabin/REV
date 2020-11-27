#include "scenes/demo.h"
#include "application.h"
#include "math/color.h"
#include "tools/static_string_builder.hpp"

DemoScene::DemoScene()
    : Scene(ConstString("DemoScene", CSTRLEN("DemoScene"))),
      m_GraphicsProgram(GraphicsAPI::GetProgramManager()->CreateGraphicsProgram("../sandbox/assets/shaders/shader.vert",
                                                                                "../sandbox/assets/shaders/shader.pixel")),
      m_VertexBuffer(),
      m_IndexBuffer(),
      m_ConstantBuffer(),
      m_VertexData{{ Math::v4(-0.5f, -0.5f, 0.5f, 1.0f), Math::Color::g_RedA1    },
                   { Math::v4(-0.5f,  0.5f, 0.5f, 1.0f), Math::Color::g_GreenA1  },
                   { Math::v4( 0.5f,  0.5f, 0.5f, 1.0f), Math::Color::g_BlueA1   },
                   { Math::v4( 0.5f, -0.5f, 0.5f, 1.0f), Math::Color::g_YellowA1 }},
      m_IndexData{0, 1, 2,
                  0, 2, 3},
      m_CBufferData{Math::m4::identity()},
      m_Translation(Math::m4::identity()),
      m_OriginalWindowTitle(Application::Get()->GetWindow().Title())
{
}

DemoScene::~DemoScene()
{
}

void DemoScene::OnSetCurrent()
{
    GPU::MemoryManager *gpu_memory_manager = GraphicsAPI::GetMemoryManager();
    {
        m_VertexBuffer   = gpu_memory_manager->AllocateVertexBuffer(cast<u32>(ArrayCount(m_VertexData)), sizeof(Vertex));
        m_IndexBuffer    = gpu_memory_manager->AllocateIndexBuffer(cast<u32>(ArrayCount(m_IndexData)));
        m_ConstantBuffer = gpu_memory_manager->AllocateConstantBuffer(sizeof(CBufferData), StaticString<64>("MVPMatrix", CSTRLEN("MVPMatrix")));
    }

    gpu_memory_manager->StartImmediateExecution();
    {
        gpu_memory_manager->SetResourceDataImmediate(m_VertexBuffer, m_VertexData);
        gpu_memory_manager->SetResourceDataImmediate(m_IndexBuffer, m_IndexData);
        gpu_memory_manager->SetResourceDataImmediate(m_ConstantBuffer, &m_CBufferData);
    }
    gpu_memory_manager->EndImmediateExecution();

    GraphicsAPI::GetProgramManager()->AttachResource(m_GraphicsProgram, m_ConstantBuffer);
}

void DemoScene::OnUnsetCurrent()
{
    GraphicsAPI::GetMemoryManager()->FreeMemory();
}

void DemoScene::OnUpdate()
{
    Application    *application = Application::Get();
    const Keyboard& keyboard    = application->GetInput()->GetKeyboard();
    Window&         window      = application->GetWindow();
    Timer&          timer       = application->GetTimer();

    local f32 last_print_time;
    if (!window.Fullscreened() && timer.Seconds() - last_print_time >= 0.1)
    {
        f32 FPS = timer.TicksPerSecond() / cast<f32>(timer.DeltaTicks());

        StaticStringBuilder<128> ssb;
        ssb.Precision = 2;
        ssb.Build(m_OriginalWindowTitle, " - FPS: ", FPS, " - MSPF: ", 1000.0f / FPS);

        window.SetTitle(ssb.ToString());

        last_print_time = timer.Seconds();
    }

    if (keyboard[KEY::F11].Pressed())
    {
        window.RequstFullscreen(!window.Fullscreened());
    }
    else if (keyboard[KEY::V].Pressed())
    {
        Renderer *renderer = GraphicsAPI::GetRenderer();
        renderer->SetVSync(!renderer->VSyncEnabled());
    }
    
    Math::v4 translation;

    if (keyboard[KEY::LEFT].Down())
    {
        translation.x = -timer.DeltaSeconds();
    }
    else if (keyboard[KEY::RIGHT].Down())
    {
        translation.x = timer.DeltaSeconds();
    }
    if (keyboard[KEY::UP].Down())
    {
        translation.y = timer.DeltaSeconds();
    }
    else if (keyboard[KEY::DOWN].Down())
    {
        translation.y = -timer.DeltaSeconds();
    }

    m_Translation *= Math::m4::translation(translation);

    m_CBufferData.MVP = m_Translation * Math::m4::rotation_z(timer.Seconds());

    Math::v4 center = m_CBufferData.MVP * ((m_VertexData[0].pos + m_VertexData[2].pos) / 2.0f);
    m_CBufferData.center = center.xyz;

    {
        GraphicsAPI::GetMemoryManager()->SetResoucreData(m_ConstantBuffer, &m_CBufferData);

        GPU::ProgramManager *program_manager = GraphicsAPI::GetProgramManager();
        program_manager->SetCurrentGraphicsProgram(m_GraphicsProgram);

        program_manager->BindVertexBuffer(m_GraphicsProgram, m_VertexBuffer);
        program_manager->BindIndexBuffer(m_GraphicsProgram, m_IndexBuffer);
        program_manager->DrawIndices(m_GraphicsProgram);
    }
}
