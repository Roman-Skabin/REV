//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "application.h"

namespace REV
{

Application *Application::s_Application = null;

Application *Application::Get()
{
    REV_CHECK_M(s_Application, "Application is not created yet");
    return s_Application;
}

Application::Application(const StaticString<128>& name, const StaticString<REV_PATH_CAPACITY>& revam_file)
    : m_Logger("REV logger", "../log/rev.log", Logger::TARGET::FILE),
      m_Memory(Memory::Get()),
      m_Allocator(m_Memory->PushToPermanentArea(GB(1)), GB(1), false, "Default"),
      m_WorkQueue(WorkQueue::Create(m_Logger)),
      m_Settings(Settings::Init("../bin/settings.ini")),
      m_Window(m_Logger, name),
      m_Input(Input::Create(m_Window, m_Logger)),
      m_Timer("REVMainTimer"),
      m_CurrentScene(null),
      m_AssetManager(null)
{
    REV_CHECK_M(!s_Application,
                "Only one application alowed. "
                "Application \"%s\" is already created",
                s_Application->m_Window.m_Title);
    s_Application = this;

    GraphicsAPI::SetGraphicsAPI(m_Settings->graphics_api);
    GraphicsAPI::Init(&m_Window, &m_Allocator, m_Logger);

    // @NOTE(Roman): AssetManager must be created after Graphics API initializeation.
    m_AssetManager = AssetManager::Create(&m_Allocator, revam_file);
}

Application::~Application()
{
    m_AssetManager->~AssetManager();
    GraphicsAPI::Destroy();
    s_Application = null;
}

void Application::SetCurrentScene(SceneBase *scene)
{
    if (m_CurrentScene) m_CurrentScene->OnUnsetCurrentEx();
    m_CurrentScene = scene;
    m_CurrentScene->OnSetCurrentEx();
}

void Application::Run()
{
    GPU::Renderer *renderer = GraphicsAPI::GetRenderer();

    m_Timer.Start();

    m_Window.Show();
    while (!m_Window.Closed())
    {
        m_WorkQueue->AddWork([this]{ m_Memory->ResetTransientArea(); });
        m_WorkQueue->AddWork([this]{ m_Window.Resset();              });
        m_WorkQueue->AddWork([this]{ m_Input->Reset();               });
        m_WorkQueue->Wait();

        m_Window.PollEvents();

        if (!m_Window.Closed() && !m_Window.Minimized())
        {
            m_Input->Update(m_Logger);
            m_Timer.Tick();
            m_Window.ApplyFullscreenRequest();

            renderer->StartFrame();
            m_CurrentScene->OnUpdate();
            m_CurrentScene->FlushBatch();
            renderer->EndFrame();
        }
    }

    m_WorkQueue->AddWork([this    ]{ m_Timer.Stop();         });
    m_WorkQueue->AddWork([renderer]{ renderer->WaitForGPU(); });
    m_WorkQueue->Wait();

    m_CurrentScene->OnUnsetCurrentEx();
}

}
