#include "scenes/demo.h"
#include "application.h"
#include "tools/static_string_builder.hpp"

DemoScene::DemoScene(REV::Allocator *allocator)
    : SceneBase(allocator, REV::ConstString(REV_CSTR_ARGS("DemoScene")), 1024, 36*1024),
      m_CBuffer(),
      m_CBufferData{ REV::Math::m4::identity(), REV::Math::v4(252.0f, 212.0f, 64.0f, 255.0f) / 255.0f, REV::Math::v3() },
      m_Translation(REV::Math::m4::identity()),
      m_Rect(),
      m_OriginalWindowTitle(REV::Application::Get()->GetWindow().Title())
{
}

DemoScene::~DemoScene()
{
}

void DemoScene::OnSetCurrent()
{
    REV::Vertex vertices[] =
    {
        { REV::Math::v4(-0.5f, -0.5f, 0.5f, 1.0f), REV::Math::v4(), REV::Math::v2(0.0f, 1.0f) },
        { REV::Math::v4(-0.5f,  0.5f, 0.5f, 1.0f), REV::Math::v4(), REV::Math::v2(0.0f, 0.0f) },
        { REV::Math::v4( 0.5f,  0.5f, 0.5f, 1.0f), REV::Math::v4(), REV::Math::v2(1.0f, 0.0f) },
        { REV::Math::v4( 0.5f, -0.5f, 0.5f, 1.0f), REV::Math::v4(), REV::Math::v2(1.0f, 1.0f) },
    };
    REV::Index indices[] =
    {
        0, 1, 2,
        0, 2, 3
    };

    m_Rect.Create(REV::ArrayCount(vertices), REV::ArrayCount(indices));
    m_Rect.SetData(REV::ConstArray(REV_CARRAY_ARGS(vertices)), REV::ConstArray(REV_CARRAY_ARGS(indices)));

    REV::AssetManager       *asset_manager      = REV::AssetManager::Get();
    REV::GPU::MemoryManager *gpu_memory_manager = REV::GraphicsAPI::GetMemoryManager();

    REV::AssetHandle demo_shader_resources[] =
    {
        asset_manager->LoadTexture(REV::LoadTextureDesc(REV::ConstString(REV_CSTR_ARGS("Wood")), 0, 0), false)
    };

    REV::GPU::CBufferDesc demo_shader_cbuffers[] =
    {
        { m_CBuffer = gpu_memory_manager->AllocateConstantBuffer(sizeof(CBufferData), false, REV::ConstString(REV_CSTR_ARGS("DemoSceneCB"))), 0, 0 }
    };

    REV::GPU::SamplerDesc demo_shader_samplers[] =
    {
        { gpu_memory_manager->AllocateSampler(REV::GPU::TEXTURE_ADDRESS_MODE_WRAP, REV::Math::v4(1.0f), REV::Math::v2(0.0f, 100.0f), false), 0, 0 }
    };

    m_DemoShader = asset_manager->LoadShader(REV::LoadShaderDesc(REV::ConstString(REV_CSTR_ARGS("demo_shader")),
                                                                 REV::ConstArray(REV_CARRAY_ARGS(demo_shader_resources)),
                                                                 REV::ConstArray(REV_CARRAY_ARGS(demo_shader_cbuffers)),
                                                                 REV::ConstArray(REV_CARRAY_ARGS(demo_shader_samplers))),
                                             false);
}

void DemoScene::OnUnsetCurrent()
{
}

void DemoScene::OnSetResourcesData()
{
    REV::GPU::MemoryManager *gpu_memory_manager = REV::GraphicsAPI::GetMemoryManager();
    gpu_memory_manager->SetBufferData(m_CBuffer, &m_CBufferData);
}

void DemoScene::OnUpdate()
{
    REV::Application    *application = REV::Application::Get();
    const REV::Keyboard& keyboard    = application->GetInput()->GetKeyboard();
    const REV::Mouse&    mouse       = application->GetInput()->GetMouse();
    REV::Window&         window      = application->GetWindow();
    REV::Timer&          timer       = application->GetTimer();

    REV_LOCAL f32 last_print_time;
    if (!window.Fullscreened() && timer.Seconds() - last_print_time >= 0.1)
    {
        f32 FPS = timer.TicksPerSecond() / cast(f32, timer.DeltaTicks());

        REV::StaticStringBuilder<128> ssb;
        ssb.m_FloatFormat.Precision = 2;
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
        REV::GPU::DeviceContext *device_context = REV::GraphicsAPI::GetDeviceContext();
        device_context->SetVSync(!device_context->VSyncEnabled());
    }

    REV::Math::v4 translation;
    /**/ if (keyboard[REV::KEY::LEFT ].Down()) translation.x = -timer.DeltaSeconds();
    else if (keyboard[REV::KEY::RIGHT].Down()) translation.x =  timer.DeltaSeconds();
    /**/ if (keyboard[REV::KEY::UP   ].Down()) translation.y =  timer.DeltaSeconds();
    else if (keyboard[REV::KEY::DOWN ].Down()) translation.y = -timer.DeltaSeconds();
    /**/ if (mouse.DeltaWheel())               translation.z = -mouse.DeltaWheel() * timer.DeltaSeconds();
    m_Translation *= REV::Math::m4::translation(translation);

    m_CBufferData.mvp    = m_Translation; // * REV::Math::m4::rotation_z(timer.Seconds());
    m_CBufferData.center = (m_CBufferData.mvp * ((m_Rect.vertices[0].position + m_Rect.vertices[2].position) / 2.0f)).xyz;

    SetCurrentGraphicsShader(m_DemoShader);
    SubmitEntity(&m_Rect);
}
