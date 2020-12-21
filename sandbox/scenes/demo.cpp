#include "scenes/demo.h"
#include "application.h"
#include "tools/static_string_builder.hpp"

DemoScene::DemoScene(REV::Allocator *allocator)
    : SceneBase(allocator, "DemoScene", "../sandbox/assets/shaders/demo_shaders.hlsl", 1024, 36*1024),
      m_ConstantBuffer(),
      m_CBufferData{ REV::Math::m4::identity(), REV::Math::v4(252.0f, 212.0f, 64.0f, 255.0f) / REV::Math::v4(255.0f), REV::Math::v3() },
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
        m_ConstantBuffer = gpu_memory_manager->AllocateConstantBuffer(sizeof(m_CBufferData), REV::StaticString<64>("CB_DemoScene", REV_CSTRLEN("CB_DemoScene")));
    }
    REV::GraphicsAPI::GetProgramManager()->AttachResource(m_GraphicsProgram, m_ConstantBuffer);

    REV::Vertex vertices[] =
    {
        { REV::Math::v4(-0.5f, -0.5f, 0.5f, 1.0f), REV::Math::v4(), REV::Math::v2() },
        { REV::Math::v4(-0.5f,  0.5f, 0.5f, 1.0f), REV::Math::v4(), REV::Math::v2() },
        { REV::Math::v4( 0.5f,  0.5f, 0.5f, 1.0f), REV::Math::v4(), REV::Math::v2() },
        { REV::Math::v4( 0.5f, -0.5f, 0.5f, 1.0f), REV::Math::v4(), REV::Math::v2() },
    };
    REV::Index indices[] =
    {
        0, 1, 2,
        0, 2, 3
    };

    m_Rect.allocator = m_Allocator;
    m_Rect.Create(REV::ArrayCount(vertices), REV::ArrayCount(indices));
    CopyMemory(m_Rect.vertices, vertices, sizeof(vertices));
    CopyMemory(m_Rect.indices, indices, sizeof(indices));
}

void DemoScene::OnUnsetCurrent()
{
    m_Rect.Destroy();
}

void DemoScene::OnSetResourcesData()
{
    REV::GPU::MemoryManager *gpu_memory_manager = REV::GraphicsAPI::GetMemoryManager();
    {
        gpu_memory_manager->SetResourceData(m_ConstantBuffer, &m_CBufferData); 
    }
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
        REV::GPU::Renderer *renderer = REV::GraphicsAPI::GetRenderer();
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

    m_CBufferData.mvp    = m_Translation * REV::Math::m4::rotation_z(timer.Seconds());
    m_CBufferData.center = (m_CBufferData.mvp * ((m_Rect.vertices[0].position + m_Rect.vertices[2].position) / 2.0f)).xyz;

    SubmitEntity(&m_Rect);
}
