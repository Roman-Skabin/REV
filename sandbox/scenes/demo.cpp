#include "scenes/demo.h"
#include "application.h"
#include "tools/static_string_builder.hpp"
#include "sandbox.h"
#include "graphics/vertex_index_instance_buffers.h"

DemoScene::DemoScene(REV::Allocator *allocator)
    : Scene(REV::ConstString(REV_CSTR_ARGS("DemoScene"))),
      m_CBuffer(),
      m_CBufferData{ REV::Math::m4::identity(), REV::Math::v4(252.0f, 212.0f, 64.0f, 255.0f) / 255.0f, REV_INVALID_ENTITY_ID },
      m_Translation(REV::Math::m4::identity()),
      m_Rect("Rect"),
      m_DemoShader(),
      m_PickedEntity(REV_INVALID_ENTITY_ID),
      m_OriginalWindowTitle(REV::Application::Get()->GetWindow().Title())
{
}

DemoScene::~DemoScene()
{
}

void DemoScene::OnSetCurrent()
{
    m_Rect.SetVertices(REV::ConstArray<REV::VertexPosTex>(
    {
        { REV::Math::v4(-0.5f, -0.5f, 0.5f, 1.0f), REV::Math::v2(0.0f, 1.0f) },
        { REV::Math::v4(-0.5f,  0.5f, 0.5f, 1.0f), REV::Math::v2(0.0f, 0.0f) },
        { REV::Math::v4( 0.5f,  0.5f, 0.5f, 1.0f), REV::Math::v2(1.0f, 0.0f) },
        { REV::Math::v4( 0.5f, -0.5f, 0.5f, 1.0f), REV::Math::v2(1.0f, 1.0f) },
    }));

    m_Rect.SetIndices(REV::ConstArray<REV::Index16>(
    {
        0, 1, 2,
        0, 2, 3
    }));

    CreateResources();
    CreateShaders();
    AddOpaquePass();
}

void DemoScene::OnUnsetCurrent()
{
}

REV_INTERNAL REV::Math::v2 REV_VECTORCALL WindowToRTCoord(REV::Math::v2 win_xy)
{
    REV::Math::v2 rt_wh  = REV::Math::v2s_to_v2(REV::GraphicsAPI::GetDeviceContext()->RTSize());
    REV::Math::v2 win_wh = REV::Math::v2s_to_v2(REV::Application::Get()->GetWindow().Size());
    return (win_xy * rt_wh) / win_wh;
}

void DemoScene::OnUpdate()
{
    REV::Application          *application        = REV::Application::Get();
    Sandbox                   *sandbox            = cast(Sandbox *, application);
    const REV::Keyboard&       keyboard           = application->GetInput()->GetKeyboard();
    const REV::Mouse&          mouse              = application->GetInput()->GetMouse();
    REV::Window&               window             = application->GetWindow();
    REV::Timer&                timer              = application->GetTimer();
    const REV::ResourceHandle& mouse_pick_texture = sandbox->GetMousePickTexture();
    REV::MemoryManager        *gpu_memory_manager = REV::GraphicsAPI::GetMemoryManager();
    REV::DeviceContext        *device_context     = REV::GraphicsAPI::GetDeviceContext();

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
                                                          REV::TEXTURE_FORMAT_U32,
                                                          REV::Math::v2_to_v2u(WindowToRTCoord(REV::Math::v2s_to_v2(mouse.Pos())))));
    }
    else if (mouse.LeftButton().Released())
    {
        m_PickedEntity = REV_INVALID_ENTITY_ID;
    }

    //
    // Update Rect
    //
    REV::Math::v4 translation;
    if (m_PickedEntity == m_Rect.ID())
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
    m_CBufferData.entity_id = m_Rect.ID();

    gpu_memory_manager->SetBufferData(m_CBuffer, &m_CBufferData);
}

