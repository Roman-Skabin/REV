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
      m_DemoShader(),
      m_PickedEntity(0),
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

    REV::GPU::ShaderResourceDesc demo_shader_resources_descs[] =
    {
        REV::GPU::ShaderResourceDesc(m_CBuffer, 0, 0),
        REV::GPU::ShaderResourceDesc(asset_manager->GetAsset(wood_texture_asset)->texture, 0, 0),
        REV::GPU::ShaderResourceDesc(gpu_memory_manager->AllocateSampler(REV::GPU::TEXTURE_ADDRESS_MODE_WRAP, REV::Math::v4(1.0f), REV::Math::v2(0.0f, 100.0f), false), 0, 0),
        REV::GPU::ShaderResourceDesc(sandbox->GetMousePickTexture(), REV::Math::v4())
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

REV_INTERNAL REV::Math::v2 REV_VECTORCALL WindowToRTCoord(REV::Math::v2 win_xy)
{
    REV::Math::v2 rt_wh  = REV::Math::v2s_to_v2(REV::GraphicsAPI::GetDeviceContext()->RTSize());
    REV::Math::v2 win_wh = REV::Math::v2s_to_v2(REV::Application::Get()->GetWindow().Size());
    return (win_xy * rt_wh) / win_wh;
}

void DemoScene::OnUpdate()
{
    REV::Application               *application        = REV::Application::Get();
    Sandbox                        *sandbox            = cast(Sandbox *, application);
    const REV::Keyboard&            keyboard           = application->GetInput()->GetKeyboard();
    const REV::Mouse&               mouse              = application->GetInput()->GetMouse();
    REV::Window&                    window             = application->GetWindow();
    REV::Timer&                     timer              = application->GetTimer();
    const REV::GPU::ResourceHandle& mouse_pick_texture = sandbox->GetMousePickTexture();
    REV::GPU::MemoryManager        *gpu_memory_manager = REV::GraphicsAPI::GetMemoryManager();
    REV::GPU::DeviceContext        *device_context     = REV::GraphicsAPI::GetDeviceContext();

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
        device_context->SetVSync(!device_context->VSyncEnabled());
    }

    //
    // Update picked entity
    //
    if (mouse.LeftButton().Pressed())
    {
        m_PickedEntity = *cast(u32 *, GetTexturePixelData(gpu_memory_manager->GetTextureData(mouse_pick_texture),
                                                          0, 0, 0,
                                                          REV::GPU::TEXTURE_FORMAT_U32,
                                                          REV::Math::v2_to_v2u(WindowToRTCoord(REV::Math::v2s_to_v2(mouse.Pos())))));
    }
    else if (mouse.LeftButton().Released())
    {
        m_PickedEntity = 0;
    }

    //
    // Update Rect
    //
    REV::Math::v4 translation;
    if (m_PickedEntity == m_Rect.ID)
    {
        // @NOTE(Roman): delta mouse position in window space to delta mouse position in render target space also normalized to [-1, 1].

        REV::Math::v2 window_dimension          = REV::Math::v2s_to_v2(window.Size());
        REV::Math::v2 rt_dimension              = REV::Math::v2s_to_v2(device_context->RTSize());
        REV::Math::v2 window_mouse_delta_pos    = REV::Math::v2s_to_v2(mouse.DeltaPos());
        REV::Math::v2 current_window_mouse_pos  = REV::Math::v2s_to_v2(mouse.Pos());

        __m128 mm_window_dimension          = window_dimension.load();
        __m128 mm_rt_dimension              = rt_dimension.load();
        __m128 mm_window_mouse_delta_pos    = window_mouse_delta_pos.load();
        __m128 mm_current_window_mouse_pos  = current_window_mouse_pos.load();
        __m128 mm_previous_window_mouse_pos = _mm_sub_ps(mm_current_window_mouse_pos, mm_window_mouse_delta_pos);
        __m128 mm_0_5                       = _mm_set_ps1(0.5f);
        __m128 mm_2_0                       = _mm_set_ps1(2.0f);
        __m128 mm_0_n1                      = _mm_setr_ps(1.0f, -1.0f, 0.0f, 0.0f);

        __m128 mm_current_rt_mouse_pos  = _mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_mul_ps(mm_current_window_mouse_pos,  mm_rt_dimension), _mm_mul_ps(mm_rt_dimension, mm_window_dimension)), mm_0_5), mm_2_0);
        __m128 mm_previous_rt_mouse_pos = _mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_mul_ps(mm_previous_window_mouse_pos, mm_rt_dimension), _mm_mul_ps(mm_rt_dimension, mm_window_dimension)), mm_0_5), mm_2_0);

        translation.xy = _mm_mul_ps(_mm_sub_ps(mm_current_rt_mouse_pos, mm_previous_rt_mouse_pos), mm_0_n1);
    }
    else
    {
        /**/ if (keyboard[REV::KEY::LEFT ].Down()) translation.x = -timer.DeltaSeconds();
        else if (keyboard[REV::KEY::RIGHT].Down()) translation.x =  timer.DeltaSeconds();
        /**/ if (keyboard[REV::KEY::UP   ].Down()) translation.y =  timer.DeltaSeconds();
        else if (keyboard[REV::KEY::DOWN ].Down()) translation.y = -timer.DeltaSeconds();
    }
    m_Translation *= REV::Math::m4::translation(translation);

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
