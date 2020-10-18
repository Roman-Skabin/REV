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
    : m_Logger("Engine logger", "../log/cengine.log", Logger::TARGET::FILE),
      m_Memory(Memory::Get()),
      m_Allocator(m_Memory->PushToPermanentArea(GB(1)), GB(1), false),
      m_WorkQueue(WorkQueue::Create(m_Logger)),
      m_Window(m_Logger,
               name,
               v4s(10, 10, 960, 540)),
      m_Input(Input::Create(m_Window, m_Logger)),
      m_Timer("EngineMainTimer"),
      m_AppComponents(&m_Allocator)
{
    CheckM(!s_Application,
           "Only one application alowed. "
           "Application \"%s\" is already created",
           s_Application->m_Window.m_Title);
    s_Application = this;

    GraphicsAPI::SetGraphicsAPI(api);
    GraphicsAPI::CreateRenderer(&m_Window, m_Logger, v2(1920.0f, 1080.0f));
    GraphicsAPI::CreateGPUMemoryManager(&m_Allocator, m_Logger, GB(2ui64));
    GraphicsAPI::CreateGPUProgramManager(&m_Allocator);
}

Application::~Application()
{
    GraphicsAPI::GetGPUProgramManager()->Destroy();
    GraphicsAPI::GetGPUMemoryManager()->Destroy();
    GraphicsAPI::GetRenderer()->Destroy();
    s_Application = null;
}

void Application::Run()
{
    IRenderer *renderer = GraphicsAPI::GetRenderer();

    m_Timer.Start();

    m_Window.Show();
    while (!m_Window.Closed())
    {
        m_Memory->ResetTransientArea();
        m_Window.Resset();
        m_Input->Reset();

        m_Window.PollEvents();

        if (!m_Window.Closed() && !m_Window.Minimized())
        {
            m_Input->Update(m_Logger);
            m_Timer.Tick();
            m_Window.ApplyFullscreenRequst();

            renderer->StartFrame();
            for (AppComponent *component : m_AppComponents) // @TODO(Roman): MT?
            {
                component->OnUpdate();
            }
            renderer->EndFrame();
        }
    }

    m_Timer.Stop();
    renderer->FlushGPU();
}
