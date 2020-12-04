//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "application.h"

Application *Application::s_Application = null;

Application *Application::Get()
{
    CheckM(s_Application, "Application is not created yet");
    return s_Application;
}

Application::Application(const StaticString<128>& name, GraphicsAPI::API api)
    : m_Logger("Engine logger", "../log/engine.log", Logger::TARGET::FILE),
      m_Memory(Memory::Get()),
      m_Allocator(m_Memory->PushToPermanentArea(GB(1)), GB(1), false, "Internal"),
      m_WorkQueue(WorkQueue::Create(m_Logger)),
      m_Window(m_Logger,
               name,
               Math::v4s(10, 10, 960, 540)),
      m_Input(Input::Create(m_Window, m_Logger)),
      m_Timer("EngineMainTimer"),
      m_SceneManager(SceneManager::Create(MB(16)))
{
    CheckM(!s_Application,
           "Only one application alowed. "
           "Application \"%s\" is already created",
           s_Application->m_Window.m_Title);
    s_Application = this;

    GraphicsAPI::SetGraphicsAPI(api);
    GraphicsAPI::Init(&m_Window, &m_Allocator, m_Logger, Math::v2s(1920, 1080));
}

Application::~Application()
{
    GraphicsAPI::Destroy();
    s_Application = null;
}

void Application::Run()
{
    Renderer *renderer = GraphicsAPI::GetRenderer();

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
            m_SceneManager->CurrentScene()->OnUpdate();
            renderer->EndFrame();
        }
    }

    m_WorkQueue->AddWork([this    ]{ m_Timer.Stop();         });
    m_WorkQueue->AddWork([renderer]{ renderer->WaitForGPU(); });
    m_WorkQueue->Wait();
}
