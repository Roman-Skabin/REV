#include "scenes/demo.h"
#include "application.h"
#include "math/color.h"
#include "tools/static_string_builder.hpp"

DemoScene::DemoScene()
    : Scene(REV::ConstString("DemoScene", REV_CSTRLEN("DemoScene"))),
      m_GraphicsProgram(REV::GraphicsAPI::GetProgramManager()->CreateGraphicsProgram("../sandbox/assets/shaders/demo_shaders.hlsl")),
      m_VertexBuffer(),
      m_IndexBuffer(),
      m_ConstantBuffer(),
      m_VertexData{{ REV::Math::v4(-0.5f, -0.5f, 0.5f, 1.0f), REV::Math::Color::g_RedA1    },
                   { REV::Math::v4(-0.5f,  0.5f, 0.5f, 1.0f), REV::Math::Color::g_GreenA1  },
                   { REV::Math::v4( 0.5f,  0.5f, 0.5f, 1.0f), REV::Math::Color::g_BlueA1   },
                   { REV::Math::v4( 0.5f, -0.5f, 0.5f, 1.0f), REV::Math::Color::g_YellowA1 }},
      m_IndexData{0, 1, 2,
                  0, 2, 3},
      m_CBufferData{REV::Math::m4::identity()},
      m_Translation(REV::Math::m4::identity()),
      m_OriginalWindowTitle(REV::Application::Get()->GetWindow().Title())
{
}

DemoScene::~DemoScene()
{
}

void DemoScene::OnSetCurrent()
{
    REV::GPU::MemoryManager *gpu_memory_manager = REV::GraphicsAPI::GetMemoryManager();
    {
        m_VertexBuffer   = gpu_memory_manager->AllocateVertexBuffer(REV::cast<REV::u32>(REV::ArrayCount(m_VertexData)), sizeof(Vertex));
        m_IndexBuffer    = gpu_memory_manager->AllocateIndexBuffer(REV::cast<REV::u32>(REV::ArrayCount(m_IndexData)));
        m_ConstantBuffer = gpu_memory_manager->AllocateConstantBuffer(sizeof(CBufferData), REV::StaticString<64>("MVPMatrix", REV_CSTRLEN("MVPMatrix")));
    }

    gpu_memory_manager->StartImmediateExecution();
    {
        gpu_memory_manager->SetResourceDataImmediate(m_VertexBuffer, m_VertexData);
        gpu_memory_manager->SetResourceDataImmediate(m_IndexBuffer, m_IndexData);
        gpu_memory_manager->SetResourceDataImmediate(m_ConstantBuffer, &m_CBufferData);
    }
    gpu_memory_manager->EndImmediateExecution();

    REV::GraphicsAPI::GetProgramManager()->AttachResource(m_GraphicsProgram, m_ConstantBuffer);
}

void DemoScene::OnUnsetCurrent()
{
    REV::GraphicsAPI::GetMemoryManager()->FreeMemory();
}

void DemoScene::OnUpdate()
{
    REV::Application    *application = REV::Application::Get();
    const REV::Keyboard& keyboard    = application->GetInput()->GetKeyboard();
    REV::Window&         window      = application->GetWindow();
    REV::Timer&          timer       = application->GetTimer();

    REV_LOCAL REV::f32 last_print_time;
    if (!window.Fullscreened() && timer.Seconds() - last_print_time >= 0.1)
    {
        REV::f32 FPS = timer.TicksPerSecond() / REV::cast<REV::f32>(timer.DeltaTicks());

        REV::StaticStringBuilder<128> ssb;
        ssb.Precision = 2;
        ssb.Build(m_OriginalWindowTitle, " - FPS: ", FPS, " - MSPF: ", 1000.0f / FPS);

        window.SetTitle(ssb.ToString());

        last_print_time = timer.Seconds();
    }

    if (keyboard[REV::KEY::F11].Pressed())
    {
        window.RequstFullscreen(!window.Fullscreened());
    }
    else if (keyboard[REV::KEY::V].Pressed())
    {
        REV::Renderer *renderer = REV::GraphicsAPI::GetRenderer();
        renderer->SetVSync(!renderer->VSyncEnabled());
    }
    
    REV::Math::v4 translation;

    if (keyboard[REV::KEY::LEFT].Down())
    {
        translation.x = -timer.DeltaSeconds();
    }
    else if (keyboard[REV::KEY::RIGHT].Down())
    {
        translation.x = timer.DeltaSeconds();
    }
    if (keyboard[REV::KEY::UP].Down())
    {
        translation.y = timer.DeltaSeconds();
    }
    else if (keyboard[REV::KEY::DOWN].Down())
    {
        translation.y = -timer.DeltaSeconds();
    }

    m_Translation *= REV::Math::m4::translation(translation);

    m_CBufferData.MVP = m_Translation * REV::Math::m4::rotation_z(timer.Seconds());

    REV::Math::v4 center = m_CBufferData.MVP * ((m_VertexData[0].pos + m_VertexData[2].pos) / 2.0f);
    m_CBufferData.center = center.xyz;

    {
        REV::GraphicsAPI::GetMemoryManager()->SetResoucreData(m_ConstantBuffer, &m_CBufferData);

        REV::GPU::ProgramManager *program_manager = REV::GraphicsAPI::GetProgramManager();
        program_manager->SetCurrentGraphicsProgram(m_GraphicsProgram);

        program_manager->BindVertexBuffer(m_GraphicsProgram, m_VertexBuffer);
        program_manager->BindIndexBuffer(m_GraphicsProgram, m_IndexBuffer);
        program_manager->DrawIndices(m_GraphicsProgram);
    }
}
