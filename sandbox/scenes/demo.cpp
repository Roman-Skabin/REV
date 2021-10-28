#include "scenes/demo.h"
#include "application.h"
#include "tools/static_string_builder.hpp"
#include "sandbox.h"

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

    REV::GPU::MemoryManager *gpu_memory_manager = REV::GraphicsAPI::GetMemoryManager();
    REV::AssetManager       *asset_manager      = REV::AssetManager::Get();

    m_CBuffer                           = gpu_memory_manager->AllocateConstantBuffer(sizeof(CBufferData), false, REV::ConstString(REV_CSTR_ARGS("DemoSceneCB")));
    REV::AssetHandle wood_texture_asset = asset_manager->LoadTexture(REV::ConstString(REV_CSTR_ARGS("Wood")), false);

    Sandbox *sandbox = static_cast<Sandbox *>(REV::Application::Get());

    REV::GPU::ShaderResourceDesc demo_shader_resources_descs[] = {
        { m_CBuffer, 0, 0 },
        { asset_manager->GetAsset(wood_texture_asset)->texture, 0, 0 },
        { gpu_memory_manager->AllocateSampler(REV::GPU::TEXTURE_ADDRESS_MODE_WRAP, REV::Math::v4(1.0f), REV::Math::v2(0.0f, 100.0f), false), 0, 0 },
        { sandbox->GetMousePickTexture(), 0, 0 }
    };

    m_DemoShader = asset_manager->LoadShader(REV::LoadShaderDesc(REV::ConstString(REV_CSTR_ARGS("demo_shader")),
                                                                 REV::ConstArray(REV_CARRAY_ARGS(demo_shader_resources_descs))),
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

    //
    // Update Rect
    //
    REV::Math::v2 mouse_translation;
    if (mouse.LeftButton().Down())
    {
        Sandbox *sandbox   = cast(Sandbox *, REV::Application::Get());
        u32      entity_id = *cast(u32 *, GetTexturePixelData(REV::GraphicsAPI::GetMemoryManager()->GetTextureData(sandbox->GetMousePickTexture()),
                                                              0, 0, 0,
                                                              REV::GPU::TEXTURE_FORMAT_U32,
                                                              REV::Math::v2s_to_v2u(mouse.Pos())));
        if (entity_id == m_Rect.ID)
        {
            mouse_translation = REV::Math::v2s_to_v2(mouse.DeltaPos()) * timer.DeltaSeconds();
        }
    }

    REV::Math::v4 translation;
    /**/ if (keyboard[REV::KEY::LEFT ].Down()) translation.x = -timer.DeltaSeconds();
    else if (keyboard[REV::KEY::RIGHT].Down()) translation.x =  timer.DeltaSeconds();
    /**/ if (keyboard[REV::KEY::UP   ].Down()) translation.y =  timer.DeltaSeconds();
    else if (keyboard[REV::KEY::DOWN ].Down()) translation.y = -timer.DeltaSeconds();
    m_Translation *= REV::Math::m4::translation(translation + REV::Math::v4(mouse_translation, 0.0f, 0.0f));

    m_CBufferData.mvp       = m_Translation;
    m_CBufferData.center    = (m_CBufferData.mvp * ((m_Rect.vertices[0].position + m_Rect.vertices[2].position) / 2.0f)).xyz;
    m_CBufferData.entity_id = m_Rect.ID;

    //
    // Set demo shader
    //
    SetCurrentGraphicsShader(m_DemoShader);

    //
    // Render Rect
    //
    SubmitEntity(&m_Rect);
}