void DemoScene::CreateResources()
{
    REV::MemoryManager *gpu_memory_manager = REV::GraphicsAPI::GetMemoryManager();
    REV::AssetManager  *asset_manager      = REV::AssetManager::Get();

    m_CBuffer = gpu_memory_manager->AllocateConstantBuffer(sizeof(CBufferData), false, REV::ConstString(REV_CSTR_ARGS("DemoSceneCB")));

    REV::AssetHandle wood_texture_asset = asset_manager->LoadTexture(REV::ConstString(REV_CSTR_ARGS("Wood")), false);

    m_WoodTexture        = asset_manager->GetAsset(wood_texture_asset)->texture;
    m_WoodTextureSampler = gpu_memory_manager->AllocateSampler(REV::TEXTURE_ADDRESS_MODE_WRAP,
                                                               REV::Math::v4(1.0f),
                                                               REV::Math::v2(0.0f, 100.0f),
                                                               false);
}

void DemoScene::CreateShaders()
{
    REV::AssetManager *asset_manager = REV::AssetManager::Get();
    Sandbox           *sandbox       = cast(Sandbox *, REV::Application::Get());

    m_DemoShader = asset_manager->LoadShader(REV::LoadShaderDesc(REV::ConstString(REV_CSTR_ARGS("demo_shader")),
                                                                 REV::ConstArray<REV::ShaderResourceDesc>(
                                                                 {
                                                                     REV::ShaderResourceDesc::CBV(m_CBuffer, 0, 0),
                                                                     REV::ShaderResourceDesc::SRV(m_WoodTexture, 0, 0),
                                                                     REV::ShaderResourceDesc::Sampler(m_WoodTextureSampler, 0, 0)
                                                                     REV::ShaderResourceDesc::RTV(sandbox->GetColorTarget(), REV::Math::v4(0.2f, 0.2f, 0.2f, 1.0f)),
                                                                     REV::ShaderResourceDesc::RTV(sandbox->GetMousePickTexture(), REV::Math::v4()),
                                                                     REV::ShaderResourceDesc::DSV(sandbox->GetDepthTarget(), 0.0f),
                                                                 }),
                                                                 REV::ConstArray<REV::ConstString>(
                                                                 {
                                                                     REV::ConstString(REV_CSTR_ARGS("TEST_DEFINE=1"))
                                                                 })),
                                             false);
}

void DemoScene::AddOpaquePass()
{
    Sandbox                  *sandbox          = cast(Sandbox *, REV::Application::Get());
    REV::AssetManager        *asset_manager    = REV::AssetManager::Get();
    REV::ForwardPlusPipeline& forward_pipeline = sandbox->GetForwardPlusPipeline();

    REV::RenderPass& opaque_pass = forward_pipeline.AddRenderPass(REV::ConstString(REV_CSTR_ARGS("Opaque Pass")));
    {
        REV::RenderPassRTDesc color_rt_desc;
        color_rt_desc.resource_handle = sandbox->GetColorTarget();
        color_rt_desc.load_action     = REV::RT_DS_LOAD_ACTION_CLEAR;
        color_rt_desc.store_action    = REV::RT_DS_STORE_ACTION_STORE;
        color_rt_desc.clear_value     = REV::Math::v4(0.2f, 0.2f, 0.2f, 1.0f);

        REV::RenderPassRTDesc mouse_picker_rt_desc;
        mouse_picker_rt_desc.resource_handle = sandbox->GetMousePickTexture();
        mouse_picker_rt_desc.load_action     = REV::RT_DS_LOAD_ACTION_CLEAR;
        mouse_picker_rt_desc.store_action    = REV::RT_DS_STORE_ACTION_STORE;
        mouse_picker_rt_desc.clear_value     = REV::Math::v4();

        REV::RenderPassDSDesc ds_desc;
        ds_desc.resource_handle    = sandbox->GetDepthTarget();
        ds_desc.depth_load_action  = REV::RT_DS_LOAD_ACTION_CLEAR;
        ds_desc.depth_store_action = REV::RT_DS_STORE_ACTION_STORE;
        ds_desc.depth_clear_value  = 0.0f;

        opaque_pass.AddRenderTarget(color_rt_desc);
        opaque_pass.AddRenderTarget(mouse_picker_rt_desc);
        opaque_pass.SetDepthStencil(ds_desc);
    }
}
